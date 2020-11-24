include ../../py/mkenv.mk

CROSS = 1

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# include py core make definitions
include $(TOP)/py/py.mk

ifeq ($(CROSS), 1)
CROSS_COMPILE ?= arm-none-eabi-
endif

INC += -I.
INC += -I./HAL/inc
INC += -I$(TOP)/lib/cmsis/inc
INC += -I$(TOP)
INC += -I$(BUILD)

ifeq ($(CROSS), 1)
DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CFLAGS_CORTEX_M4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -msoft-float -fsingle-precision-constant -Wdouble-promotion -Wfloat-conversion
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_CORTEX_M4) $(COPT)
LDFLAGS = -nostdlib -T quickfeather.ld -Map=$@.map --cref --gc-sections
else
LD = gcc
CFLAGS = -m32 $(INC) -Wall -Werror -Wdouble-promotion -Wfloat-conversion -std=c99 $(COPT)
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

LIBS =

SRC_C = \
	main.c \
	moduos.c \
	modmachine.c \
	uart_core.c \
	spi.c \

PYLIB_SRC_C = \
	lib/utils/printf.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	lib/utils/sys_stdio_mphal.c \
	lib/utils/interrupt_char.c \

PYMOD_SRC_C = \
	extmod/modubinascii.c \
	extmod/machine_mem.c \
	extmod/machine_spi.c \
	\
	extmod/vfs.c \
	extmod/vfs_reader.c \
	extmod/vfs_blockdev.c \
	extmod/vfs_lfs.c \
	lib/littlefs/lfs2.c \
	lib/littlefs/lfs2_util.c \

NOT_YET = \
	extmod/utime_mphal.c \
	extmod/machine_spiflash.c \
	drivers/memory/spiflash.c \

SRC_QSTR += $(SRC_C)
SRC_QSTR += $(PYLIB_SRC_C)
SRC_QSTR += $(PYMOD_SRC_C)

OBJ = \
	$(PY_CORE_O) \
	$(addprefix $(BUILD)/, $(SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(PYLIB_SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(PYMOD_SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o)) \
	$(BUILD)/_frozen_mpy.o

all: $(BUILD)/firmware.bin

$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $(BUILD)/firmware.bin

flash: $(BUILD)/firmware.bin
	python3 ../../../TinyFPGA-Programmer-Application/tinyfpga-programmer-gui.py  \
		--m4app $< \
		--port /dev/ttyACM1 \
		--reset \



include $(TOP)/py/mkrules.mk
