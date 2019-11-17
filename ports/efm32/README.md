# The minimal port for the Silcon Labs EFM32 

This port is intended to be a minimal MicroPython port that actually runs
on the Ikea Tradfri lighting hardware or other Silican Labs EFM32 boards.

# EFM32 features

Datasheet for the [EFR32MG1P132G1](https://www.silabs.com/documents/public/data-sheets/efr32mg1-datasheet.pdf)
used in the Ikea devices.  QFN48 package, 256 KB of internal flash, 32 KB of internal RAM, external SPI flash
for storing OTA images or other things.

* [Gecko SDK](https://github.com/SiliconLabs/Gecko_SDK)
* [SDK documentation](https://siliconlabs.github.io/Gecko_SDK_Doc/)


* It boots!
* It writes to the serial port!

## Not yet supported
* Zigbee
* Zigbee OTA
* Bluetooth LE
* Thread?
* External SPI flash
* Self programming
* PWM
* GPIO

# Building for an EFM32 MCU

The `Makefile` is setup to build for the EFR32MG1P cpu and enables a UART
for communication.  To build:

    $ make

This version of the build should work out-of-the-box on a Ikea 10w LED dimmer (and
anything similar), and will give you a MicroPython REPL on UART1 at 115200
baud (output on PB14, input on PB15).

The high-current LED driver is on pin PB13.

SWD is on PF1(SWCLK) and PF0(SWD).  Installation is easiest with OpenOCD or
similar SWD probe.

## Building without the built-in MicroPython compiler

This minimal port can be built with the built-in MicroPython compiler
disabled.  This will reduce the firmware by about 20k on a Thumb2 machine,
and by about 40k on 32-bit x86.  Without the compiler the REPL will be
disabled, but pre-compiled scripts can still be executed.

To test out this feature, change the `MICROPY_ENABLE_COMPILER` config
option to "0" in the mpconfigport.h file in this directory.  Then
recompile and run the firmware and it will execute the frozentest.py
file.

# RAIL

The RAIL library overrides some interrupts, which are marked as `weak` in startup code:
* ` RFSENSE_IRQHandler`
* ` AGC_IRQHandler`
* ` BUFC_IRQHandler`
* ` FRC_IRQHandler`
* ` FRC_PRI_IRQHandler`
* ` MODEM_IRQHandler`
* ` PROTIMER_IRQHandler`
* ` RAC_RSM_IRQHandler`
* ` RAC_SEQ_IRQHandler`
* ` SYNTH_IRQHandler`


# Licensing

Micropython is redistributed under a MIT license.  See `../../LICENSE`
for the contents.

The `emlib` and `gecko_sdk` directories are copyright Silicon Laboratories
and are redistributed under a modified 3-clause BSD license:

```
Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com

 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
```

The `startup_efr32mg1p.c` and `system_efr32mg1p.c` files are
copyright ARM Limited and are redistributed under an unmodified 3-clause BSD license:

```
   Copyright (c) 2011 - 2014 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
```

The RAIL RF modem library has its own license in `rail/Silabs_License_Agreement.txt`
