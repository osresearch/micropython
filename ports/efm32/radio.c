/*
 * Interface to the EFM32 radio module in IEEE 802.15.4 mode
 */
#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objarray.h"
#include "py/mphal.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_core.h"
#include "radio.h"
#include "zrepl.h"

#include "rail.h"
#include "protocol/ieee802154/rail_ieee802154.h"

#ifndef RADIO_CHANNEL
#define RADIO_CHANNEL 11
#endif

uint8_t radio_mac_address[8];
uint16_t radio_short_address = 0xFFFF; // default value
uint16_t radio_pan_id = 0xFFFF; // default value
static bool radio_promiscuous;
static volatile int radio_tx_pending;
static int radio_channel;


typedef enum {
    RADIO_UNINIT,
    RADIO_INITING,
    RADIO_IDLE,
    RADIO_TX,
    RADIO_RX,
    RADIO_CALIBRATION
} siliconlabs_modem_state_t;

static siliconlabs_modem_state_t radio_state = RADIO_UNINIT;

static volatile RAIL_Handle_t rail = NULL;
static void rail_callback_events(RAIL_Handle_t rail, RAIL_Events_t events);

// can't be const since the buffer is used for writes
static RAIL_Config_t rail_config = {
	.eventsCallback		= rail_callback_events,
	.protocol		= NULL, // must be NULL for ieee802.15.4
	.scheduler		= NULL, // not multi-protocol
	.buffer			= {}, // must be zero
};

static const RAIL_DataConfig_t rail_data_config = {
	.txSource = TX_PACKET_DATA,
	.rxSource = RX_PACKET_DATA,
	.txMethod = PACKET_MODE,
	.rxMethod = PACKET_MODE,
};


static const RAIL_IEEE802154_Config_t ieee802154_config = {
	// if zrepl is used for receiving, then promiscuous must be
	// enabled since filtering on multipurpose frames is not supported
	.promiscuousMode	= false,
	.isPanCoordinator	= false,
	.framesMask		= RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES,
	.ackConfig = {
		.enable			= true, // enable autoack
		.ackTimeout		= 54 * 16, // 54 symbols * 16 us/symbol = 864 usec
		.rxTransitions = {
			.success = RAIL_RF_STATE_RX, // go to Tx to send the ACK
			.error = RAIL_RF_STATE_RX, // ignored
		},
		.txTransitions = {
			.success = RAIL_RF_STATE_RX, // go to Rx for receiving the ACK
			.error = RAIL_RF_STATE_RX, // ignored
		},
	},
	.timings = {
		.idleToRx		= 100,
		.idleToTx		= 100,
		.rxToTx			= 192, // 12 symbols * 16 us/symbol
		.txToRx			= 192 - 10, // slightly lower to make sure we get to RX in time
		.rxSearchTimeout	= 0, // not used
		.txToRxSearchTimeout	= 0, // not used
	},
	.addresses		= NULL, // will be set by explicit calls
};

static const RAIL_TxPowerConfig_t paInit2p4 = {
	.mode			= RAIL_TX_POWER_MODE_2P4_HP,
	.voltage		= 1800,
	.rampTime		= 10,
};

static const RAIL_CsmaConfig_t csma_config = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;

/*
 * Called when radio calibration is required
 */
void RAILCb_CalNeeded()
{
	printf("calibrateRadio\n");
	//calibrateRadio = true;
}

static void rail_callback_rfready(RAIL_Handle_t rail)
{
	radio_state = RADIO_IDLE;
	radio_tx_pending = 0;
}

#define MAX_PKTS 4
static volatile unsigned rx_buffer_write;
static volatile unsigned rx_buffer_read;
static uint8_t rx_buffers[MAX_PKTS][MAC_PACKET_MAX_LENGTH];
static uint8_t rx_buffer_copy[MAC_PACKET_MAX_LENGTH];
static uint8_t radio_tx_buffer[MAC_PACKET_MAX_LENGTH];


#define FRAME_TYPE_ACK	0x02

/*
 * Send a pre-formed ACK message ASAP in the RX path,
 * using the AutoAckFifo.  Needs to fill in the sequence
 * number from the message, so this is not safe to call from
 * outside the RX interrupt.
 */
