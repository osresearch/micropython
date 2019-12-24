#ifndef _efm32_mphalport_h_
#define _efm32_mphalport_h_

static inline mp_uint_t mp_hal_ticks_ms(void) { return 0; }

// we need to figure out how to interrupt the REPL
static inline void mp_hal_set_interrupt_char(char c) { }

// gpio functions
extern mp_hal_pin_obj_t mp_hal_pin_lookup(unsigned pin_id);
extern unsigned mp_hal_pin_id(mp_hal_pin_obj_t pin);
extern void mp_hal_pin_input(mp_hal_pin_obj_t pin);
extern void mp_hal_pin_output(mp_hal_pin_obj_t pin);
extern void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin);
extern int mp_hal_pin_read(mp_hal_pin_obj_t pin);
extern void mp_hal_pin_write(mp_hal_pin_obj_t pin, int value);

#endif
