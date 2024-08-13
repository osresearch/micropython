/**
 * \file
 *
 * \brief Linker script for running in internal FLASH on the SAML22J18A
 *
 * Copyright (c) 2018 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */


/* Memory Space Definitions:
 *  0x00000000-0x00002000: Bootloader       (length  0x2000 or 8192 bytes)
 *  0x00002000-0x0003C000: Firmware         (length 0x3A000 or 237568 bytes)
 *  0x0003C000-0x00040000: EEPROM Emulation (length  0x2000 or 8192 bytes)
 *  0x20000000-0x20008000: RAM              (length  0x8000 or 32768 bytes)
 */

_flashsize = 256K;  /* The physical flash size */
_bootloader = 8K;   /* Must match the ORIGIN value of FLASH */


MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00002000, LENGTH = _codesize
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 32K
}

/* Top end of the stack, with room for double-tap variable */
_estack = ORIGIN(RAM) + LENGTH(RAM) - 8;
_sstack = _estack - 8K;

_oflash_fs = ORIGIN(FLASH) + _codesize;
_sflash_fs = _flashsize - _codesize - _bootloader;

_sheap = _ebss;
_eheap = _sstack;