static int
radio_tx_autoack(uint8_t seq)
{
	static uint8_t ack_buf[] = {
		0x05,	// length, including FCS
		FRAME_TYPE_ACK,
		0x00,   // FCF bits that we don't care about
		0x00,	// seq goes here
	};

	ack_buf[3] = seq;
	int rc = RAIL_WriteAutoAckFifo(rail, ack_buf, 5);
	if (rc != RAIL_STATUS_NO_ERROR)
		printf("ack rc=%d\n", rc);
	return rc;
}


static void process_packet(RAIL_Handle_t rail)
{
	RAIL_RxPacketInfo_t info;
	RAIL_RxPacketHandle_t handle = RAIL_GetRxPacketInfo(rail, RAIL_RX_PACKET_HANDLE_NEWEST, &info);

	// not a valid receive? discard it.
	if (info.packetStatus != RAIL_RX_PACKET_READY_SUCCESS)
		goto done;
	// too long? discard it.
	if (info.packetBytes > MAC_PACKET_MAX_LENGTH)
		goto done;

	// check for an ack packet and turn off the auto ack for this rx
	uint8_t header[4];
	RAIL_PeekRxPacket(rail, handle, header, sizeof(header), 0);
	const uint8_t len = header[0];
	const uint8_t packet_type = header[1] & 0x03;
	const uint8_t ack_requested = header[1] & 0x20;
	if (len == 5 && packet_type == FRAME_TYPE_ACK)
	{
		// this is an ack, so don't send a reply
		RAIL_CancelAutoAck(rail);
		// could check to see if this acks our last packet
		// and not forward it up the stack, but we let it pass for now
		//waiting_for_ack = false;
		//last_ack_pending_bit = (header[1] & (1 << 4)) != 0;
		//printf("got ack\n");
	} else
	if (ack_requested && !radio_promiscuous)
	{
		// ACK requested and to us, assume the sequence number
		// is the third byte in the header (after the length byte,
		// and the two bytes of the FCF).
		radio_tx_autoack(header[3]);
	}

	{
		RAIL_RxPacketDetails_t details;
		details.timeReceived.timePosition = RAIL_PACKET_TIME_DEFAULT;
		details.timeReceived.totalPacketBytes = 0;
		RAIL_GetRxPacketDetails(rail, handle, &details);

		unsigned write_index = rx_buffer_write;
		uint8_t * rx_buffer = rx_buffers[write_index];
		if (info.packetBytes > MAC_PACKET_MAX_LENGTH - 2)
		{
			// should never happen?
			printf("rx too long %d\n", info.packetBytes);
		} else {
			RAIL_CopyRxPacket(rx_buffer, &info); // puts the length in byte 0

			// allow the zrepl to process this message
			// length is in the first byte and includes
			// an extra two bytes at the end that we ignore
			if (zrepl_recv(rx_buffer+1, *rx_buffer - 2))
			{
				// consumed by zrepl, do not forward it
			} else
			if (write_index == MAX_PKTS - 1)
				rx_buffer_write = 0;
			else
				rx_buffer_write = write_index + 1;
		}

/*
		// cancel the ACK if the sender did not request one
		// buffer[0] == length
		// buffer[1] == frame_type[0:2], security[3], frame_pending[4], ack_req[5], intrapan[6]
		// buffer[2] == destmode[2:3], version[4:5], srcmode[6:7]
		if ((rx_buffer[1] & (1 << 5)) == 0)
			RAIL_CancelAutoAck(rail);
*/

#if 0
		printf("rx %2d bytes lqi=%d rssi=%d:", info.packetBytes, details.lqi, details.rssi);

		// skip the length byte
		for(int i = 1 ; i < info.packetBytes ; i++)
		{
			printf(" %02x", rx_buffer[i]);
		}
		printf("\n");
#endif
	}

done:
	RAIL_ReleaseRxPacket(rail, handle);
}


/*
 * Callbank from the radio interrupt when there is an event.
 * rx 00000000:00018030
 */

