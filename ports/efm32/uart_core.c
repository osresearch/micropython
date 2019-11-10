#include <unistd.h>
#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 * Gecko_SDK/platform/Device/SiliconLabs/EFR32MG1P/Include/efm32mg1p_usart.h
 */

#if MICROPY_MIN_USE_CORTEX_CPU
#include "em_usart.h"
#include "em_cmu.h"
#include "em_gpio.h"

#define USART USART1
#define USART_CLOCK  cmuClock_USART1
#define USART_TX_PORT gpioPortC
#define USART_TX_PIN 10
#define USART_TX_LOCATION 15 // ? found in af_pins.h somehow
#define USART_RX_PORT gpioPortC
#define USART_RX_PIN 11
#define USART_RX_LOCATION 15 // ?

#endif

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
#if MICROPY_MIN_USE_STDOUT
    int r = read(0, &c, 1);
    (void)r;
#elif MICROPY_MIN_USE_CORTEX_CPU
    c = USART_Rx(USART);
#endif
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
#if MICROPY_MIN_USE_STDOUT
    int r = write(1, str, len);
    (void)r;
#elif MICROPY_MIN_USE_CORTEX_CPU
    while (len--) {
        USART_Tx(USART, *str++);
    }
#endif
}

void mp_hal_stdout_init(void)
{
#if MICROPY_MIN_USE_STDOUT
    return;
#elif MICROPY_MIN_USE_CORTEX_CPU
    // 115200 n81
    CMU_ClockEnable(USART_CLOCK, true);
    CMU_ClockEnable(cmuClock_GPIO, true);

    static const USART_InitAsync_TypeDef config = USART_INITASYNC_DEFAULT;
    USART_InitAsync(USART, &config);

  GPIO_PinModeSet(USART_TX_PORT, USART_TX_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(USART_RX_PORT, USART_RX_PIN, gpioModeInput, 1);

  // Configure route
  USART->ROUTELOC0  = USART_TX_LOCATION << _USART_ROUTELOC0_TXLOC_SHIFT;
  USART->ROUTELOC0 |= USART_RX_LOCATION << _USART_ROUTELOC0_RXLOC_SHIFT;
  USART->ROUTEPEN   = USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_RXPEN;

  // Enable TX/RX
  USART->CMD = USART_CMD_RXEN
                | USART_CMD_TXEN;
#endif
}
