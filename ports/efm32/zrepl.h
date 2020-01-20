#ifndef _zrepl_h_
#define _zrepl_h_

extern int zrepl_active;

// hook the uart print function and send 802.15.4 messages
extern void zrepl_send(const char * str, size_t len);

// forwards characters from the 802.15.4 message into the uart recv function
extern void zrepl_recv(const void * msg, size_t len);

#endif
