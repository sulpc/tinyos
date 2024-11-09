#include "bsp.h"
#include "util_ringbuffer.h"

#include <stm32f10x.h>

/**
 * USART1:
 *   PA9  - TX
 *   PA10 - RX
 */


#define uart_putc(c)                                                                                                   \
    do {                                                                                                               \
        USART1->DR = (c);                                                                                              \
        while ((USART1->SR & 0x0040) == 0)                                                                             \
            ;                                                                                                          \
    } while (0)

static bool              uart_console_inited = false;
static uint8_t           uart_console_data_buffer[UART_CONSOLE_BUFFER_SIZE];
static util_ringbuffer_t uart_console_buffer;
static uint8_t           group_prio_bits = 0;

static void nvic_priogroup_config(uint8_t group_bits)
{
    group_prio_bits = (group_bits > 4) ? 4 : group_bits;

    uint32_t temp = SCB->AIRCR;
    temp          = (temp & 0x0000F8FF) | 0x05FA0000;   // write key
    temp |= (7 - group_prio_bits) << 8;
    SCB->AIRCR = temp;
}


void nvic_config(uint8_t prio, uint8_t sub_prio, uint8_t irq)
{
    // ISER: 8 regs, 240 bits
    NVIC->ISER[irq >> 5] |= 1 << (irq & 0x1F);   // enable irq

    prio     = prio << (8 - group_prio_bits);
    sub_prio = sub_prio << 4;

    // NVIC_IP[7:4] : prio+sub_prio
    NVIC->IP[irq] |= (prio | sub_prio) & 0xF0;
}


int sysirq_init(void)
{
    nvic_priogroup_config(2);
    return 0;
}


int uart_console_init(uint32_t bound)
{
    util_ringbuffer_init(&uart_console_buffer, uart_console_data_buffer, sizeof(uart_console_data_buffer));

    float    usartDiv;
    uint16_t divMantissa;
    uint16_t divFraction;
    // integer part
    usartDiv    = (float)(SystemCoreClock) / (bound * 16);
    divMantissa = usartDiv;
    // fractional part
    usartDiv    = (usartDiv - divMantissa) * 16;
    divFraction = usartDiv;
    if ((usartDiv - divFraction) > 0.5)
        divFraction += 1;

    // PA9  TX Alternate function output Push-pull 0xB
    // PA10 RX Floating input                      0x4

    RCC->APB2ENR |= 1 << 2;                           // enable PORTA clock
    RCC->APB2ENR |= 1 << 14;                          // enable uart clock
    GPIOA->CRH &= 0xFFFFF00F;                         //
    GPIOA->CRH |= 0x000004B0;                         // config GPIOA[9:10]
    RCC->APB2RSTR |= 1 << 14;                         // reset uart1
    RCC->APB2RSTR &= ~(1 << 14);                      //
    USART1->BRR = (divMantissa << 4) | divFraction;   // set bound
    USART1->CR1 |= 0x200C;                            // enable uart, tx, rx
    nvic_config(3, 3, USART1_IRQn);                   // init uart1 irq
    USART1->CR1 |= 0x1u << 5;                         // enable uart RXNEIE

    uart_console_inited = true;
    uart_putc('\n');
    uart_console_puts("uart init ok\n");
    return 0;
}


int uart_console_put(const uint8_t* data, uint16_t len)
{
    if (uart_console_inited) {
        uint16_t idx = 0;
        for (idx = 0; idx < len; idx++) {
            uart_putc(data[idx]);
        }
        return 0;
    }
    return -1;
}


int uart_console_puts(const char* str)
{
    if (uart_console_inited) {
        if (str != nullptr) {
            while (*str != '\0') {
                uart_putc(*str);
                str++;
            }
        }
        return 0;
    }
    return -1;
}

int uart_console_putc(char c)
{
    uart_putc(c);
    return 0;
}

int uart_console_getc(void)
{
    if (uart_console_inited) {
        if (!util_ringbuffer_empty(&uart_console_buffer)) {
            char c;
            util_ringbuffer_getc(&uart_console_buffer, &c);
            return (int)c;
        }
    }
    return -1;
}


void uart_console_isr(void)
{
    // tos_enter_isr();
    if (USART1->SR & (0x1u << 5)) {   // RXNE
        uint8_t data = USART1->DR;

        if (uart_console_inited) {
            if (!util_ringbuffer_full(&uart_console_buffer)) {
                util_ringbuffer_putc(&uart_console_buffer, data);
            }
        }
    }
    // tos_exit_isr();
}
