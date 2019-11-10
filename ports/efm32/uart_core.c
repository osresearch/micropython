#include <unistd.h>
#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 * Gecko_SDK/platform/Device/SiliconLabs/EFR32MG1P/Include/efm32mg1p_usart.h
 */

#if MICROPY_MIN_USE_STM32_MCU
#include "em_usart.h"
#include "em_cmu.h"
#include "em_gpio.h"

#define USART USART1
#define USART_CLOCK  cmuClock_USART1
#define USART_TX_PORT gpioPortB
#define USART_TX_PIN 14
#define USART_TX_LOCATION 9 // ? found in af_pins.h somehow
#define USART_RX_PORT gpioPortB
#define USART_RX_PIN 15
#define USART_RX_LOCATION 9 // ?

// might be pa0, pa1, pb12 or pb13
#define LED_PORT gpioPortB
#define LED_PIN 13

#endif

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
#if MICROPY_MIN_USE_STDOUT
    int r = read(0, &c, 1);
    (void)r;
#elif MICROPY_MIN_USE_STM32_MCU
    return USART_Rx(USART);
#endif
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
#if MICROPY_MIN_USE_STDOUT
    int r = write(1, str, len);
    (void)r;
#elif MICROPY_MIN_USE_STM32_MCU
    while (len--) {
	unsigned char c = *str++;
        USART_Tx(USART, c);
	if (c & 0x80)
		GPIO_PinOutSet(LED_PORT, LED_PIN);
	else
		GPIO_PinOutClear(LED_PORT, LED_PIN);
    }
#endif
}

void mp_hal_stdout_init(void)
{
#if MICROPY_MIN_USE_STDOUT
    return;
#elif MICROPY_MIN_USE_STM32_MCU
    // 115200 n81
    CMU_ClockEnable(USART_CLOCK, true);
    CMU_ClockEnable(cmuClock_GPIO, true);

    static const USART_InitAsync_TypeDef config = USART_INITASYNC_DEFAULT;
    USART_InitAsync(USART, &config);

  GPIO_PinModeSet(LED_PORT, LED_PIN, gpioModePushPull, 0);

  GPIO_PinModeSet(USART_TX_PORT, USART_TX_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(USART_RX_PORT, USART_RX_PIN, gpioModeInput, 1);

  // Configure route
  USART->ROUTELOC0  = USART_TX_LOCATION << _USART_ROUTELOC0_TXLOC_SHIFT;
  USART->ROUTELOC0 |= USART_RX_LOCATION << _USART_ROUTELOC0_RXLOC_SHIFT;
  USART->ROUTEPEN   = USART_ROUTEPEN_TXPEN | USART_ROUTEPEN_RXPEN;

  // Enable TX/RX
  USART->CMD = USART_CMD_RXEN
                | USART_CMD_TXEN;

#if 0
  // Clock peripherals
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_LDMA, true);
  CMU_ClockEnable(UART_CLOCK, true);

  // Set up USART
  USART->CMD       = USART_CMD_RXDIS
                      | USART_CMD_TXDIS
                      | USART_CMD_MASTERDIS
                      | USART_CMD_RXBLOCKDIS
                      | USART_CMD_TXTRIDIS
                      | USART_CMD_CLEARTX
                      | USART_CMD_CLEARRX;
  USART->CTRL      = _USART_CTRL_RESETVALUE;
  USART->CTRLX     = _USART_CTRLX_RESETVALUE;
  USART->FRAME     = _USART_FRAME_RESETVALUE;
  USART->TRIGCTRL  = _USART_TRIGCTRL_RESETVALUE;
  USART->CLKDIV    = _USART_CLKDIV_RESETVALUE;
  USART->IEN       = _USART_IEN_RESETVALUE;
  USART->IFC       = _USART_IFC_MASK;
  USART->ROUTEPEN  = _USART_ROUTEPEN_RESETVALUE;
  USART->ROUTELOC0 = _USART_ROUTELOC0_RESETVALUE;
  USART->ROUTELOC1 = _USART_ROUTELOC1_RESETVALUE;

  // Configure databits, stopbits and parity
  USART->FRAME = USART_FRAME_DATABITS_EIGHT
                  | USART_FRAME_STOPBITS_ONE
                  | USART_FRAME_PARITY_NONE;

  // Configure oversampling and baudrate
  USART->CTRL |= USART_CTRL_OVS_X16;

  if(CMU->HFCLKSTATUS == CMU_HFCLKSTATUS_SELECTED_HFRCO) {
    refFreq = 19000000;
  } else {
    refFreq = 38400000;
  }

  clkdiv = 32 * refFreq + (16 * UART_BAUDRATE) / 2;
  clkdiv /= (16 * UART_BAUDRATE);
  clkdiv -= 32;
  clkdiv *= 8;

  // Verify that resulting clock divider is within limits
  BTL_ASSERT(clkdiv <= _USART_CLKDIV_DIV_MASK);

  // If asserts are not enabled, make sure we don't write to reserved bits
  clkdiv &= _USART_CLKDIV_DIV_MASK;

  USART->CLKDIV = clkdiv;

#endif
#endif
}

void gpio(int value)
{
	if (value)
		GPIO_PinOutSet(LED_PORT, LED_PIN);
	else
		GPIO_PinOutClear(LED_PORT, LED_PIN);
}
