#include <unistd.h>
#include "py/mpconfig.h"
#include "py/stream.h"
#include "py/runtime.h"

/*
 * Core UART functions to implement for a port
 */
#include "eoss3_hal_uart.h"
#include "eoss3_dev.h"
#include "s3x_clock_hal.h"

static void uart_putc(uint8_t c, bool blocking);
//static const char hexdigit[16] = "0123456789abcdef";

#define CONFIG_RX_IRQ
#undef CONFIG_TX_IRQ

#ifdef CONFIG_RX_IRQ
#define UART_RX_MASK 0x3F
static volatile uint8_t uart_rx_buf[UART_RX_MASK+1];
static volatile uint32_t uart_rx_head;
static volatile uint32_t uart_rx_tail;
static volatile uint32_t uart_rx_drop;
static char mp_interrupt_char = '\x3'; // control c

// receive a character from the uart or from the zrepl
void uart_recv(const uint8_t c)
{
	if (mp_interrupt_char == (int) c)
		mp_keyboard_interrupt();

	const uint8_t head = uart_rx_head;
	const uint8_t next_head = (head + 1) & UART_RX_MASK;

	// drop the character if there is not room
	if (next_head == uart_rx_tail)
	{
		UART->UART_DR = '!';
		uart_rx_drop = 1;
		return;
	}

	// room in the queue, store it
	uart_rx_buf[head] = c;
	uart_rx_head = next_head;
}
#endif

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

static void uart_send(void)
{
	// nothing to do if the queue is empty
	const uint8_t tail = uart_tx_tail;
	const uint8_t tail_next = (tail + 1) & UART_TX_MASK;

	if (tail == uart_tx_head)
	{
		// turn off the buffer level interrupt
		UART->UART_IMSC &= ~UART_IMSC_TX;
		return;
	}

	const uint8_t c = uart_tx_buf[tail];
	UART->UART_DR = c;

	uart_tx_tail = tail_next;
}
#endif

void uart_handler(void)
{
#if 0
	uart_putc(hexdigit[(uartid >> 4) & 0xF], 1);
	uart_putc(hexdigit[(uartid >> 0) & 0xF], 1);
	uart_putc('=', 1);
	const uint32_t ris = UART->UART_RIS;
	const uint32_t tfr = UART->UART_TFR;
	uart_putc(hexdigit[(ris >> 8) & 0xF], 1);
	uart_putc(hexdigit[(ris >> 4) & 0xF], 1);
	uart_putc(hexdigit[(ris >> 0) & 0xF], 1);
	uart_putc(',', 1);
	uart_putc(hexdigit[(tfr >> 8) & 0xF], 1);
	uart_putc(hexdigit[(tfr >> 4) & 0xF], 1);
	uart_putc(hexdigit[(tfr >> 0) & 0xF], 1);
	uart_putc('\r', 1);
	uart_putc('\n', 1);
#endif

#ifdef CONFIG_RX_IRQ
	if (UART->UART_MIS & UART_MIS_RX)
		UART->UART_ICR = UART_IC_RX;

	if (UART->UART_MIS & UART_MIS_RX_TIMEOUT)
		UART->UART_ICR = UART_IC_RX_TIMEOUT;

	while (!(UART->UART_TFR & UART_TFR_RX_FIFO_EMPTY))
		uart_recv(UART->UART_DR);
#endif

#ifdef CONFIG_TX_IRQ
	if (UART->UART_MIS & UART_MIS_TX)
		UART->UART_ICR = UART_IC_TX;

	while (!(UART->UART_TFR & UART_TFR_TX_FIFO_FULL))
		uart_send();
#endif

	// flag that we've handled the UART interrupt
	INTR_CTRL->OTHER_INTR &= UART_INTR_DETECT;
}

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
	if (!(UART->UART_TFR & UART_TFR_RX_FIFO_EMPTY))
		return MP_STREAM_POLL_RD;
#endif
	return 0;
}

