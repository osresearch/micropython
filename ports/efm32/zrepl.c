#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "radio.h"
#include "zrepl.h"

extern uint8_t radio_mac_address[];
extern void uart_recv(uint8_t c);

int zrepl_active;


// "Multipurpose" frames are used (type 5), which have the minimum amount of
// overhead.  There are two valid Frame Control Fields (FCF), one
// for zrepl sends and one for zrepl receives.
#define ZREPL_FCF_SEND	(0 \
	| 5 << 0 /* Multipurpose frame */ \
	| 0 << 3 /* Short frame type */ \
	| 0 << 4 /* No dest address is present */ \
	| 3 << 6 /* Long source address is present */ \
	)

#define ZREPL_FCF_RECV	(0 \
	| 5 << 0 /* Multipurpose frame */ \
	| 0 << 3 /* Short frame type */ \
	| 3 << 4 /* Long dest address is present */ \
	| 0 << 6 /* No source address is present */ \
	)

typedef struct
{
	uint8_t fcf;
	uint8_t seq;
	uint8_t mac[8];
	uint8_t data[];
} zrepl_packet_t;

// hook the uart print function and send 802.15.4 messages
static uint8_t zrepl_seq = 0;

void zrepl_send(const char * str, size_t len)
{
	if (!zrepl_active)
		return;

	// turn it off for now
	zrepl_active = 0;

	while(len)
	{
		// wait up to 10 ms for any existing packets to go out
		// if there is a tx in process, we don't want to interfere
		// with the transfer.
		uint8_t buf[64];
		zrepl_packet_t * const msg = (void*) buf;
		if (!msg)
			break;

		// fill in the header
		msg->fcf = ZREPL_FCF_SEND;
		msg->seq = zrepl_seq++;
		memcpy(msg->mac, radio_mac_address, sizeof(msg->mac));

		const size_t max_data_len = 60 - sizeof(*msg);
		size_t data_len = len;
		if (data_len > max_data_len)
			data_len = max_data_len;

		// fill in the buffer, up to the length
		memcpy(msg->data, str, data_len);
		len -= data_len;
		str += data_len;

		// and put it on the wire; abort if there is a failure
		if (radio_tx_buffer_send(msg, sizeof(*msg) + data_len) != 0)
			; // return;
	}

	// turn it back on
	zrepl_active = 1;
}

// forwards characters from the 802.15.4 message into the uart recv function
// happens even if zrepl_active is not set.
// returns 0 if not for us, 1 if it was processed and should be ignored
int zrepl_recv(const void * raw, size_t len)
{
	const zrepl_packet_t * const msg = raw;

	// verify that this is a zrepl message to us
	if (len < sizeof(*msg))
		return 0;
	if (msg->fcf != ZREPL_FCF_RECV)
		return 0;
	if (memcmp(msg->mac, radio_mac_address, sizeof(msg->mac)) != 0)
		return 0;

	// this is addressed to us and the correct format.
	// process it as if the user had typed it into the
	len -= sizeof(*msg);
	const uint8_t * c = msg->data;
	while(len--)
		uart_recv(*c++);

	return 1;
};