static void rail_callback_events(RAIL_Handle_t rail, RAIL_Events_t events)
{
	if(0)
	{
		// ignore certain bit patterns unless other things are set
		events &= ~( 0
			| RAIL_EVENT_RX_PREAMBLE_LOST
			| RAIL_EVENT_RX_PREAMBLE_DETECT
			| RAIL_EVENT_RX_TIMING_LOST
			| RAIL_EVENT_RX_TIMING_DETECT
		);

		if(events == 0)
			return;

		printf("rx %08x:%08x\n",
			(unsigned int)(events >> 32),
			(unsigned int)(events >> 0));
	}

	if (events & RAIL_EVENT_RSSI_AVERAGE_DONE)
	{
		printf("rssi %d\n", RAIL_GetAverageRssi(rail));
	}

	if (events & RAIL_EVENT_RX_ACK_TIMEOUT)
	{
		//
	}

	if (events & RAIL_EVENT_RX_PACKET_RECEIVED)
	{
		process_packet(rail);
	}

	if (events & RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND)
	{
           /*
            * Indicates a Data Request is being received when using IEEE 802.15.4
            * functionality. This occurs when the command byte of an incoming frame is
            * for a data request, which requests an ACK. This callback is called before
            * the packet is fully received to allow the node to have more time to decide
            * whether to set the frame pending in the outgoing ACK. This event only ever
            * occurs if the RAIL IEEE 802.15.4 functionality is enabled.
            *
            * Call \ref RAIL_IEEE802154_GetAddress to get the source address of the
            * packet.
            */
	}

	if (events & RAIL_EVENTS_TX_COMPLETION)
	{
		// they are done with our packet, either for good or worse
		//printf("TX done!\n");
		radio_tx_pending = 0;
	}

	if (events & RAIL_EVENT_CAL_NEEDED)
	{
		// we should flag that a calibration is needed
		// does this cancel any transmits?
		radio_tx_pending = 0;
	}

	// lots of other events that we don't handle
}

static void radio_channel_set(unsigned channel)
{
	RAIL_Idle(rail, RAIL_IDLE_FORCE_SHUTDOWN_CLEAR_FLAGS, true);
	radio_state = RADIO_RX;
	radio_channel = channel;
	RAIL_StartRx(rail, channel, NULL);
}

void radio_init(void)
{
	// do not re-init
	if (radio_state != RADIO_UNINIT)
		return;

	// prevent recursive entry if zrepl is active
	const int old_zrepl = zrepl_active;
	zrepl_active = 0;

	if(1)
	{
		RAIL_Version_t version;
		RAIL_GetVersion(&version, true);
		printf("mac=%08x:%08x",
			(unsigned int) DEVINFO->UNIQUEH,
			(unsigned int) DEVINFO->UNIQUEL
		);

		printf(" rail=%d.%d-%d build %d flags %d (%08x)%s\n",
			version.major,
			version.minor,
			version.rev,
			version.build,
			version.flags,
			(unsigned int) version.hash,
			version.multiprotocol ? " multiprotocol" : ""
		);
	}

	rail = RAIL_Init(&rail_config, rail_callback_rfready);
	RAIL_ConfigData(rail, &rail_data_config);
	RAIL_ConfigCal(rail, RAIL_CAL_ALL);
	RAIL_IEEE802154_Config2p4GHzRadio(rail);
	RAIL_IEEE802154_Init(rail, &ieee802154_config);
	RAIL_ConfigEvents(rail, RAIL_EVENTS_ALL, 0
		| RAIL_EVENT_RSSI_AVERAGE_DONE
		| RAIL_EVENT_RX_ACK_TIMEOUT
		| RAIL_EVENT_RX_PACKET_RECEIVED
		| RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND
		| RAIL_EVENTS_TX_COMPLETION
		| RAIL_EVENTS_TXACK_COMPLETION
		| RAIL_EVENT_CAL_NEEDED
	);

	RAIL_ConfigTxPower(rail, &paInit2p4);
	RAIL_SetTxPower(rail, 255); // max

	// use the device unique id as the mac for network index 0
	memcpy(&radio_mac_address[0], (const void*)&DEVINFO->UNIQUEL, 4);
	memcpy(&radio_mac_address[4], (const void*)&DEVINFO->UNIQUEH, 4);
	RAIL_IEEE802154_SetLongAddress(rail, radio_mac_address, 0);

	// set the short address to something other than 0
	RAIL_IEEE802154_SetShortAddress(rail, radio_short_address, 0);
	RAIL_IEEE802154_SetPanId(rail, radio_pan_id, 0);

	// always use the same tx buffer
	RAIL_SetTxFifo(rail, radio_tx_buffer, 0, sizeof(radio_tx_buffer));

	// unpause auto-ack
	RAIL_PauseRxAutoAck(rail, false);

	// cache the current promiscuous mode
	radio_promiscuous = ieee802154_config.promiscuousMode;

	radio_channel_set(RADIO_CHANNEL);
	zrepl_active = old_zrepl;
}

