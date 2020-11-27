/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Portions copyright (c) 2013-2015 Damien P. George
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
#include <string.h>

#include "modmachine.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "py/objstr.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/stream.h"
#include "extmod/machine_mem.h"
//#include "extmod/machine_pin.h"
//#include "extmod/machine_pwm.h"
#include "machine_spiflash.h"

#include "Fw_global_config.h"
#include "eoss3_dev.h"

//#include "zrepl.h"
//#include "em_core.h"


// need to implement these
#if 0
// Returns a string of 12 bytes (96 bits), which is the unique ID for the MCU.
STATIC mp_obj_t machine_unique_id(void) {
    byte *id = (byte*)MP_HAL_UNIQUE_ID_ADDRESS;
    return mp_obj_new_bytes(id, 12);
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_unique_id_obj, machine_unique_id);


STATIC mp_obj_t machine_lightsleep(size_t n_args, const mp_obj_t *args) {
    if (n_args != 0) {
        mp_obj_t args2[2] = {MP_OBJ_NULL, args[0]};
        pyb_rtc_wakeup(2, args2);
    }
    powerctrl_enter_stop_mode();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_lightsleep_obj, 0, 1, machine_lightsleep);

STATIC mp_obj_t machine_deepsleep(size_t n_args, const mp_obj_t *args) {
    if (n_args != 0) {
        mp_obj_t args2[2] = {MP_OBJ_NULL, args[0]};
        pyb_rtc_wakeup(2, args2);
    }
    powerctrl_enter_standby_mode();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_deepsleep_obj, 0, 1, machine_deepsleep);
#endif

static mp_obj_t
machine_reset(void) {
	NVIC_SystemReset();
	return mp_const_none; // better not reach here!
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

static mp_obj_t
machine_bootloader(void) {
	PMU->MISC_POR_3 = REBOOT_CAUSE_FLASHING;
	NVIC_SystemReset();
	return mp_const_none; // better not reach here!
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_bootloader_obj, machine_bootloader);

extern const mp_obj_type_t mp_machine_eoss3_spi_type;

#if 0
extern const mp_obj_module_t mp_module_crypto;

static mp_obj_t
machine_stdio_poll(void)
{
        if (mp_hal_stdio_poll(MP_STREAM_POLL_RD))
		return mp_const_true;
	else
		return mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_stdio_poll_obj, machine_stdio_poll);

static mp_obj_t
machine_zrepl(mp_obj_t active_obj)
{
	int active = mp_obj_int_get_checked(active_obj);
	int old = zrepl_active;
	zrepl_active = active;
	if (old)
		return mp_const_true;
	else
		return mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_zrepl_obj, machine_zrepl);
#endif

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_umachine) },
    { MP_ROM_QSTR(MP_QSTR_reset),               MP_ROM_PTR(&machine_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_bootloader),          MP_ROM_PTR(&machine_bootloader_obj) },

/*
    { MP_ROM_QSTR(MP_QSTR_unique_id),           MP_ROM_PTR(&machine_unique_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_soft_reset),          MP_ROM_PTR(&machine_soft_reset_obj) },
*/
    { MP_ROM_QSTR(MP_QSTR_mem8),                MP_ROM_PTR(&machine_mem8_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem16),               MP_ROM_PTR(&machine_mem16_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem32),               MP_ROM_PTR(&machine_mem32_obj) },

    { MP_ROM_QSTR(MP_QSTR_SPIFlash),            MP_ROM_PTR(&mp_machine_spiflash_type) },

#if 0
    { MP_ROM_QSTR(MP_QSTR_Crypto),              MP_ROM_PTR(&mp_module_crypto) },
    { MP_ROM_QSTR(MP_QSTR_Pin),                 MP_ROM_PTR(&machine_pin_type) },
    { MP_ROM_QSTR(MP_QSTR_PWM),                 MP_ROM_PTR(&machine_pwm_type) },
    { MP_ROM_QSTR(MP_QSTR_SPI),                 MP_ROM_PTR(&mp_machine_soft_spi_type) },
    { MP_ROM_QSTR(MP_QSTR_stdio_poll),          MP_ROM_PTR(&machine_stdio_poll_obj) },
    { MP_ROM_QSTR(MP_QSTR_zrepl),               MP_ROM_PTR(&machine_zrepl_obj) },
#endif
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t machine_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};

