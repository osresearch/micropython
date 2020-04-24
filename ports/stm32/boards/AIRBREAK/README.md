# MicroPython on the ResMed S10 CPAP/BiPAP machines
![MicroPython console says hello](https://live.staticflickr.com/65535/49801308566_f68836654d_b.jpg)

This is an experimental port of MicroPython to the ResMed S10 CPAP
machines, made posisble by [AirBreak.dev](https://airbreak.dev/).
It is not yet suitable as a replacement for the vendor firmware,
although it is a good starting point for projects that want to
experiment with different features and add new functionality to
the hardware.

## Current status:

[X] Serial console
[X] SD Card

### UI features
[X] Status LEDs
[ ] White LEDs
[ ] LCD drawing
[X] LCD backlight
[X] Buttons

### Motor and sensors control
[X] Openloop, low-speed motor control
[X] Back EMF measurements (uncalibrated)
[ ] High speed, closed loop RPM motor control
[ ] Closed loop pressure control
[ ] Closed loop flow control
[X] Analog pressure sensor (uncalibrated)
[ ] Differential pressure sensor (i2c?)

### Accessories
[ ] Cell modem
[ ] Humidifier
[ ] Heater


## Installation
![ST-Link v2 and TC-2050 connected to SWD port](https://airbreak.dev/images/airsense-stlink.jpg)
Please see <https://airbreak.dev/disassembly> and <https://airbreak.dev/firmware>
for instructions on disassembly and connecting to the JTAG port for
reflashing with OpenOCD.

![Blue accessory pinout](https://live.staticflickr.com/65535/49801320021_12cf03a5e5_b.jpg)

The serial UART is 3.3v TTL at 115200 and can be accessed via pins 6, 7 and 9
on the blu eaccessory connector.


