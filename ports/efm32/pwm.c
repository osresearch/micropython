#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "extmod/machine_pin.h"
#include "em_timer.h"

#define TIMER_TOP 8192

static mp_obj_t timer_duty(mp_obj_t duty_obj)
{
	const int channel = 1;
	int duty = mp_obj_int_get_checked(duty_obj);

	if (duty < 0)
		duty = 0;
	else
	if (duty >= TIMER_TOP)
		duty = TIMER_TOP-1;

	TIMER_CompareBufSet(TIMER1, channel, duty);

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(timer_duty_obj, timer_duty);


// see pin mapping markdown table
static mp_obj_t timer_location(mp_obj_t location_obj)
{
	const unsigned location = mp_obj_int_get_checked(location_obj);
	if (location < 0 || location > 15)
		mp_raise_ValueError("Timer location out of range");

	const int channel = 1;

	// Create the timer count control object initializer
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
	timerCCInit.mode = timerCCModePWM;
	timerCCInit.cmoa = timerOutputActionToggle;
 
	// Configure CC channel
	TIMER_InitCC(TIMER1, channel, &timerCCInit);
 
	/* Route CC1 to location and enable pin;
  	 * values from efm32/gecko_sdk/include/efr32mg1p_timer.h
	 * #define TIMER_ROUTEPEN_CC0PEN (0x1UL << 0)
	 */
	TIMER1->ROUTEPEN |= (0x1UL << channel);
	TIMER1->ROUTELOC0 |= (location << (8*channel));
 
	// Set Top Value to zero for initial
	TIMER_TopSet(TIMER1, TIMER_TOP);
 
	// Leave it off for now
	TIMER_CompareBufSet(TIMER1, channel, 0);
 
	// Create a timerInit object, based on the API default
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	timerInit.prescale = timerPrescale128;
 
	TIMER_Init(TIMER1, &timerInit);

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(timer_location_obj, timer_location);


STATIC const mp_map_elem_t timer_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_timer) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_duty), (mp_obj_t) &timer_duty_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_location), (mp_obj_t) &timer_location_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_timer_globals,
    timer_globals_table
);

const mp_obj_module_t mp_module_timer_obj = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_timer_globals,
};
