#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "extmod/machine_pin.h"
#include "em_gpio.h"

// On the 10W LED driver, PB13 is the pin that PWMs the single output channel
#define LED_PORT gpioPortB
#define LED_PIN 13

struct efm_pin_t {
	mp_obj_base_t base;
	unsigned port;
	unsigned pin;
	unsigned gpio_id;
};

static const struct efm_pin_t pins[] = {
	// left side going down
	{ {&machine_pin_type}, gpioPortA, 0,  0 },
	{ {&machine_pin_type}, gpioPortA, 1,  1 },
	{ {&machine_pin_type}, gpioPortB, 12, 2 },
	{ {&machine_pin_type}, gpioPortB, 13, 3 }, // LED PWM

	// right side going up
	{ {&machine_pin_type}, gpioPortB, 15, 4 },
	{ {&machine_pin_type}, gpioPortB, 14, 5 },
	{ {&machine_pin_type}, gpioPortC, 12, 6 }, // TX
	{ {&machine_pin_type}, gpioPortC, 11, 7 }, // RX
	{ {&machine_pin_type}, gpioPortF, 0,  8 }, // SWCLK
	{ {&machine_pin_type}, gpioPortF, 1,  9 }, // SWD
	{ {&machine_pin_type}, gpioPortF, 2,  10 }, // ?
	{ {&machine_pin_type}, gpioPortF, 3,  11 }, // ?

	// internal connections
	{ {&machine_pin_type}, gpioPortB, 11,  12 }, // spi cs
	{ {&machine_pin_type}, gpioPortD, 13,  13 }, // spi sck
	{ {&machine_pin_type}, gpioPortD, 14,  14 }, // spi miso
	{ {&machine_pin_type}, gpioPortD, 15,  15 }, // spi mosi
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
