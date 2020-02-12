#ifndef _efm32_mphalport_h_
#define _efm32_mphalport_h_

#include <py/obj.h>
#include "mpconfigport.h"

#define mp_hal_delay_us_fast mp_hal_delay_us

void mp_hal_set_interrupt_char(char c);

// gpio functions
struct _mp_hal_pin_t {
	mp_obj_base_t base;
	unsigned port;
	unsigned pin;
	uint8_t gpio_id;
	uint8_t pwm_config; // ff == not used
};

extern mp_hal_pin_obj_t mp_hal_pin_lookup(unsigned pin_id);
extern unsigned mp_hal_pin_id(mp_hal_pin_obj_t pin);
extern void mp_hal_pin_input(mp_hal_pin_obj_t pin);
extern void mp_hal_pin_output(mp_hal_pin_obj_t pin);
extern void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin);
extern int mp_hal_pin_read(mp_hal_pin_obj_t pin);
extern void mp_hal_pin_write(mp_hal_pin_obj_t pin, int value);


// pwm functions
struct _mp_hal_pwm_t {
	mp_obj_base_t base; // required
	mp_hal_pin_obj_t pin; // required
	uint8_t active; // required
};

//typedef mp_hal_pwm_t * mp_hal_pwm_obj_t;
//typedef mp_hal_pin_t * mp_hal_pin_obj_t;

extern void mp_hal_pwm_init(void);
extern int mp_hal_pwm_freq_get(mp_hal_pwm_obj_t pwm);
extern void mp_hal_pwm_freq(mp_hal_pwm_obj_t pwm, int freq);
extern int mp_hal_pwm_duty_get(mp_hal_pwm_obj_t pwm);
extern void mp_hal_pwm_duty(mp_hal_pwm_obj_t pwm, int duty);
extern void mp_hal_pwm_deinit(mp_hal_pwm_obj_t pwm);

#endif
