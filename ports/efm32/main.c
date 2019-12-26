/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 * Copyright (c) 2019 Trammell Hudson
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"

void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}

/*
 * Stand-alone micropython
 */
static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[24000];
#endif

int main(int argc, char **argv)
{
	// must be called immediately in main() to handle errata
	CHIP_Init();

	// mbed-os does this?
	EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
	EMU_DCDCInit(&dcdcInit);

	// init the 38.4 MHz high frequency clock for the radio
	CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
	CMU_HFXOInit(&hfxoInit);

	// Switch HFCLK to HFXO and disable HFRCO
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
	//SystemHFXOClockSet(EFR32_HFXO_FREQ); //should already be at 38.4 MHz
	//CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
	CMU_ClockEnable(cmuClock_CORELE, true);

	/* Turn RTCC clock gate back on to keep RTC time correct */
	CMU_ClockEnable(cmuClock_RTCC, true);



    extern void mp_hal_stdout_init(void);
    extern char mp_hal_stdin_rx_chr(void);
    mp_hal_stdout_init();
    int stack_dummy;

soft_reset:
    stack_top = (char*)&stack_dummy;

    gc_init(heap, heap + sizeof(heap));
    mp_init();

    // run boot-up scripts
    pyexec_frozen_module("__init__.py");
    pyexec_file_if_exists("boot.py");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
        pyexec_file_if_exists("main.py");
    }

    for (;;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }

    gc_sweep_all();
    printf("MPY: soft reboot\n");
    mp_deinit();
    goto soft_reset;

    return 0;
}

void _start(void)
{
	main(0, NULL);
}


void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    //gc_dump_info();
}

void nlr_jump_fail(void *val) {
    while (1);
}

void NORETURN __fatal_error(const char *msg) {
    while (1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif


void NMI_Handler         (void) { printf("%s\n", __func__); while(1); }
void MemManage_Handler   (void) { printf("%s\n", __func__); while(1); }
void BusFault_Handler    (void) { printf("%s\n", __func__); while(1); }
void UsageFault_Handler  (void) { printf("%s\n", __func__); while(1); }
void SVC_Handler         (void) { printf("%s\n", __func__); while(1); }
void PendSV_Handler      (void) { printf("%s\n", __func__); while(1); }
void SysTick_Handler     (void) { printf("%s\n", __func__); while(1); }


/*
 * During a hard fault r0, r1, r2, r3, r12, lr, pc, psr are
 * pushed onto the stack. the other registers are preserved.
 *
 * Really should print the faulting PC
 */
void HardFault_Handler   (void) __attribute__((__naked__));
void HardFault_Handler   (void)
{
	printf("%s\n", __func__);
	while(1);
}
