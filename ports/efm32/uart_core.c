#include <unistd.h>
#include "py/mpconfig.h"
#include "py/stream.h"

/*
 * Core UART functions to implement for a port
 * Gecko_SDK/platform/Device/SiliconLabs/EFR32MG1P/Include/efm32mg1p_usart.h
 *
 * To find the "Location" values, look in gecko_sdk/include/efr32mg1p_af_pins.h
 *
 */

#if MICROPY_MIN_USE_CORTEX_CPU
#include "em_usart.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "lib/utils/interrupt_char.h"

#define USART USART1
#define USART_CLOCK  cmuClock_USART1

#if 0
// Ikea 10W LED dimmer
#define USART_TX_PORT gpioPortC
#define USART_TX_PIN 10
#define USART_TX_LOCATION 15 // ? found in af_pins.h somehow
#define USART_RX_PORT gpioPortC
#define USART_RX_PIN 11
#define USART_RX_LOCATION 15 // ?
#else
// Ikea On/Off switch, which doesn't use the Gecko board
// There are dedicated TX/RX pins on the underside of the case.
#define USART_TX_PORT gpioPortB
#define USART_TX_PIN 15
#define USART_TX_LOCATION 10 // ? found in af_pins.h somehow
#define USART_RX_PORT gpioPortB
#define USART_RX_PIN 14
#define USART_RX_LOCATION 8 // ?
#endif

#endif


#define CONFIG_RX_IRQ

#ifdef CONFIG_RX_IRQ

#define UART_RX_MASK 0x3F
static volatile uint8_t uart_rx_buf[UART_RX_MASK+1];
static volatile uint8_t uart_rx_head;
static volatile uint8_t uart_rx_tail;
static volatile uint8_t uart_rx_drop;

void USART1_RX_IRQHandler()
{
	if ((USART1->IF & USART_IF_RXDATAV) == 0)
		return;

	uint8_t c = (uint8_t)USART1->RXDATA;

	if (mp_interrupt_char == (int) c)
		mp_keyboard_interrupt();

	uint8_t head = uart_rx_head;
	uint8_t next_head = (head + 1) & UART_RX_MASK;

	// drop the character if there is not room
	if (next_head == uart_rx_tail)
	{
		uart_rx_drop = 1;
		return;
	}

	// room in the queue, store it
	uart_rx_buf[head] = c;
	uart_rx_head = next_head;
}
#endif

// check for any data available
uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags)
{
	if ((poll_flags & MP_STREAM_POLL_RD) == 0)
		return 0;
#ifdef CONFIG_RX_IRQ
	// anything in the queue?
	return uart_rx_head != uart_rx_tail;
#else
	if (USART->STATUS & USART_STATUS_RXDATAV)
		return MP_STREAM_POLL_RD;
	return 0;
#endif
}

// Receive single character
int mp_hal_stdin_rx_chr(void)
{
	unsigned char c = 0;

#ifdef CONFIG_RX_IRQ
	while(!mp_hal_stdio_poll(MP_STREAM_POLL_RD))
		;
	uint8_t tail = uart_rx_tail;
	c = uart_rx_buf[tail];
	uart_rx_tail = (tail + 1) % UART_RX_MASK;
#else
	// spin on the incoming register
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

#ifdef CONFIG_RX_IRQ
  // enable RX interrupts on USART1
  USART_IntClear(USART1, USART_IF_RXDATAV);
  USART_IntEnable(USART1, USART_IF_RXDATAV);

  NVIC_ClearPendingIRQ(USART1_RX_IRQn);
  NVIC_EnableIRQ(USART1_RX_IRQn);
#endif

#endif
}