// Receive single character
int mp_hal_stdin_rx_chr(void)
{
#ifdef CONFIG_RX_IRQ
	while (uart_rx_head == uart_rx_tail)
		;
	const uint32_t tail = uart_rx_tail;
	const unsigned char c = uart_rx_buf[tail];
	uart_rx_tail = (tail + 1) & UART_RX_MASK;
#else
	// spin on the incoming register
	// wait for anything in the FIFO
	while (UART->UART_TFR & UART_TFR_RX_FIFO_EMPTY)
		;
	const unsigned char c = UART->UART_DR & 0xFF;
#endif
	return c;
}


static void uart_putc(uint8_t c, bool blocking)
{
#ifdef CONFIG_TX_IRQ
	const uint8_t head = uart_tx_head;
	const uint8_t head_next = (head + 1) & UART_TX_MASK;

	// wait for space, return early if not blocking
	while (head_next == uart_tx_tail)
	{
		if (!blocking)
			return;
	}

	uart_tx_buf[head] = c;
	uart_tx_head = head_next;

	// reenable TX interrupts (even if they are already enabled)
	UART->UART_IMSC = UART_IMSC_TX;
#else
	// wait for space, return early if not blocking
	while (UART->UART_TFR & UART_TFR_TX_FIFO_FULL)
	{
		if (!blocking)
			return;
	}

	UART->UART_DR = c;
#endif
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
    while (len--)
	uart_putc(*str++, 1);
}

// Send nul terminated string
void mp_hal_stdout_tx_str(const char *str)
{
    while (*str)
	uart_putc(*str++, 1);
}

// Send a string of a given length, but replace \n with \r\n
void mp_hal_stdout_tx_strn_cooked(const char *str, mp_uint_t len)
{
	while (len--)
	{
		const char c = *str++;
		if (c == '\n')
			uart_putc('\r', 1);
		uart_putc(c, 1);
	}
}


void mp_hal_uart_init(const unsigned baud_in)
{
	UART->UART_CR = 0;
	UART->UART_IMSC = 0;
	UART->UART_RSR = 0;
	UART->UART_LCR_H = UART_LCR_WLEN_8_BITS; // | UART_LCR_ENABLE_FIFO;

	//S3x_Clk_Enable(S3X_M4_PRPHRL_CLK);
	// Setup the Clock Source as OSC
	CRU->CLK_SWITCH_FOR_D = 0; // ( default )
	// 2. Setup the Divider based on OSC frequency
	CRU->CLK_CTRL_D_0 = 0x20E; // ( divide by 16 , default )
	// 3. Unlock the Clock Gate register
	MISC->LOCK_KEY_CTRL = MISC_LOCK_KEY;
	// 4. Enable the Clock Gate
	CRU->C11_CLK_GATE = 1; // This register is write-protected. Please do Step 3 first to unlock access to this register

	// assuming they haven't reconfigured the OSC register
	const unsigned clock = 76970000 / 16;
	const unsigned baud = baud_in * 16;
	UART->UART_IBRD = clock / baud;
	UART->UART_FBRD = (64 * (clock % baud)) / baud;

#if defined(CONFIG_RX_IRQ) || defined(CONFIG_TX_IRQ)
	INTR_CTRL->OTHER_INTR &= UART_INTR_DETECT;
	INTR_CTRL->OTHER_INTR_EN_M4 |= UART_INTR_EN_M4;
	NVIC_ClearPendingIRQ(Uart_IRQn);
	//NVIC_SetPriority(Uart_IRQn, 0); // ?
	NVIC_EnableIRQ(Uart_IRQn);
#endif

#ifdef CONFIG_RX_IRQ
	// Only RX interrupts need to be turned on;
	// TX interrupts will be enabled when data is ready
	UART->UART_IMSC = UART_IMSC_RX;
#endif

        // Set up IO pin for TX, RX?

	// enable the UART now that it is configured
	UART->UART_CR = UART_CR_UART_ENABLE
		| UART_CR_RX_ENABLE
		| UART_CR_TX_ENABLE;
}
