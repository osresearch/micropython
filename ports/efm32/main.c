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
#include "em_usart.h"
#include "zrepl.h"
#include "radio.h"

extern uint8_t __StackTop;
extern uint8_t __HeapBase;
extern uint8_t __HeapLimit;

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

	/* Enable AES clock: otherwise result from AES ops is 0 */
	CMU_ClockEnable(cmuClock_CRYPTO, true);

	/* Enable clock for GPIO module and TIMER1 for PWM */
	CMU_ClockEnable(cmuClock_GPIO, true);
	CMU_ClockEnable(cmuClock_TIMER1, true );

	/* TODO: figure out if we can turn off clocks to save power */

	extern void mp_hal_stdout_init(void);
	extern char mp_hal_stdin_rx_chr(void);
	mp_hal_stdout_init();

	radio_init();
	zrepl_active = 1;

soft_reset:

    //gc_init(heap, heap + sizeof(heap));
    gc_init(&__HeapBase, &__HeapLimit);
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
    gc_collect_root(&dummy, ((mp_uint_t)&__StackTop - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
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

static void uart_str(const char * str)
{
	while(*str)
		USART_Tx(USART1, *str++);
}

static void uart_u32(const unsigned val)
{
	static const char hexdigit[] = "0123456789abcdef";
	USART_Tx(USART1, hexdigit[(val >> 28) & 0xF]);
	USART_Tx(USART1, hexdigit[(val >> 24) & 0xF]);
	USART_Tx(USART1, hexdigit[(val >> 20) & 0xF]);
	USART_Tx(USART1, hexdigit[(val >> 16) & 0xF]);
	USART_Tx(USART1, hexdigit[(val >> 12) & 0xF]);
	USART_Tx(USART1, hexdigit[(val >>  8) & 0xF]);
	USART_Tx(USART1, hexdigit[(val >>  4) & 0xF]);
	USART_Tx(USART1, hexdigit[(val >>  0) & 0xF]);
}


void NMI_Handler         (void) { uart_str(__func__); while(1); }
void MemManage_Handler   (void) { uart_str(__func__); while(1); }
void BusFault_Handler    (void) { uart_str(__func__); while(1); }
void UsageFault_Handler  (void) { uart_str(__func__); while(1); }
void SVC_Handler         (void) { uart_str(__func__); while(1); }
void PendSV_Handler      (void) { uart_str(__func__); while(1); }
void SysTick_Handler     (void) { uart_str(__func__); while(1); }


/*
 * During a hard fault r0, r1, r2, r3, r12, lr, pc, psr are
 * pushed onto the stack. the other registers are preserved.
 *
 * Really should print the faulting PC
 */
void HardFault_Handler   (void) __attribute__((__naked__));
void HardFault_Handler   (void)
{
	// read MMFAR in GDB: x/xw 0xE000ED34
	//const uint32_t fault_address = SCB->MMFAR;
	//const uint32_t fault_register = SCB->CFSR;

	uart_str("!!!!!\r\n");

	while(1)
	{
		// print it char by char
		uart_str(__func__);
		USART_Tx(USART1, ':');

		uart_u32(SCB->CFSR);
		if (SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk)
		{
			uart_str(" Bus=");
			uart_u32(SCB->BFAR);
		}

		if (SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk)
		{
			uart_str(" Mem=");
			uart_u32(SCB->MMFAR);
		}

		USART_Tx(USART1, '\r');
		USART_Tx(USART1, '\n');

		// should flash gpio0 LED or someting
		for(unsigned long i = 0 ; i < (1 << 28) ; i++)
			__asm__ __volatile__("nop");
	}
}
