/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
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
#ifndef MICROPY_INCLUDED_EXTMOD_MACHINE_SPIFLASH_H
#define MICROPY_INCLUDED_EXTMOD_MACHINE_SPIFLASH_H

#include "py/obj.h"
#include "py/mphal.h"
#include "drivers/memory/spiflash.h"

typedef struct _mp_machine_spiflash_obj_t {
    mp_obj_base_t base;
    mp_obj_base_t * spi;
    mp_spiflash_config_t spi_flash_config;
    //mp_spiflash_cache_t spi_flash_cache;
    mp_spiflash_t spi_flash;
} mp_machine_spiflash_obj_t;

extern const mp_obj_type_t mp_machine_spiflash_type;

#endif // MICROPY_INCLUDED_EXTMOD_MACHINE_SPI_H
