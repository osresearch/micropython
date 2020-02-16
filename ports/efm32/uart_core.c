#include <unistd.h>
#include <string.h>
#include "py/mpconfig.h"
#include "py/stream.h"

/*
 * Core UART functions to implement for a port
 * Gecko_SDK/platform/Device/SiliconLabs/EFR32MG1P/Include/efm32mg1p_usart.h
 *
 * To find the "Location" values, look in gecko_sdk/include/efr32mg1p_af_pins.h
 *
 */

#include "em_usart.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "lib/utils/interrupt_char.h"
#include "zrepl.h"

// this definition is insufficient; there are places where usart1 is used
#define USART USART1
#define USART_CLOCK  cmuClock_USART1

#define CONFIG_RX_IRQ
#define CONFIG_TX_IRQ

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




#ifdef CONFIG_RX_IRQ

#define UART_RX_MASK 0x3F
static volatile uint8_t uart_rx_buf[UART_RX_MASK+1];
static volatile uint8_t uart_rx_head;
static volatile uint8_t uart_rx_tail;
static volatile uint8_t uart_rx_drop;

// receive a character from the uart or from the zrepl
void uart_recv(uint8_t c)
{
	if (mp_interrupt_char == (int) c)
		mp_keyboard_interrupt();

	const uint8_t head = uart_rx_head;
	const uint8_t next_head = (head + 1) & UART_RX_MASK;

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

void USART1_RX_IRQHandler()
{
	if ((USART1->IF & USART_IF_RXDATAV) == 0)
		return;

	uart_recv((uint8_t)USART1->RXDATA);

}
#endif

// check for any data available
uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags)
{
	if ((poll_flags & MP_STREAM_POLL_RD) == 0)
		return 0;
#ifdef CONFIG_RX_IRQ
	// anything in the queue?
	if (uart_rx_head != uart_rx_tail)
		return MP_STREAM_POLL_RD;
#else
	if (USART->STATUS & USART_STATUS_RXDATAV)
		return MP_STREAM_POLL_RD;
#endif
	return 0;
}

// Receive single character
int mp_hal_stdin_rx_chr(void)
{
	unsigned char c = 0;

#ifdef CONFIG_RX_IRQ
	while(!mp_hal_stdio_poll(MP_STREAM_POLL_RD))
		;
	const uint8_t tail = uart_rx_tail;
	c = uart_rx_buf[tail];
	uart_rx_tail = (tail + 1) & UART_RX_MASK;
#else
	// spin on the incoming register
	c = USART_Rx(USART);
#endif
	return c;
}


#ifdef CONFIG_TX_IRQ

#define UART_TX_MASK 0x3F
static volatile uint8_t uart_tx_buf[UART_TX_MASK+1];
static volatile uint8_t uart_tx_head;
static volatile uint8_t uart_tx_tail;

void uart_tx_flush(void)
{
	while (uart_tx_tail != uart_tx_head)
		;
}

void USART1_TX_IRQHandler(void)
{
	// nothing to do if the queue is empty
	const uint8_t tail = uart_tx_tail;
	const uint8_t tail_next = (tail + 1) & UART_TX_MASK;

	if (tail == uart_tx_head)
	{
		// turn off the buffer level interrupt
		USART_IntDisable(USART1, USART_IF_TXBL);
		return;
	}

	const uint8_t c = uart_tx_buf[tail];
	USART->TXDATA = (uint32_t) c;

	uart_tx_tail = tail_next;
}
#endif


static void uart_putc(uint8_t c, bool blocking)
{
#ifdef CONFIG_TX_IRQ
	const uint8_t head = uart_tx_head;
	const uint8_t head_next = (head + 1) & UART_TX_MASK;

	do {
		// wait for space in the queue to open up
		if (head_next == uart_tx_tail)
			continue;

		uart_tx_buf[head] = c;
		uart_tx_head = head_next;

		// if the interrupts are not currently enabled,
		// there are no pending characters and we can start it
		USART_IntEnable(USART1, USART_IF_TXBL);
		break;
	} while(blocking);
#else
	while (!(USART->STATUS & USART_STATUS_TXBL))
	{
		if (!blocking)
			return;
	}

	USART_Tx(USART, c);
#endif
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
	if (zrepl_active)
		zrepl_send(str, len);

	while (len--)
		uart_putc(*str++, 1);
}

// Send a string of a given length, but replace \n with \r\n
void mp_hal_stdout_tx_strn_cooked(const char *str, mp_uint_t len)
{
	if (zrepl_active)
		zrepl_send(str, len);

	while (len--)
	{
		const char c = *str++;
		if (c == '\n')
			uart_putc('\r', 1);
		uart_putc(c, 1);
	}
}

void mp_hal_stdout_tx_str(const char *str)
{
	mp_hal_stdout_tx_strn(str, strlen(str));
}


void mp_hal_stdout_init(void)
{
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

#ifdef CONFIG_TX_IRQ
  // configure TX interrupts on USART1, but do not enable yet
  USART_IntClear(USART1, USART_IF_TXBL);
  //USART_IntEnable(USART1, USART_IF_TXBL);

  NVIC_ClearPendingIRQ(USART1_TX_IRQn);
  NVIC_EnableIRQ(USART1_TX_IRQn);
#endif
}
