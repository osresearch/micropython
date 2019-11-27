include ../../py/mkenv.mk

# Default to building a cross compiler
CROSS ?= 1

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

ifeq ($(CROSS), 1)
CROSS_COMPILE = arm-none-eabi-
endif

INC += -I.
INC += -Igecko_sdk/include
INC += -Iemlib/inc
# should the radio lib be optional?
INC += -Irail
LIBS += rail/TARGET_EFR32_1/librail_efr32xg1_release.a

# does this m4 have hardfloat?
#LIBS += /usr/lib/gcc/arm-none-eabi/6.3.1/thumb/libgcc.a
LIBS += /usr/lib/gcc/arm-none-eabi/6.3.1/thumb/v7e-m/fpv4-sp/softfp/libgcc.a

INC += -I$(TOP)
INC += -I$(BUILD)

ifeq ($(CROSS), 1)
DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CFLAGS_CORTEX_M4 = \
	-mthumb \
	-mtune=cortex-m4 \
	-mcpu=cortex-m4 \
	-msoft-float \
	-fsingle-precision-constant \
	-Wdouble-promotion \
	-DEFR32MG1P132F256GM32 \
	-falign-functions=2 \

#LINKER_SCRIPT = tradfri-10w.ld
LINKER_SCRIPT = efr32mg1p.ld

CFLAGS = $(INC) -g -Wall -Werror -std=c99 -nostdlib $(CFLAGS_CORTEX_M4) $(COPT)
LDFLAGS = -nostdlib -T $(LINKER_SCRIPT) -Map=$@.map --cref --gc-sections
else
LD = gcc
CFLAGS = -m32 $(INC) -Wall -Werror -std=c99 $(COPT)
LDFLAGS = -m32 -Wl,-Map=$@.map,--cref -Wl,--gc-sections
endif

CSUPEROPT = -Os # save some code space

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

SRC_C = \
	main.c \
	uart_core.c \
	gpio.c \
	radio.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	lib/utils/sys_stdio_mphal.c \
	lib/utils/interrupt_char.c \

# Scan all of the source files for QSTR
SRC_QSTR += $(SRC_C)

# EMLIB from Silicon Labs for the Gecko boards
# These are not included in the QSTR scan
LIB_C += \
	system_efr32mg1p.c \
	startup_efr32mg1p.c \
	../emlib/src/em_usart.c \
	../emlib/src/em_cmu.c \
	../emlib/src/em_gpio.c \
	../emlib/src/em_system.c \
	../emlib/src/em_core.c \
	../emlib/src/em_emu.c \

OBJ = $(PY_CORE_O) \
	$(addprefix $(BUILD)/, $(SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(LIB_C:.c=.o)) \
	$(BUILD)/frozen_mpy.o \

ifeq ($(CROSS), 1)
all: $(BUILD)/firmware.dfu
else
all: $(BUILD)/firmware.elf
endif

# Override the default args to mpy-tool so that the frozen modules
# will have the correct configuration. All the .py files in modules/
# are linked into the firmware.
MPY_TOOL = $(PYTHON) $(TOP)/tools/mpy-tool.py -mlongint-impl=none
FROZEN_MPY_DIR = modules

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $(BUILD)/firmware.bin

$(BUILD)/led10w.bin: $(BUILD)/firmware.bin
	cp "$<" "$@"
	dd if="$(HOME)/ikea/led10w.bin" of="$@" bs=1024 count=196 conv=notrunc
	# fixup the stack pointer to make room for our bss
	perl -e 'print pack("V", 0x200041c0 - 0x800)' \
		| dd of="$@" bs=1 seek=16384 conv=notrunc # 0x4000
	# jump to microptyhon_setup in place of the nvram check
	# ldr r0, [pc,#4]; ldr r0, [r0]; blx r0; bx lr ; ADDRESS
	perl -e 'print pack("H*V", "0148006800477047", 0x31000)' \
		| dd of="$@" bs=1 seek=60260 conv=notrunc # 0xeb64
no:
	# jump to micropython_loop in the serial buffer loop
	# ldr r0, [pc,#4]; ldr r0, [r0]; bx r0; bx lr ; ADDRESS
	perl -e 'print pack("H*V", "0148006800477047", 0x31004)' \
		| dd of="$@" bs=1 seek=34436 conv=notrunc # 0x8684

$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(DFU) -b 0x08000000:$(BUILD)/firmware.bin $@

deploy: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $< to the board"
	$(Q)$(PYTHON) $(PYDFU) -u $<

# Run emulation build on a POSIX system with suitable terminal settings
run:
	stty raw opost -echo
	build/firmware.elf
	@echo Resetting terminal...
# This sleep is useful to spot segfaults
	sleep 1
	reset

test: $(BUILD)/firmware.elf
	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"

include $(TOP)/py/mkrules.mk
