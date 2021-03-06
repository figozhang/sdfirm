#include <target/uart.h>

/* XXX: AT90SCR100 has only one USART: USART0 */
AT90SCR100_USART(0)

static void __uart_hw_config_brg(uint32_t baudrate)
{
	uint16_t brg;
	uint8_t divisor;

	/* Baudrate Generator Calculation Equation
	 *
	 * baudrate = (CLK_IO * 1000) / ((divisor) * (UBRR + 1))
	 * thus
	 * UBRR = (CLK_IO * 1000) / (baudrate * divisor) - 1
	 *
	 * Considering error rate, UBRR = UBRR_error + 0.5
	 * UBRR = (CLK_IO * 1000) / (baudrate * divisor) - 0.5
	 *	= ((((CLK_IO / divisor) * 20) / (baudrate / 100)) - 1) / 2
	 */
	/* Since 4096 > UBRR >= 0, from the following formula:
	 * UBRR = (CLK_IO * 1000) / (baudrate * divisor) - 0.5
	 * we know that there are tow configurable mode choices:
	 * 1: U2XN = 1, divisor = 8 (1 * 8);
	 * 2: U2XN = 0, divisor = 16 (2 * 8),
	 * thus,
	 * UBRR = (CLK_IO * 125) / (baudrate * choice) - 0.5
	 *      = ((CLK_IO * 250) / (baudrate * choice) - 1) / 2
	 *
	 * considering UBRR >= 0, it means:
	 * (baudrate / 250) <= (CLK_IO / choice)
	 * this could mean:
	 * if (baudrate / 250) > (CLK_IO), we have to choose 2
	 * if (baudrate / 250) <= (CLK_IO / 2), we can choose 2
	 *
	 * considering UBRR < 4096, it means:
	 * (baudrate / 250) > ((CLK_IO / 8193) / choice)
	 * this could mean:
	 * if (baudrate / 250) > (CLK_IO / 8193), we can choose 1
	 * if (baudrate / 250) <= (CLK_IO / 8193), we have to choose 2
	 */
	if (div32u(baudrate, 250) <= (uint32_t)div16u(CLK_IO, 8193) ||
	    div32u(baudrate, 250) > CLK_IO) {
		__usart0_hw_clear_double();
		divisor = 16;
	} else {
		__usart0_hw_set_double();
		divisor = 8;
	}
	brg = div16u((uint16_t)(mul16u(div16u(CLK_IO, divisor), 20)),
		     (uint16_t)(div32u(baudrate, 100))) - 1;
	brg = div16u(brg, 2);

	__usart0_hw_config_brg(brg);
}

void uart_hw_dbg_config(uint8_t params, uint32_t baudrate)
{
	uint8_t bits = uart_bits(params) - 5;

	/* asynchronous mode */
	__usart0_hw_config_mode(USART_HW_MODE_ASYNC);

	/* transmission bits */
	__usart0_hw_set_bits(bits);
	/* parity */
	__usart0_hw_set_parity(uart_parity(params));
	/* stop bits */
	__usart0_hw_set_stopb(uart_stopb(params));

	__uart_hw_config_brg(baudrate);
}

void uart_hw_dbg_write(uint8_t byte)
{
	__usart0_hw_write_byte(byte);
}

void uart_hw_dbg_start(void)
{
	__usart0_hw_enable_io();
}

void uart_hw_dbg_stop(void)
{
	__usart0_hw_disable_io();
}
