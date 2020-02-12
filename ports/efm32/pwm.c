#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "extmod/machine_pin.h"
#include "em_timer.h"

void mp_hal_pwm_init(void)
{
	static bool init_done;
	if (init_done)
		return;
	init_done = 1;

	// Set Top Value to 
	TIMER_TopSet(TIMER1, MP_HAL_PWM_TOP);

	// Create a timerInit object and set prescaler
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	timerInit.prescale = timerPrescale128;
 
	TIMER_Init(TIMER1, &timerInit);
}


int mp_hal_pwm_freq_get(mp_hal_pwm_obj_t pwm)
{
	return -1;
}

void mp_hal_pwm_freq(mp_hal_pwm_obj_t pwm, int freq)
{
	mp_raise_ValueError("PWM.Freq() not supported");
}

int mp_hal_pwm_duty_get(mp_hal_pwm_obj_t pwm)
{
	if (pwm->pin->pwm_config == 0xFF)
		mp_raise_ValueError("PWM not supported");

	const unsigned channel = (pwm->pin->pwm_config >> 4) & 0xF;
	//const unsigned location = (pin->pwm_config >> 0) & 0xF;
	return TIMER1->CC[channel].CCV;
}

void mp_hal_pwm_duty(mp_hal_pwm_obj_t pwm, int duty)
{
	const uint8_t pwm_config = pwm->pin->pwm_config;
	if (pwm_config == 0xFF)
		mp_raise_ValueError("PWM not supported");

	const unsigned channel = (pwm_config >> 4) & 0xF;
	const unsigned location = (pwm_config >> 0) & 0xF;

	if (!pwm->active)
	{
		pwm->active = 1;

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
	}
 
	TIMER_CompareBufSet(TIMER1, channel, duty);
}

void mp_hal_pwm_deinit(mp_hal_pwm_obj_t pwm)
{
	if (!pwm->active)
		return;
	pwm->active = 0;

	const uint8_t pwm_config = pwm->pin->pwm_config;
	if (pwm_config == 0xFF)
		return;

	const unsigned channel = (pwm_config >> 4) & 0xF;
	//const unsigned location = (pwm_config >> 0) & 0xF;

	// Reset the timer count control object initializer to default
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
	TIMER_InitCC(TIMER1, channel, &timerCCInit);
	 
	/* Disable route CC1 to location and enable pin;
	 * values from efm32/gecko_sdk/include/efr32mg1p_timer.h
	 */
	TIMER1->ROUTEPEN |= (0x0UL << channel);
}
