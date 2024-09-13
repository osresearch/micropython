MCU_SERIES = SAML22
CMSIS_MCU = SAML22J18A
LD_FILES = boards/saml22j18.ld sections.ld
TEXT0 = 0x2000

# saml22j18 has 256KB of flash
MICROPY_HW_CODESIZE=248K


# Only the SAML22 has the SLCD driver, so it is not part of
# the main SAMD source tree
SRC_C += \
	boards/SENSORWATCH/slcd.c \
	boards/SENSORWATCH/hpl_slcd.c \

INC += -I$(TOP)/lib/asf4/$(MCU_SERIES_LOWER)/hpl/slcd
INC += -I$(BOARD_DIR)/config

ASF4_SRC_C += $(addprefix lib/asf4/$(MCU_SERIES_LOWER)/,\
	hal/src/hal_slcd_sync.c \
	hpl/core/hpl_init.c \
	hpl/pm/hpl_pm.c \
	hpl/mclk/hpl_mclk.c \
	hpl/gclk/hpl_gclk.c \
	hpl/oscctrl/hpl_oscctrl.c \
	hpl/osc32kctrl/hpl_osc32kctrl.c \
)

