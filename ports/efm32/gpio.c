#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "em_gpio.h"

// On the 10W LED driver, PB13 is the pin that PWMs the single output channel
#define LED_PORT gpioPortB
#define LED_PIN 13

static mp_obj_t gpio_set(mp_obj_t arg)
{
	static int mode_set;
	if (!mode_set)
	{
		mode_set = 1;
		GPIO_PinModeSet(LED_PORT, LED_PIN, gpioModePushPull, 0);
	}

	mp_int_t val = mp_obj_int_get_checked(arg);

	//printf("gpio called arg=%d\n", val);

	if (val)
		GPIO_PinOutSet(LED_PORT, LED_PIN);
	else
		GPIO_PinOutClear(LED_PORT, LED_PIN);
	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(gpio_set_obj, gpio_set);

STATIC const mp_map_elem_t gpio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gpio) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set), (mp_obj_t) &gpio_set_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_gpio_globals,
    gpio_globals_table
);

const mp_obj_module_t mp_module_gpio = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_gpio_globals,
};


