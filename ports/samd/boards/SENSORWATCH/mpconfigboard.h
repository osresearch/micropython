#define MICROPY_HW_BOARD_NAME "SensorWatch"
#define MICROPY_HW_MCU_NAME   "SAML22J18A"

#define MICROPY_HW_DFLL_USB_SYNC    (1)
#define MICROPY_PY_MACHINE_ADC (0) // for now
#define MICROPY_PY_MACHINE_DAC (0) // for now
#define MICROPY_PY_MACHINE_PWM (0) // for now

// do not bring in the hal utils_assert.h, use the system one instead
#include <assert.h>
#define _ASSERT_H_INCLUDED
#define ASSERT(x) assert(x)
