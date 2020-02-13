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

EFM_FAMILY = efr32mg1p
EFM_DEV = EFR32MG1P132F256GM32
EFM_DIR = $(EFM_FAMILY)

# does this m4 have hardfloat?
#LIBS += /usr/lib/gcc/arm-none-eabi/6.3.1/thumb/libgcc.a
LIBS += /usr/lib/arm-none-eabi/newlib/thumb/v7e-m/fpv4-sp/softfp/libm.a
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
	-D$(EFM_DEV) \
	-falign-functions=2 \

LINKER_SCRIPT = $(EFM_DIR)/$(EFM_FAMILY).ld

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
	../$(EFM_DIR)/startup_$(EFM_FAMILY).c \
	../$(EFM_DIR)/system_$(EFM_FAMILY).c \
	../emlib/src/em_usart.c \
	../emlib/src/em_cmu.c \
	../emlib/src/em_gpio.c \
	../emlib/src/em_system.c \
	../emlib/src/em_core.c \
	../emlib/src/em_emu.c \
	../emlib/src/em_crypto.c \
	../emlib/src/em_timer.c \

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

mpy-cross: $(MPY_TOOL_BIN)
	$(MAKE) -C ../../mpy-cross

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

test: $(BUILD)/firmware.elf
	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"

include $(TOP)/py/mkrules.mk
