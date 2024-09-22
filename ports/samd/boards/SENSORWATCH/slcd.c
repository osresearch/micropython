/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/smallint.h"

#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>
#include <hal_gpio.h>
#include <hpl_slcd.h>
#include <hal_slcd_sync.h>

// TODO: support other boards with different LCD pins
// test board is A1-05

#define GPIO_PIN_FUNCTION_B 1
#define SLCD0 GPIO(GPIO_PORTB, 6)
#define SLCD1 GPIO(GPIO_PORTB, 7)
#define SLCD2 GPIO(GPIO_PORTB, 8)
#define SLCD3 GPIO(GPIO_PORTB, 9)
#define SLCD4 GPIO(GPIO_PORTA, 4)
#define SLCD5 GPIO(GPIO_PORTA, 5)
#define SLCD6 GPIO(GPIO_PORTA, 6)
#define SLCD7 GPIO(GPIO_PORTA, 7)
#define SLCD8 GPIO(GPIO_PORTA, 8)
#define SLCD9 GPIO(GPIO_PORTA, 9)
#define SLCD10 GPIO(GPIO_PORTA, 10)
#define SLCD11 GPIO(GPIO_PORTA, 11)
#define SLCD12 GPIO(GPIO_PORTB, 11)
#define SLCD13 GPIO(GPIO_PORTB, 12)
#define SLCD14 GPIO(GPIO_PORTB, 13)
#define SLCD15 GPIO(GPIO_PORTB, 14)
#define SLCD16 GPIO(GPIO_PORTB, 15)
#define SLCD17 GPIO(GPIO_PORTA, 12)
#define SLCD18 GPIO(GPIO_PORTA, 13)
#define SLCD19 GPIO(GPIO_PORTA, 14)
#define SLCD20 GPIO(GPIO_PORTA, 15)
#define SLCD21 GPIO(GPIO_PORTA, 16)
#define SLCD22 GPIO(GPIO_PORTA, 17)
#define SLCD23 GPIO(GPIO_PORTA, 18)
#define SLCD24 GPIO(GPIO_PORTA, 19)
#define SLCD25 GPIO(GPIO_PORTB, 16)
#define SLCD26 GPIO(GPIO_PORTB, 17)


static struct slcd_sync_descriptor SEGMENT_LCD_0;

static void slcd_port_init(void) {
	gpio_set_pin_function(SLCD0, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD1, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD2, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD3, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD4, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD5, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD6, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD7, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD8, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD9, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD10, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD11, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD12, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD13, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD14, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD15, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD16, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD17, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD18, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD19, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD20, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD21, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD22, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD23, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD24, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD25, GPIO_PIN_FUNCTION_B);
	gpio_set_pin_function(SLCD26, GPIO_PIN_FUNCTION_B);
}


/**
 * \brief SLCD initialization function
 *
 * Enables SLCD peripheral, clocks and initializes SLCD driver
 */
static mp_obj_t slcd_init(void) {
    hri_mclk_set_APBCMASK_SLCD_bit(SLCD);
    slcd_sync_init(&SEGMENT_LCD_0, SLCD);
    slcd_port_init();
    slcd_sync_enable(&SEGMENT_LCD_0);
    return mp_const_true;
}

void slcd_deinit(void)
{
    slcd_sync_deinit(&SEGMENT_LCD_0);
    hri_mclk_clear_APBCMASK_SLCD_bit(SLCD);
}

static mp_obj_t slcd_clear(void) {
    SLCD->SDATAL0.reg = 0;
    SLCD->SDATAL1.reg = 0;
    SLCD->SDATAL2.reg = 0;
    return mp_const_true;
}

static mp_obj_t slcd_on(mp_obj_t com_obj, mp_obj_t seg_obj) {
    uint8_t com = mp_obj_get_int(com_obj);
    uint8_t seg = mp_obj_get_int(seg_obj);
	// todo test against CONF_SLCD_COM_NUM?
    if (com < 0 || seg < 0)
        return mp_const_false;
    slcd_sync_seg_on(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
    return mp_const_true;
}

static mp_obj_t slcd_off(mp_obj_t com_obj, mp_obj_t seg_obj) {
    uint8_t com = mp_obj_get_int(com_obj);
    uint8_t seg = mp_obj_get_int(seg_obj);
    if (com < 0 || seg < 0)
        return mp_const_false;
    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
    return mp_const_true;
}

// the com and segment are packed into a single 8-bit value,
// with 2 bits for the com and 6 bits for the segment.
static mp_obj_t slcd_set(mp_obj_t packed_obj, mp_obj_t value_obj) {
    uint8_t packed = mp_obj_get_int(packed_obj);
    uint8_t value = mp_obj_get_int(value_obj);
    uint8_t com = packed >> 6;
    uint8_t seg = packed & 0x3F;
    if (com > 2 || seg > 24)
        return mp_const_false;

    if (value)
	    slcd_sync_seg_on(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
    else
	    slcd_sync_seg_off(&SEGMENT_LCD_0, SLCD_SEGID(com, seg));
    return mp_const_true;
}

static MP_DEFINE_CONST_FUN_OBJ_0(slcd_init_obj, slcd_init);
static MP_DEFINE_CONST_FUN_OBJ_0(slcd_clear_obj, slcd_clear);
static MP_DEFINE_CONST_FUN_OBJ_2(slcd_on_obj, slcd_on);
static MP_DEFINE_CONST_FUN_OBJ_2(slcd_off_obj, slcd_off);
static MP_DEFINE_CONST_FUN_OBJ_2(slcd_set_obj, slcd_set);

static const mp_rom_map_elem_t mp_module_slcd_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_slcd) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&slcd_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&slcd_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&slcd_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&slcd_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&slcd_set_obj) },
};

static MP_DEFINE_CONST_DICT(mp_module_slcd_globals, mp_module_slcd_globals_table);

const mp_obj_module_t mp_module_slcd = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_slcd_globals,
};

MP_REGISTER_MODULE(MP_QSTR_slcd, mp_module_slcd);

