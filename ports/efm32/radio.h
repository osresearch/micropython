#ifndef _radio_h_
#define _radio_h_

/* 802.15.4 maximum size of a single packet including PHY byte is 128 bytes
 * but we don't support that */
#define MAC_PACKET_MAX_LENGTH   112
/* Offsets of prepended data in packet buffer */
#define MAC_PACKET_OFFSET_RSSI  0
#define MAC_PACKET_OFFSET_LQI   1
/* This driver prepends RSSI and LQI */
#define MAC_PACKET_INFO_LENGTH  2

extern uint8_t radio_mac_address[8];
extern uint8_t radio_tx_buffer[MAC_PACKET_MAX_LENGTH + 1];
extern volatile int radio_tx_pending;

extern int radio_send_tx_buffer(void);

#endif
