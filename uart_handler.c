#include "uart_handler.h"

// Default UART for logging
static USART_TypeDef *log_uart = USART1;

static uint16_t compute_uart_brr(uint32_t periph_clk, uint32_t baudrate) {
    return ((periph_clk + (baudrate / 2U)) / baudrate);
}

void uart_init(USART_TypeDef *USARTx, uint32_t baudrate) {
    uint32_t periph_clk;
    
    if (USARTx == USART1) {
        // Enable USART1 and GPIOA clock
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN;
        
        // Configure PA9 (TX) as alternate function push-pull
        GPIOA->CRH &= ~(0xF << 4);
        GPIOA->CRH |= (0xB << 4);  // AF push-pull, 50MHz
        
        // Configure PA10 (RX) as input floating
        GPIOA->CRH &= ~(0xF << 8);
        GPIOA->CRH |= (0x4 << 8);  // Input floating
        
        periph_clk = 72000000;  // APB2 clock (72MHz at max system speed)
        log_uart = USART1;
    } else if (USARTx == USART2) {
        // Enable USART2 and GPIOA clock
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
        
        // Configure PA2 (TX) as alternate function push-pull
        GPIOA->CRL &= ~(0xF << 8);
        GPIOA->CRL |= (0xB << 8);  // AF push-pull, 50MHz
        
        // Configure PA3 (RX) as input floating
        GPIOA->CRL &= ~(0xF << 12);
        GPIOA->CRL |= (0x4 << 12);  // Input floating
        
        periph_clk = 36000000;  // APB1 clock (36MHz at max system speed)
    } else if (USARTx == USART3) {
        // Enable USART3 and GPIOB clock
        RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        
        // Configure PB10 (TX) as alternate function push-pull
        GPIOB->CRH &= ~(0xF << 8);
        GPIOB->CRH |= (0xB << 8);  // AF push-pull, 50MHz
        
        // Configure PB11 (RX) as input floating
        GPIOB->CRH &= ~(0xF << 12);
        GPIOB->CRH |= (0x4 << 12);  // Input floating
        
        periph_clk = 36000000;  // APB1 clock (36MHz at max system speed)
    }
    
    // Set baudrate
    USARTx->BRR = compute_uart_brr(periph_clk, baudrate);
    
    // Enable transmitter and receiver
    USARTx->CR1 = USART_CR1_TE | USART_CR1_RE;
    
    // Enable USART
    USARTx->CR1 |= USART_CR1_UE;
}

// Rest of uart_handler.c remains the same...
void uart_write(USART_TypeDef *USARTx, uint8_t data) {
    while (!(USARTx->SR & USART_SR_TXE));
    USARTx->DR = data;
}

uint8_t uart_read(USART_TypeDef *USARTx) {
    while (!(USARTx->SR & USART_SR_RXNE));
    return (uint8_t)(USARTx->DR & 0xFF);
}

void uart_log(const char *str) {
    while (*str) {
        uart_write(log_uart, *str++);
    }
}

void uart_write_hex(uint8_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    uart_write(log_uart, hex_chars[(value >> 4) & 0x0F]);
    uart_write(log_uart, hex_chars[value & 0x0F]);
}

void uart_write_float(float value) {
    int32_t int_part = (int32_t)value;
    uint32_t frac_part = (uint32_t)((value - int_part) * 100);
    
    if (int_part < 0) {
        uart_write(log_uart, '-');
        int_part = -int_part;
    }
    
    char buffer[10];
    int idx = 0;
    
    if (int_part == 0) {
        buffer[idx++] = '0';
    } else {
        while (int_part > 0) {
            buffer[idx++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }
    
    for (int i = idx - 1; i >= 0; i--) {
        uart_write(log_uart, buffer[i]);
    }
    
    uart_write(log_uart, '.');
    uart_write(log_uart, '0' + (frac_part / 10));
    uart_write(log_uart, '0' + (frac_part % 10));
}
