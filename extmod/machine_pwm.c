/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Portions Copyright (c) 2016 Damien P. George
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
#include "py/nlr.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"
#include "extmod/machine_pin.h"

// Forward dec'l
extern const mp_obj_type_t machine_pwm_type;

/******************************************************************************/

// MicroPython bindings for PWM

static void
pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
	mp_hal_pwm_obj_t self = MP_OBJ_TO_PTR(self_in);
	mp_printf(print, "PWM(%u", mp_hal_pin_id(self->pin));
	if (self->active) {
		mp_printf(print, ", duty=%u", 
			mp_hal_pwm_duty_get(self));
	}
	mp_printf(print, ")");
}

static void
pwm_init_helper(
	mp_hal_pwm_obj_t self,
        size_t n_args,
	const mp_obj_t *pos_args,
	mp_map_t *kw_args
)
{
	enum { ARG_freq, ARG_duty };
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_freq, MP_ARG_INT, {.u_int = -1} },
		{ MP_QSTR_duty, MP_ARG_INT, {.u_int = -1} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, pos_args, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	// Maybe change PWM timer
	int tval = args[ARG_freq].u_int;
	if (tval != -1) {
		mp_hal_pwm_freq(self, tval);
	}

	// Set duty cycle?
	int dval = args[ARG_duty].u_int;
	if (dval != -1) {
		mp_hal_pwm_duty(self, dval);
	}
}

static mp_obj_t
pwm_make_new(
	const mp_obj_type_t *type,
	size_t n_args,
	size_t n_kw,
	const mp_obj_t *args
)
{
	mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
	mp_hal_pin_obj_t pin = MP_OBJ_TO_PTR(args[0]);

	// create PWM object from the given pin
	mp_hal_pwm_obj_t self = m_new_obj(struct _mp_hal_pwm_t);
	self->base.type = &machine_pwm_type;
	self->pin = pin;
	self->active = 0;

	// start the PWM subsystem if it's not already running
	mp_hal_pwm_init();

	// start the PWM running for this channel
	mp_map_t kw_args;
	mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	pwm_init_helper(self, n_args - 1, args + 1, &kw_args);

	return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t
pwm_init(
	size_t n_args,
        const mp_obj_t *args,
	mp_map_t *kw_args
)
{
	pwm_init_helper(args[0], n_args - 1, args + 1, kw_args);
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pwm_init_obj, 1, pwm_init);

static mp_obj_t
pwm_deinit(mp_obj_t self_in)
{
	mp_hal_pwm_obj_t self = MP_OBJ_TO_PTR(self_in);
	mp_hal_pwm_deinit(self);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pwm_deinit_obj, pwm_deinit);

static mp_obj_t pwm_freq(size_t n_args, const mp_obj_t *args) {
	mp_hal_pwm_obj_t self = MP_OBJ_TO_PTR(args[0]);

	if (n_args == 1)
		return MP_OBJ_NEW_SMALL_INT(mp_hal_pwm_freq_get(self));

	int freq = mp_obj_get_int(args[1]);
	mp_hal_pwm_freq(self, freq);
	return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pwm_freq_obj, 1, 2, pwm_freq);

static mp_obj_t pwm_duty(size_t n_args, const mp_obj_t *args)
{
	mp_hal_pwm_obj_t self = MP_OBJ_TO_PTR(args[0]);

	// get if no args
	if (n_args == 1)
		return MP_OBJ_NEW_SMALL_INT(mp_hal_pwm_duty_get(self));

	// set (and limit)
	int duty = mp_obj_get_int(args[1]);
	if (duty < 0)
		duty = 0;
	if (duty >= MP_HAL_PWM_TOP)
		duty = MP_HAL_PWM_TOP - 1;

	mp_hal_pwm_duty(self, duty);
	return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pwm_duty_obj,
    1, 2, pwm_duty);

static const mp_rom_map_elem_t pwm_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pwm_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pwm_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&pwm_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&pwm_duty_obj) },
};

static MP_DEFINE_CONST_DICT(pwm_locals_dict,
    pwm_locals_dict_table);

const mp_obj_type_t machine_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = pwm_print,
    .make_new = pwm_make_new,
    .locals_dict = (mp_obj_dict_t*)&pwm_locals_dict,
};
