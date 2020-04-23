MCU_SERIES = f4
CMSIS_MCU = STM32F405xx
AF_FILE = boards/stm32f405_af.csv
LD_FILES = boards/stm32f405.ld boards/common_ifs.ld
# should generate one file, not two
#TEXT_ADDR = 0x08000000
#TEXT1_ADDR = 0x08020000
CFLAGS += -g

FROZEN_MANIFEST = $(BOARD_DIR)/manifest.py

