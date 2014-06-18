#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <unistd.h>
#include <getopt.h>

#include "logging.h"
#include "http.h"
#include "tcp.h"
#include "usb.h"

static void start_daemon(uint32_t requested_port)
{
	// Capture USB device
	struct usb_sock_t *usb_sock = usb_open();
	if (usb_sock == NULL)
		goto cleanup_usb;

	// Capture a socket
	struct tcp_sock_t *tcp_socket = tcp_open(requested_port);
	if (tcp_socket == NULL)
		goto cleanup_tcp;

	// TODO: print port then fork
	uint32_t real_port = tcp_port_number_get(tcp_socket);
	if (requested_port != 0 && requested_port != real_port) {
		ERR("Received port number did not match requested port number. "
		    "The requested port number may be too high.");
		goto cleanup_tcp;
	}
	printf("%u\n", real_port);

	struct usb_conn_t *usb = usb_conn_get(usb_sock);
	while (1) {
		// TODO: spawn thread
		struct http_message_t *msg_client = NULL;
		struct http_message_t *msg_server = NULL;
		struct tcp_conn_t *tcp = tcp_conn_accept(tcp_socket);
		if (tcp == NULL) {
			ERR("Failed to open tcp connection");
			goto cleanup_conn;
		}
		msg_client = http_message_new();
		msg_server = http_message_new();
		if (msg_client == NULL || msg_server == NULL) {
			ERR("Creating messages failed");
			goto cleanup_conn;
		}

		while (!tcp->is_closed) {
			struct http_packet_t *pkt;

			// Client's request
			for (;;) {
				pkt = tcp_packet_get(tcp, msg_client);
				if (pkt == NULL)
					break;

				printf("%.*s", (int)pkt->filled_size, pkt->buffer);
				usb_conn_packet_send(usb, pkt);
				packet_free(pkt);
			}

			// Server's responce
			for (;;) {
				pkt = usb_conn_packet_get(usb, msg_server);
				if (pkt == NULL)
					break;

				printf("%.*s", (int)pkt->filled_size, pkt->buffer);
				tcp_packet_send(tcp, pkt);
				packet_free(pkt);
			}
		}



	cleanup_conn:
		if (msg_client != NULL)
			message_free(msg_client);
		if (msg_server != NULL)
			message_free(msg_server);
		if (tcp != NULL)
			tcp_conn_close(tcp);
		// TODO: when we fork make sure to return here
	}

cleanup_tcp:
	if (tcp_socket!= NULL)
		tcp_close(tcp_socket);
cleanup_usb:
	if (usb_sock != NULL)
		usb_close(usb_sock);
	return;
}

int main(int argc, char *argv[])
{
	int c;
	long long port = 0;
	int show_help = 0;
	setting_log_target = LOGGING_STDERR;

	// TODO: support long options
	while ((c = getopt(argc, argv, "hp:u:s:l")) != -1) {
		switch (c) {
		case '?':
		case 'h':
			show_help = 1;
			break;
		case 'p':
			// Request specific port
			port = atoi(optarg);
			if (port < 0) {
				ERR("Port number must be non-negative");
				return 1;
			}
			if (port > (long long)UINT_MAX) {
				ERR("Port number must be %u or less, "
				    "but not negative", UINT_MAX);
				return 2;
			}
			break;
		case 'u':
			// [u]sb device to bind with
			break;
		case 'l':
			// Redirect logging to syslog
			setting_log_target = LOGGING_SYSLOG;
			break;
		// TODO: support --syslog for more daemon like errors
		}
	}

	if (show_help) {
		printf(
		"Usage: %s -v <vendorid> -m <productid> -p <port>\n"
		"Options:\n"
		"  -h           Show this help message\n"
		"  -v <vid>     Vendor ID of desired printer\n"
		"  -m <pid>     Product ID of desired printer\n"
		"  -s <serial>  Serial number of desired printer\n"
		"  -p <portnum> Port number to bind against\n"
		"  -l           Redirect logging to syslog\n"
		, argv[0]);
		return 0;
	}

	start_daemon((uint32_t)port);
	return 0;
}
