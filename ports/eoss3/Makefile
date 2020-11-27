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

CFLAGS += -DLFS2_NO_MALLOC -DLFS2_NO_DEBUG -DLFS2_NO_WARN -DLFS2_NO_ERROR -DLFS2_NO_ASSERT

LIBS =
#LIBS += /usr/lib/arm-none-eabi/newlib/thumb/v7e-m/fpv4-sp/softfp/libm.a
LIBS += /usr/lib/gcc/arm-none-eabi/6.3.1/thumb/v7e-m/fpv4-sp/softfp/libgcc.a

SRC_C = \
	main.c \
	moduos.c \
	modmachine.c \
	uart_core.c \
	machine_spiflash.c \

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
	drivers/memory/spiflash.c \
	\
	extmod/vfs.c \
	extmod/vfs_reader.c \
	extmod/vfs_blockdev.c \
	extmod/vfs_lfs.c \
	lib/littlefs/lfs2.c \
	lib/littlefs/lfs2_util.c \

NOT_YET = \
	extmod/machine_spi.c \
	extmod/utime_mphal.c \

SRC_QSTR += $(SRC_C)
SRC_QSTR += $(PYLIB_SRC_C)
SRC_QSTR += $(PYMOD_SRC_C)

OBJ = \
	$(PY_CORE_O) \
	$(addprefix $(BUILD)/, $(SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(PYLIB_SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(PYMOD_SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o)) \
	$(BUILD)/frozen_mpy.o

all: $(BUILD)/firmware.bin

# Override the default args to mpy-tool so that the frozen modules
# will have the correct configuration. All the .py files in modules/
# are linked into the firmware.
MPY_TOOL_BIN = $(TOP)/tools/mpy-tool.py
MPY_TOOL = $(PYTHON) $(MPY_TOOL_BIN) -mlongint-impl=mpz
FROZEN_MPY_DIR = modules

mpy-cross: $(MPY_TOOL_BIN)
	$(MAKE) -C ../../mpy-cross

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) \
		-O binary \
		-j .isr_vector \
		-j .text \
		-j .data \
		$^ \
		$@
	$(Q)truncate -s '%256' $@

flash: $(BUILD)/firmware.bin
	python3 ../../../TinyFPGA-Programmer-Application/tinyfpga-programmer-gui.py  \
		--port /dev/ttyACM1 \
		--reset \
		--m4app $< \



include $(TOP)/py/mkrules.mk
