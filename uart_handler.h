#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include "stm32f103xb.h"
#include <stdint.h>

// UART function prototypes
void uart_init(USART_TypeDef *USARTx, uint32_t baudrate);
void uart_write(USART_TypeDef *USARTx, uint8_t data);
uint8_t uart_read(USART_TypeDef *USARTx);
void uart_log(const char *str);
void uart_write_hex(uint8_t value);
void uart_write_float(float value);

#endif // UART_HANDLER_H
