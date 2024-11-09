#ifndef _BSP_H_
#define _BSP_H_

#include "util_types.h"

#define UART_CONSOLE_BUFFER_SIZE 256
#define uart_console_isr         USART1_IRQHandler
#define uart_console_put         console_put
#define uart_console_puts        console_puts
#define uart_console_putc        console_putc
#define uart_console_getc        console_getc
#define uart_console_init        console_init


int  uart_console_init(uint32_t bound);
int  uart_console_put(const uint8_t* data, uint16_t len);
int  uart_console_puts(const char* data);
int  uart_console_putc(char c);
int  uart_console_getc(void);
void uart_console_isr(void);

int sysirq_init(void);

#endif
