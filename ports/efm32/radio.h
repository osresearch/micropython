#ifndef _radio_h_
#define _radio_h_

/* 802.15.4 maximum size of a single packet including PHY byte is 128 bytes
 * but we don't support that */
#define MAC_PACKET_MAX_LENGTH   128
/* Offsets of prepended data in packet buffer */
#define MAC_PACKET_OFFSET_RSSI  0
#define MAC_PACKET_OFFSET_LQI   1
/* This driver prepends RSSI and LQI */
#define MAC_PACKET_INFO_LENGTH  2

extern void radio_init(void);

extern uint8_t radio_mac_address[8];

extern int radio_tx_buffer_send(const void * buf, size_t len);

#endif
