#define MICROPY_HW_BOARD_NAME "SensorWatch"
#define MICROPY_HW_MCU_NAME   "SAML22J18A"
#define MICROPY_HW_XOSC32K  (1) // 32 KHz watch crystal

#define MICROPY_HW_DFLL_USB_SYNC    (0) // use the 32 KHz crystal instead
#define MICROPY_PY_MACHINE_DAC (0) // the saml22j18 has no DAC

// do not bring in the hal utils_assert.h, use the system one instead
#include <assert.h>
#define _ASSERT_H_INCLUDED
#define ASSERT(x) assert(x)
