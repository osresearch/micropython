#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "em_gpio.h"
#include "em_cmu.h"

#include "rail/rail.h"
#include "rail/pa.h"
#include "rail/ieee802154/rail_ieee802154.h"

/* 802.15.4 maximum size of a single packet including PHY byte is 128 bytes */
#define MAC_PACKET_MAX_LENGTH   128
/* Offsets of prepended data in packet buffer */
#define MAC_PACKET_OFFSET_RSSI  0
#define MAC_PACKET_OFFSET_LQI   1
/* This driver prepends RSSI and LQI */
#define MAC_PACKET_INFO_LENGTH  2


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

static const RAIL_IEEE802154_Config_t ieee802154_config = {
	.promiscuousMode	= true,
	.isPanCoordinator	= false,
	.framesMask		= RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES,
	.ackConfig = {
		.enable			= true,
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

static uint8_t MAC_address[8];

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
}

static volatile int rx_buffer_valid;
static uint8_t rx_buffer[MAC_PACKET_MAX_LENGTH + MAC_PACKET_INFO_LENGTH];

static void process_packet(RAIL_Handle_t rail)
{
	RAIL_RxPacketInfo_t info;
	RAIL_RxPacketHandle_t handle = RAIL_GetRxPacketInfo(rail, RAIL_RX_PACKET_HANDLE_NEWEST, &info);
	if (info.packetStatus != RAIL_RX_PACKET_READY_SUCCESS)
		return;

	// deal with acks early
	uint8_t header[4];
	RAIL_PeekRxPacket(rail, handle, header, 4, 0);
/*
	if (header[0] == 5
	&&  header[3] == current_tx_sequence
	&& waiting_for_ack)
	{
		RAIL_CancelAutoAck(rail);
		waiting_for_ack = false;
		last_ack_pending_bit = (header[1] & (1 << 4)) != 0;
		printf("got ack\n");
	} else {
*/
 {
		RAIL_RxPacketDetails_t details;
		details.timeReceived.timePosition = RAIL_PACKET_TIME_DEFAULT;
		details.timeReceived.totalPacketBytes = 0;
		RAIL_GetRxPacketDetails(rail, handle, &details);
		RAIL_CopyRxPacket(rx_buffer, &info);
		rx_buffer_valid = 1;

		// cancel the ACK if the sender did not request one
		// buffer[0] == length
		// buffer[1] == frame_type[0:2], security[3], frame_pending[4], ack_req[5], intrapan[6]
		// buffer[2] == destmode[2:3], version[4:5], srcmode[6:7]
		if ((rx_buffer[1] & (1 << 5)) == 0)
			RAIL_CancelAutoAck(rail);

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

	RAIL_ReleaseRxPacket(rail, handle);
}


/*
 * Callbank from the radio interrupt when there is an event.
 * rx 00000000:00018030
 */

static void rail_callback_events(RAIL_Handle_t rail, RAIL_Events_t events)
{
/*
	// ignore certain bit patterns unless other things are set
	events &= ~( 0
		| RAIL_EVENT_RX_PREAMBLE_LOST
		| RAIL_EVENT_RX_PREAMBLE_DETECT
		| RAIL_EVENT_RX_TIMING_LOST
		| RAIL_EVENT_RX_TIMING_DETECT
	);

	if(events == 0)
		return;

	printf("rx %08x:%08x\n", (unsigned int)(events >> 32), (unsigned int)(events >> 0));
*/

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

	if (events & RAIL_EVENT_TX_PACKET_SENT)
	{
		// they are done with our packet, maybe signal something?
	}

	if (events & RAIL_EVENT_CAL_NEEDED)
	{
		// we should flag that a calibration is needed
	}

	// lots of other events that we don't handle
}

#if 0
/*
 * This will be called by the radio library when a data request is received.
 */
void
RAILCb_IEEE802154_DataRequestCommand(
	RAIL_IEEE802154_Address_t *data
)
{
  // Placeholder validation for when a data request should have the frame
  // pending bit set in the ACK.
  if (data->length == RAIL_IEEE802154_LongAddress)
  {
    printf("%d bytes from %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
	data->length,
	data->longAddress[0],
	data->longAddress[1],
	data->longAddress[2],
	data->longAddress[3],
	data->longAddress[4],
	data->longAddress[5],
	data->longAddress[6],
	data->longAddress[7]
    );
    if (data->longAddress[0] == 0xAA)
      RAIL_IEEE802154_SetFramePending();
  } else {
    if ((data->shortAddress & 0xFF) == 0xAA)
    printf("%d bytes from %04x\n",
	data->length,
	data->shortAddress);
      RAIL_IEEE802154_SetFramePending();
  }
}


#endif

static mp_obj_t radio_rxbytes_get(void) // mp_obj_t arg)
{
	if (!rx_buffer_valid)
		return mp_const_none;
	rx_buffer_valid = 0;

	return mp_obj_new_bytearray(rx_buffer[0], rx_buffer+1);
}

MP_DEFINE_CONST_FUN_OBJ_0(radio_rxbytes_obj, radio_rxbytes_get);

STATIC const mp_map_elem_t radio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_radio) },
    //{ MP_OBJ_NEW_QSTR(MP_QSTR_set), (mp_obj_t) &gpio_set_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get), (mp_obj_t) &radio_rxbytes_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_radio_globals,
    radio_globals_table
);

const mp_obj_module_t mp_module_radio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_radio_globals,
};


int radio_init(void)
{
	printf("%s: mac %08x:%08x\n", __func__, (unsigned int) DEVINFO->UNIQUEH, (unsigned int) DEVINFO->UNIQUEL);

	rail = RAIL_Init(&rail_config, rail_callback_rfready);
	RAIL_ConfigCal(rail, RAIL_CAL_ALL);
	RAIL_IEEE802154_Config2p4GHzRadio(rail);
	RAIL_IEEE802154_Init(rail, &ieee802154_config);
	RAIL_ConfigEvents(rail, RAIL_EVENTS_ALL, 0
		| RAIL_EVENT_RSSI_AVERAGE_DONE
		| RAIL_EVENT_RX_ACK_TIMEOUT
		| RAIL_EVENT_RX_PACKET_RECEIVED
		| RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND
		| RAIL_EVENT_TX_PACKET_SENT
		| RAIL_EVENT_CAL_NEEDED
	);

	RAIL_ConfigTxPower(rail, &paInit2p4);
	RAIL_SetTxPower(rail, 255); // max

	// use the device unique id as the mac for network index 0
	memcpy(&MAC_address[0], (const void*)&DEVINFO->UNIQUEH, 4);
	memcpy(&MAC_address[4], (const void*)&DEVINFO->UNIQUEL, 4);
	RAIL_IEEE802154_SetLongAddress(rail, MAC_address, 0);

	// start the radio
	RAIL_Idle(rail, RAIL_IDLE_FORCE_SHUTDOWN_CLEAR_FLAGS, true);
	int channel = 11;
	radio_state = RADIO_RX;
	RAIL_StartRx(rail, channel, NULL);

/*
	bool status;

	status = RAIL_IEEE802154_SetPanId(nodeAddress.panId);
	status = RAIL_IEEE802154_SetShortAddress(nodeAddress.shortAddr);
*/
	return 0;
}

