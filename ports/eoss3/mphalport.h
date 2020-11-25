static inline mp_uint_t mp_hal_ticks_ms(void) {
    return 0;
}

static inline void mp_hal_set_interrupt_char(char c) {
}

// not yet implemented
static inline mp_hal_pin_obj_t mp_hal_get_pin_obj(void* x) {return 0;}
static inline void mp_hal_pin_output(mp_hal_pin_obj_t x) {}

extern void mp_hal_pin_write(mp_hal_pin_obj_t x, int value);
