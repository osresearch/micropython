/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Trammell Hudson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <math.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
//#include "drivers/bldc/bldc.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_gpio.h"

// Configure the pins and timer1 for complementary output.
// the micropython API doesn't set the bits exactly right.
// todo: file a bug to enable them
// Fast timer3 interrupt handler to update the three phase outputs
// on timer 1 channels 1, 2 and 3.

static int sin_table[256];
static unsigned counter;

STATIC mp_obj_t
bldc_init(mp_obj_t pwm_max_obj)
{
	const float pwm_max = mp_obj_get_float(pwm_max_obj);

	// initialize the sine table
	for(int t = 0 ; t < 256 ; t++)
		sin_table[t] = (int)(pwm_max * sinf(t * 2.0f * M_PI / 256.0f));

#if 0
	TIM_Base_InitTypeDef tim1_config = {
		.Prescaler	= 0,
		.Period		= 3360,
		.CounterMode	= TIM_COUNTERMODE_UP,
		.ClockDivision	= TIM_CLOCKDIVISION_DIV1,
		.RepetitionCounter = 0,
	};

	// configure timer 1
	__HAL_RCC_TIM1_CLK_ENABLE();
	NVIC_SetPriority(IRQn_NONNEG(self->irqn), IRQ_PRI_TIMX);
	NVIC_SetPriority(TIM1_CC_IRQn, IRQ_PRI_TIMX);

	TIM1->CR1 &= ~TIM_CR1_CEN; // disable

	TIM1->CR1 = TIM_CR1_URS; // only counter overflow generates update interruprs

	//TIM1->CR2 = 0x4011; // preloaded, waiting for commutation event, mms=1: enable counter, oc4 output idle state 1.
	//TIM1->CCER = 0x1444;	// output 4 enable, complementary outputs 1-3 enable
	// TIM_SetCCR4(&TIM1,0xa80);	// what is TIM1.CH4 used for?
	//TIM1->CCMR2 |= 0x7000; // enable output compare 4

	TIM1->CR2 = TIM_CR2_CCPC | TIM_CR2_MMS_0; // 0x0011; // preloaded, waiting for commutation event, mms=1: enable counter
	TIM1->CCER = TIM_CCER_CC1NE | TIM_CCER_CC2NE | TIM_CCER_CC3NE;	// complementary outputs 1-3 enable
	TIM1->EGR |= TIM_EGR_COMG; // allow update toe CCxE, CCxNE and OCxM bits
	TIM1->ARR = 0xd20; // TIM_SetAutoreload(&TIM1,0xd20); // 3360?
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	TIM1->CCR3 = 0;
	TIM1->BDTR = 0x46 | TIM_BDTR_OSSR | TIM_BDTR_BKE | TIM_BDTR_MOE; // 0x9846; // dead time 0x46, Main Output Enable, Break Enable, no locks
	//tim_dier_something(&TIM1,0x80,1); // enable break interrupt?

	TIM1->CR1 |= TIM_CR1_CEN; // re-enable it
	HAL_TIM_Base_Start(TIM1);

	// setup the 6 gpios for the alternate mode PWM outputs
	mp_hal_gpio_clock_enable(GPIOA);
	mp_hal_gpio_clock_enable(GPIOB);

	GPIO_InitTypeDef gpio = {
		.Mode		= GPIO_MODE_AF_PP,
		.Pull		= GPIO_NOPULL,
		.Speed		= GPIO_SPEED_FREQ_HIGH,
		.Alternate	= GPIO_AF1_TIM1,
	};

	gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
	HAL_GPIO_Init(GPIOA, &gpio);

	gpio.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
	HAL_GPIO_Init(GPIOB, &gpio);
#endif

	//TIM1->EGR |= TIM_EGR_COMG; // allow update toe CCxE, CCxNE and OCxM bits
	//TIM1->ARR = PWM_MAX;
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	TIM1->CCR3 = 0;
	TIM1->CCER = TIM_CCER_CC1NE | TIM_CCER_CC2NE | TIM_CCER_CC3NE;	// complementary outputs 1-3 enable
	return mp_const_none;
}

static mp_obj_t
bldc_step(mp_obj_t step_obj)
{
	const int step = mp_obj_get_int(step_obj);

	unsigned ccer = 0;
	int out;

	const unsigned t = (counter += step) & 0xFF;

	//TIM1->CR1 &= ~TIM_CR1_CEN; // turn off the timer while adjusting things

	out = sin_table[(t + 0) & 0xFF];
	if (out >= 0)
	{
		// activate high-side output
		TIM1->CCR1 = out;
		ccer |= TIM_CCER_CC1E;
	} else {
		// activate low-side output
		TIM1->CCR1 = -out;
		ccer |= TIM_CCER_CC1NE;
	}

	out = sin_table[(t + 85) & 0xFF];
	if (out >= 0)
	{
		// activate high-side output
		TIM1->CCR2 = out;
		ccer |= TIM_CCER_CC2E;
	} else {
		// activate low-side output
		TIM1->CCR2 = -out;
		ccer |= TIM_CCER_CC2NE;
	}

	out = sin_table[(t + 170) & 0xFF];
	if (out >= 0)
	{
		// activate high-side output
		TIM1->CCR3 = out;
		ccer |= TIM_CCER_CC3E;
	} else {
		// activate low-side output
		TIM1->CCR3 = -out;
		ccer |= TIM_CCER_CC3NE;
	}

	// update the capture/compare register to select the correct outputs
	TIM1->CCER = ccer;

	//TIM1->CR1 |= TIM_CR1_CEN; // re-enable it

	return MP_OBJ_NEW_SMALL_INT(t);
}

MP_DEFINE_CONST_FUN_OBJ_1(bldc_init_obj, bldc_init);
MP_DEFINE_CONST_FUN_OBJ_1(bldc_step_obj, bldc_step);
