#pragma once
#include <stdint.h>
#include <sys/types.h>

enum http_request_t {
	HTTP_UNSET,
	HTTP_UNKNOWN,
	HTTP_CHUNKED,
	HTTP_CONTENT_LENGTH,
	HTTP_HEADER_ONLY
};

struct http_message_t {
	enum http_request_t type;

	size_t spare_filled;
	size_t spare_capacity;
	uint8_t *spare_buffer;

	size_t unreceived_size;
	uint8_t is_completed;

	// Detected from child packets
	size_t claimed_size;
	size_t received_size;
};

struct http_packet_t {

	// size of filled content
	size_t filled_size;
	size_t expected_size;

	// max capacity of buffer
	// can be exapanded
	size_t buffer_capacity;
	uint8_t *buffer;

	struct http_message_t *parent_message;

	uint8_t is_completed;
};

struct http_message_t *http_message_new(void);
void message_free(struct http_message_t *);

enum http_request_t packet_find_type(struct http_packet_t *pkt);
int packet_at_capacity(struct http_packet_t *);
int packet_pending_bytes(struct http_packet_t *);
void packet_mark_received(struct http_packet_t *, size_t);

struct http_packet_t *packet_new(struct http_message_t *);
void packet_free(struct http_packet_t *);
