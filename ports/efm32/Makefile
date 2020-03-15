BOARD ?= ikea

ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

include boards/$(BOARD)/mpconfigboard.mk

BUILD ?= build-$(BOARD)

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

GECKO_SDK = lib/efm32-base
EFM_FAMILY_UC := $(shell echo '$(EFM_FAMILY)' | tr '[:lower:]' '[:upper:]')
EFM_DIR := $(GECKO_SDK)/device/$(EFM_FAMILY_UC)
EMLIB_DIR := $(GECKO_SDK)/emlib/src
#RAIL_DIR := $(GECKO_SDK)/radio/rail_lib
RAIL_DIR := rail

# Checkout the efm32-base submodule if it is not
# already checked out
$(shell if [ ! -r "../../$(GECKO_SDK)/README.md" ]; then \
	echo >&2 "gecko_sdk missing: checkout efm32-base" ; \
	git submodule update --depth 1 --init ../../lib/efm32-base >&2 ; \
fi )

INC += -I.
INC += -I../../$(EMLIB_DIR)/../inc
INC += -I../../$(EFM_DIR)/Include
INC += -I../../$(GECKO_SDK)/cmsis/Include/
INC += -D__NO_SYSTEM_INIT
INC += -D__STARTUP_CLEAR_BSS
INC += -D__STACK_SIZE=0x1000
INC += -D__HEAP_SIZE=0x6000
# should the radio lib be optional?
#INC += -I../../$(RAIL_DIR)
#INC += -I../../$(RAIL_DIR)/common
#INC += -I../../$(RAIL_DIR)/chip/efr32/$(RAIL_FAMILY)
#LIBS += ../../$(GECKO_SDK)/protocol/bluetooth/lib/$(EFM_FAMILY_UC)/GCC/librail.a
INC += -I$(RAIL_DIR)

# RAIL_LIB must be specified by the board/$(BOARD)/mpconfigboard.mk
LIBS += $(RAIL_LIB)


INC += -I$(TOP)
INC += -I$(BUILD)
INC += -Iboards/$(BOARD)

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
	-D$(EFM_DEV) \
	-falign-functions=2 \

LIBM	:= $(shell $(CROSS_COMPILE)gcc $(CFLAGS_$(MCU_SERIES)) --print-file-name=libm.a)
LIBGCC	:= $(shell $(CROSS_COMPILE)gcc $(CFLAGS_$(MCU_SERIES)) --print-libgcc)

LIBS += $(LIBM) $(LIBGCC)

LINKER_SCRIPT = ../../$(EFM_DIR)/Source/GCC/$(EFM_FAMILY).ld

CFLAGS = $(INC) -g -Wall -Werror -std=c99 -nostdlib $(CFLAGS_$(MCU_SERIES)) $(COPT)
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
	pin.c \
	pwm.c \
	moduos.c \
	modmachine.c \
	modutime.c \
	modcrypto.c \
	radio.c \
	zrepl.c \

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
	extmod/machine_pin.c \
	extmod/machine_pwm.c \
	extmod/machine_spi.c \
	extmod/machine_spiflash.c \
	extmod/utime_mphal.c \
	drivers/bus/softspi.c \
	drivers/memory/spiflash.c \
	\
	extmod/vfs.c \
	extmod/vfs_reader.c \
	extmod/vfs_blockdev.c \
	extmod/vfs_lfs.c \
	lib/littlefs/lfs2.c \
	lib/littlefs/lfs2_util.c \

# Scan all of the source, python libraries and python module files for QSTR
SRC_QSTR += $(SRC_C)
SRC_QSTR += $(PYLIB_SRC_C)
SRC_QSTR += $(PYMOD_SRC_C)

# EMLIB from Silicon Labs for the Gecko boards
# These are not included in the QSTR scan
LIB_C += \
	$(EFM_DIR)/Source/GCC/startup_$(EFM_FAMILY).c \
	$(EFM_DIR)/Source/system_$(EFM_FAMILY).c \
	$(EMLIB_DIR)/em_usart.c \
	$(EMLIB_DIR)/em_cmu.c \
	$(EMLIB_DIR)/em_gpio.c \
	$(EMLIB_DIR)/em_system.c \
	$(EMLIB_DIR)/em_core.c \
	$(EMLIB_DIR)/em_emu.c \
	$(EMLIB_DIR)/em_crypto.c \
	$(EMLIB_DIR)/em_timer.c \

OBJ = $(PY_CORE_O) \
	$(addprefix $(BUILD)/, $(SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(PYLIB_SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(PYMOD_SRC_C:.c=.o)) \
	$(addprefix $(BUILD)/, $(LIB_C:.c=.o)) \
	$(BUILD)/frozen_mpy.o \

CFLAGS += $(CFLAGS_MOD)
CFLAGS += -DLFS2_NO_MALLOC -DLFS2_NO_DEBUG -DLFS2_NO_WARN -DLFS2_NO_ERROR -DLFS2_NO_ASSERT

ifeq ($(CROSS), 1)
all: $(BUILD)/firmware.dfu
else
all: $(BUILD)/firmware.elf
endif

# Override the default args to mpy-tool so that the frozen modules
# will have the correct configuration. All the .py files in modules/
# are linked into the firmware.
MPY_TOOL_BIN = $(TOP)/tools/mpy-tool.py
MPY_TOOL = $(PYTHON) $(MPY_TOOL_BIN) -mlongint-impl=none
FROZEN_MPY_DIR = modules

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $(BUILD)/firmware.bin

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

openocd:
	openocd -f interface/cmsis-dap.cfg -f board/efm32.cfg

gdb: $(BUILD)/firmware.elf
	gdb-multiarch \
		-iex 'target remote localhost:3333' \
		-iex 'display /i $pc' \
		$<

$(EFM_FAMILY)-pins.md: gecko_sdk/include/$(EFM_FAMILY)_af_ports.h  gecko_sdk/include/$(EFM_FAMILY)_af_pins.h
	./make-pins $^ > $@

tags: FORCE
	ctags -R \
		. \
		../../lib \
		../../py \
		../../extmod \
		../../drivers  \


test: $(BUILD)/firmware.elf
	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"

include $(TOP)/py/mkrules.mk