mp_obj_t py_radio_init(void)
{
	radio_init();
	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(radio_init_obj, py_radio_init);


/*
 * Create a byte array and copy the received bytes into it.
 * This was using a long-lived buffer, but that was causing
 * GC issues. It might be worth revisiting to avoid the allocation cost.
 */
static mp_obj_t radio_rxbytes_get(void)
{
	if (radio_state == RADIO_UNINIT)
		radio_init();

	if (rx_buffer_write == rx_buffer_read)
		return mp_const_none;

	mp_obj_array_t * buf = mp_obj_new_bytearray_by_ref(
		sizeof(rx_buffer_copy), rx_buffer_copy);
	if (!buf)
		return mp_const_none;

	// resize the buffer for the return code and copy into it
	//mp_obj_array_t * buf = MP_OBJ_TO_PTR(rx_buffer_bytearray);
	unsigned read_index = rx_buffer_read;
	const uint8_t * const rx_buffer = rx_buffers[read_index];

	CORE_ATOMIC_IRQ_DISABLE();

	const uint8_t len = rx_buffer[0];

	if (len > MAC_PACKET_MAX_LENGTH - 2)
	{
		// discard this packet? should signal something
		buf->len = MAC_PACKET_MAX_LENGTH - 2;
	} else
	if (len < 2)
	{
		// malformed packet? lost race? huh?
		buf->len = 0;
	} else
		buf->len = len - 2;

	memcpy(rx_buffer_copy, rx_buffer+1, buf->len);
	if (read_index == MAX_PKTS - 1)
		rx_buffer_read = 0;
	else
		rx_buffer_read = read_index + 1;

	CORE_ATOMIC_IRQ_ENABLE();

	return MP_OBJ_FROM_PTR(buf);
}

MP_DEFINE_CONST_FUN_OBJ_0(radio_rxbytes_obj, radio_rxbytes_get);


// returns 0 if ok, non-zero if not
int radio_tx_buffer_send(const void * buf, size_t len)
{
	//CORE_ATOMIC_IRQ_DISABLE();

	// radio tx length including the 2 byte FCS at the end
	//*(volatile uint8_t*) &radio_tx_buffer[0] = 2 + len;
	uint8_t tx_len = 2 + len;
	RAIL_WriteTxFifo(rail, &tx_len, 1, true);
	RAIL_WriteTxFifo(rail, buf, len, false);

	radio_tx_pending = 1;
	radio_state = RADIO_TX;

	// do we have to quiese the radio here?
	//RAIL_Idle(rail, RAIL_IDLE_ABORT, true);

	RAIL_TxOptions_t txOpt = RAIL_TX_OPTIONS_DEFAULT;

	// if this is not a multipurpose frame (0x5) and the FCF has
	// ack requested, then tell the radio to stay online to wait for
	// the ACK to arrive.
	const uint8_t fcf = radio_tx_buffer[1];
	if ((fcf & 0x07) != 0x05
	&&  (fcf & 0x20) != 0x00)
		txOpt |= RAIL_TX_OPTION_WAIT_FOR_ACK;

	// start the transmit, we hope!
	int rc = RAIL_StartCcaCsmaTx(
		rail,
		radio_channel,
		txOpt,
		&csma_config,
		NULL
	);

	// if it failed to start, unset the pending flag
	if (rc != 0)
		radio_tx_pending = 0;

	//CORE_ATOMIC_IRQ_ENABLE();

	return rc;
}


/*
 * Send a byte buffer to the radio.
 */
static mp_obj_t radio_txbytes(mp_obj_t buf_obj)
{
	if (radio_state == RADIO_UNINIT)
		radio_init();

	mp_buffer_info_t buf;
	mp_get_buffer_raise(buf_obj, &buf, MP_BUFFER_READ);
	const unsigned len = buf.len;

	if (len > MAC_PACKET_MAX_LENGTH - 2)
		mp_raise_ValueError("tx length too long");

	int rc = radio_tx_buffer_send(buf.buf, len);
	if (rc != 0)
		mp_raise_ValueError("tx failed");

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(radio_txbytes_obj, radio_txbytes);


static mp_obj_t radio_mac(void)
{
	if (radio_state == RADIO_UNINIT)
		radio_init();

	return mp_obj_new_bytes(radio_mac_address, sizeof(radio_mac_address));
}

MP_DEFINE_CONST_FUN_OBJ_0(radio_mac_obj, radio_mac);


static mp_obj_t mp_radio_promiscuous(size_t n_args, const mp_obj_t *args)
{
	if (radio_state == RADIO_UNINIT)
		radio_init();

	if (n_args == 1)
	{
		radio_promiscuous = mp_obj_int_get_checked(args[0]);
		RAIL_IEEE802154_SetPromiscuousMode(rail, radio_promiscuous);
		//printf("radio: %s promiscuous mode\n", radio_promiscuous ? "enabling" : "disabling");
	}

	return MP_OBJ_NEW_SMALL_INT(radio_promiscuous);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(radio_promiscuous_obj, 0, 1, mp_radio_promiscuous);


static mp_obj_t mp_radio_short_address(size_t n_args, const mp_obj_t *args)
{
	if (radio_state == RADIO_UNINIT)
		radio_init();

	if (n_args == 1)
	{
 		radio_short_address = mp_obj_int_get_checked(args[0]);
		RAIL_IEEE802154_SetShortAddress(rail, radio_short_address, 0);
		//printf("radio: addr %04x\n", radio_short_address);
	}

	if (radio_short_address == 0xFFFF)
		return mp_const_none;

	return MP_OBJ_NEW_SMALL_INT(radio_short_address);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(radio_short_address_obj, 0, 1, mp_radio_short_address);

static mp_obj_t mp_radio_pan_id(size_t n_args, const mp_obj_t *args)
{
	if (radio_state == RADIO_UNINIT)
		radio_init();

	if (n_args == 1)
	{
		radio_pan_id = mp_obj_int_get_checked(args[0]);
		RAIL_IEEE802154_SetPanId(rail, radio_pan_id, 0);
		//printf("radio: pan %04x\n", radio_pan_id);
	}

	if (radio_pan_id == 0xFFFF)
		return mp_const_none;

	return MP_OBJ_NEW_SMALL_INT(radio_pan_id);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(radio_pan_id_obj, 0, 1, mp_radio_pan_id);


static mp_obj_t mp_radio_channel(mp_obj_t channel_obj)
{
	if (radio_state == RADIO_UNINIT)
		radio_init();

	unsigned channel = mp_obj_int_get_checked(channel_obj);
	radio_channel_set(channel);

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(radio_channel_obj, mp_radio_channel);


STATIC const mp_map_elem_t radio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_radio) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t) &radio_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_promiscuous), (mp_obj_t) &radio_promiscuous_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_address), (mp_obj_t) &radio_short_address_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_pan), (mp_obj_t) &radio_pan_id_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_channel), (mp_obj_t) &radio_channel_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mac), (mp_obj_t) &radio_mac_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_rx), (mp_obj_t) &radio_rxbytes_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_tx), (mp_obj_t) &radio_txbytes_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_radio_globals,
    radio_globals_table
);

const mp_obj_module_t mp_module_radio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_radio_globals,
};

MP_REGISTER_MODULE(MP_QSTR_Radio, mp_module_radio, 1);
