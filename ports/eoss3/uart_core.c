#include <unistd.h>
#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 */
#include "eoss3_hal_uart.h"
#include "eoss3_dev.h"
#include "s3x_clock_hal.h"

// Receive single character
int mp_hal_stdin_rx_chr(void)
{
    // wait for anything in the FIFO
    while (UART->UART_TFR & UART_TFR_RX_FIFO_EMPTY)
	;
    const unsigned char c = UART->UART_DR & 0xFF;
    return c;
}

static void uart_putc(const char c)
{
        // wait for the TX fifo to be not full
        while (UART->UART_TFR & UART_TFR_TX_FIFO_FULL)
		;

        UART->UART_DR = c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
    while (len--)
	uart_putc(*str++);
}

// Send nul terminated string
void mp_hal_stdout_tx_str(const char *str)
{
    while (*str)
	uart_putc(*str++);
}

// Send a string of a given length, but replace \n with \r\n
void mp_hal_stdout_tx_strn_cooked(const char *str, mp_uint_t len)
{
	while (len--)
	{
		const char c = *str++;
		if (c == '\n')
			uart_putc('\r');
		uart_putc(c);
	}
}


void mp_hal_uart_init(const unsigned baud_in)
{
	UART->UART_CR = 0;
	UART->UART_IMSC = 0;
	UART->UART_RSR = 0;
	UART->UART_LCR_H = UART_LCR_WLEN_8_BITS | UART_LCR_ENABLE_FIFO;

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

        // Set up IO pin for TX, RX?

	// enable the UART now that it is configured
	UART->UART_CR = UART_CR_UART_ENABLE
		| UART_CR_RX_ENABLE
		| UART_CR_TX_ENABLE;
}
