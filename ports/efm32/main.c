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

#undef MICROPY_EMBER

#if MICROPY_ENABLE_COMPILER
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
#endif

#ifdef MICROPY_EMBER
/*
 * EFM32 port that co-exists with Ember.
 * We don't have the source to Ember, so this is patched into the existing
 * image and there are fixups for the real calls.  It is a bit of a mess...
 *
 * Complications:
 * - Addresses are all wrong; we have to fix them up
 * - Our BSS does not play well with their BSS.
 * - We should have no RW data since we don't have a data segment.
 * - Our Heap (and BSS) must be allocated with their malloc.
 * - We could run as a task, or we can run as a hook in their mainloop.
 *
 * We have an agreed upon location to stash our heap and bss
 */
extern void mp_hal_stdout_init(void);
extern void mp_hal_stdout_tx_str(const char * c, unsigned len);
extern int mp_hal_stdin_rx_chr_nonblocking(void);
static char * stack_top;
#if MICROPY_ENABLE_GC
static char heap[2048];
#endif

void micropython_setup(void)
{
    mp_hal_stdout_init();
    mp_hal_stdout_tx_str("uPy!", 4);

    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif
    mp_init();

    //pyexec_event_repl_init();
}

void micropython_loop(void)
{
    pyexec_frozen_module("loop.py");
/*
    int c = mp_hal_stdin_rx_chr_nonblocking();
    if (c < 0)
        return;

    if (pyexec_event_repl_process_char(c) == 0)
        return;

    // should never each here
    mp_deinit();
*/
}

// Pointers to the setup and loop function are stored
// at a fixed address, controlled by the linker script.
uint32_t setup_func
__attribute__((__section__(".setup")))
= 1 + (uint32_t) micropython_setup;

uint32_t loop_func
__attribute__((__section__(".loop")))
= 1 + (uint32_t) micropython_loop;

#if 0
// Fixup the real stack pointer so that we have room for
// our heap and bss.
const uint32_t new_ikea_stack __attribute__((__section__(".stackpointer"))) = 0x200041c0 - 2500;

/* hack to include the Ikea firmware */
__asm__ (
".section .firmware\n"
".incbin \"led10w-partial.bin\"\n"
".org 0x4000\n"
".data 0xbeef\n"
);
#endif


#else
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


    int stack_dummy;
    stack_top = (char*)&stack_dummy;


    extern void mp_hal_stdout_init(void);
    extern char mp_hal_stdin_rx_chr(void);
    mp_hal_stdout_init();

extern void radio_init(void);
radio_init();
    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif
    mp_init();
    #if MICROPY_ENABLE_COMPILER
    #if MICROPY_REPL_EVENT_DRIVEN
    pyexec_event_repl_init();
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (pyexec_event_repl_process_char(c)) {
            break;
        }
    }
    #else
    pyexec_frozen_module("frozentest.py");
    pyexec_friendly_repl();
    #endif
    //do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
    //do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);
    #else
    pyexec_frozen_module("frozentest.py");
    #endif
    mp_deinit();
    return 0;
}

#ifdef MICROPY_MIN_USE_CORTEX_CPU
void _start(void)
{
	main(0, NULL);
}
#endif

#endif

void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    //gc_dump_info();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

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
 */
void HardFault_Handler   (void) __attribute__((__naked__));
void HardFault_Handler   (void)
{
	printf("%s\n", __func__);
	while(1);
}
