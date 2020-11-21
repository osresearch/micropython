# Quicklogic EOS port

This minimal port works on the Cortex M4 inside the EOS S3 hybrid MCU.

## Building for an EOS S3

The Makefile has the ability to build for a Cortex-M CPU, and by default
includes some start-up code for the MCU and also enables a UART
on the hardware serial port at 115200 baud for communication.  To build
it should only be necessary to run:

```
    make
```

Building will produce the build/firmware.bin file which can be programmed
to an MCU using the Quicklogic fork of `tinyprog`:

```
python3 TinyFPGA-Programmer-Application/tinyfpga-programmer-gui.py \
	--m4app build/firmware.bin 
```
