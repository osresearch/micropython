#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/mphal.h"
#include "mpconfigport.h"
#include "mphalport.h"
#include "extmod/machine_pin.h"
#include "em_gpio.h"

#define NO_PWM 0xFF
#define PWM(channel,location)	((channel) << 4 | (location))

static const struct _mp_hal_pin_t pins[] = {
// This is kind of nasty and should be done waaay saner, but for now.
#include "pin_defs.h"
};

#define NUM_PINS (sizeof(pins) / sizeof(*pins))

// gpioModeInput
// gpioModeInputPull (DOUT for direction)
// gpioModePushPull


mp_hal_pin_obj_t mp_hal_pin_lookup(unsigned pin_id)
{
	if (0 <= pin_id && pin_id < NUM_PINS)
		return &pins[pin_id];
	return NULL;
}

unsigned mp_hal_pin_id(mp_hal_pin_obj_t pin)
{
	return pin->gpio_id;
}

void mp_hal_pin_input(mp_hal_pin_obj_t pin)
{
	GPIO_PinModeSet(pin->port, pin->pin, gpioModeInput, 0);
}

void mp_hal_pin_output(mp_hal_pin_obj_t pin)
{
	GPIO_PinModeSet(pin->port, pin->pin, gpioModePushPull, 0);
}

void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin)
{
	GPIO_PinModeSet(pin->port, pin->pin, gpioModeInputPull, 1); // up?
}

int mp_hal_pin_read(mp_hal_pin_obj_t pin)
{
	return GPIO_PinInGet(pin->port, pin->pin);
}

void mp_hal_pin_write(mp_hal_pin_obj_t pin, int value)
{
	if (value)
		GPIO_PinOutSet(pin->port, pin->pin);
	else
		GPIO_PinOutClear(pin->port, pin->pin);
}
