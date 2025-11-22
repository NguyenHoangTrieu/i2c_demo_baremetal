#include "i2c_handler.h"
#include "uart_handler.h"

// Internal timeout counter value (different from return code)
#define I2C_TIMEOUT_COUNT 10000

static void i2c_start(I2C_TypeDef *I2Cx) {
    I2Cx->CR1 |= I2C_CR1_START;
    while (!(I2Cx->SR1 & I2C_SR1_SB));
}

static void i2c_stop(I2C_TypeDef *I2Cx) {
    I2Cx->CR1 |= I2C_CR1_STOP;
}

static uint8_t i2c_send_address(I2C_TypeDef *I2Cx, uint8_t address, uint8_t direction) {
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    I2Cx->DR = (address << 1) | direction;
    
    while (!(I2Cx->SR1 & I2C_SR1_ADDR)) {
        if (--timeout == 0) return I2C_TIMEOUT;
        if (I2Cx->SR1 & I2C_SR1_AF) {
            I2Cx->SR1 &= ~I2C_SR1_AF;
            i2c_stop(I2Cx);
            return I2C_ERROR;
        }
    }
    
    // Clear ADDR flag by reading SR1 then SR2
    (void)I2Cx->SR1;
    (void)I2Cx->SR2;
    
    return I2C_OK;
}

void i2c_init(I2C_TypeDef *I2Cx, uint32_t speed) {
    // Enable I2C clock
    if (I2Cx == I2C1) {
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        
        // Configure PB6 (SCL) and PB7 (SDA) as alternate function open-drain
        GPIOB->CRL &= ~(0xFF << 24);
        GPIOB->CRL |= (0xEE << 24);
    } else if (I2Cx == I2C2) {
        RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        
        // Configure PB10 (SCL) and PB11 (SDA) as alternate function open-drain
        GPIOB->CRH &= ~(0xFF << 8);
        GPIOB->CRH |= (0xEE << 8);
    }
    
    // Reset I2C
    I2Cx->CR1 |= I2C_CR1_SWRST;
    I2Cx->CR1 &= ~I2C_CR1_SWRST;
    
    // Configure I2C for APB1 = 36MHz
    I2Cx->CR2 = 36;  // 36MHz peripheral clock
    
    if (speed == 100000) {
        // Standard mode (100kHz): T_high = T_low = CCR * T_PCLK1
        // CCR = PCLK1 / (2 * I2C_speed) = 36MHz / (2 * 100kHz) = 180
        I2Cx->CCR = 180;
        // TRISE = (1000ns / 27.7ns) + 1 = 37
        I2Cx->TRISE = 37;
    } else {
        // Fast mode (400kHz)
        // CCR = PCLK1 / (3 * I2C_speed) = 36MHz / (3 * 400kHz) = 30
        I2Cx->CCR = I2C_CCR_FS | 30;
        // TRISE = (300ns / 27.7ns) + 1 = 12
        I2Cx->TRISE = 12;
    }
    
    // Enable I2C
    I2Cx->CR1 |= I2C_CR1_PE;
}

uint8_t i2c_write(I2C_TypeDef *I2Cx, uint8_t device_addr, uint8_t *data, uint16_t length) {
    uint32_t timeout;
    
    // Wait until bus is not busy
    timeout = I2C_TIMEOUT_COUNT;
    while (I2Cx->SR2 & I2C_SR2_BUSY) {
        if (--timeout == 0) return I2C_TIMEOUT;
    }
    
    // Generate START condition
    i2c_start(I2Cx);
    
    // Send device address for write
    if (i2c_send_address(I2Cx, device_addr, 0) != I2C_OK) {
        return I2C_ERROR;
    }
    
    // Send data
    for (uint16_t i = 0; i < length; i++) {
        timeout = I2C_TIMEOUT_COUNT;
        while (!(I2Cx->SR1 & I2C_SR1_TXE)) {
            if (--timeout == 0) {
                i2c_stop(I2Cx);
                return I2C_TIMEOUT;
            }
        }
        I2Cx->DR = data[i];
    }
    
    // Wait for transfer to finish
    timeout = I2C_TIMEOUT_COUNT;
    while (!(I2Cx->SR1 & I2C_SR1_BTF)) {
        if (--timeout == 0) {
            i2c_stop(I2Cx);
            return I2C_TIMEOUT;
        }
    }
    
    // Generate STOP condition
    i2c_stop(I2Cx);
    
    return I2C_OK;
}

uint8_t i2c_read(I2C_TypeDef *I2Cx, uint8_t device_addr, uint8_t *data, uint16_t length) {
    uint32_t timeout;
    
    // Wait until bus is not busy
    timeout = I2C_TIMEOUT_COUNT;
    while (I2Cx->SR2 & I2C_SR2_BUSY) {
        if (--timeout == 0) return I2C_TIMEOUT;
    }
    
    // Enable ACK
    I2Cx->CR1 |= I2C_CR1_ACK;
    
    // Generate START condition
    i2c_start(I2Cx);
    
    // Send device address for read
    if (i2c_send_address(I2Cx, device_addr, 1) != I2C_OK) {
        return I2C_ERROR;
    }
    
    // Read data
    for (uint16_t i = 0; i < length; i++) {
        if (i == length - 1) {
            // Last byte: disable ACK and generate STOP
            I2Cx->CR1 &= ~I2C_CR1_ACK;
            i2c_stop(I2Cx);
        }
        
        timeout = I2C_TIMEOUT_COUNT;
        while (!(I2Cx->SR1 & I2C_SR1_RXNE)) {
            if (--timeout == 0) {
                i2c_stop(I2Cx);
                return I2C_TIMEOUT;
            }
        }
        data[i] = I2Cx->DR;
    }
    
    return I2C_OK;
}

uint8_t i2c_is_ready(I2C_TypeDef *I2Cx, uint8_t device_addr) {
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    
    while (I2Cx->SR2 & I2C_SR2_BUSY) {
        if (--timeout == 0) return I2C_ERROR;
    }
    
    i2c_start(I2Cx);
    
    uint8_t result = i2c_send_address(I2Cx, device_addr, 0);
    
    i2c_stop(I2Cx);
    
    return (result == I2C_OK) ? I2C_OK : I2C_ERROR;
}

void i2c_scan(I2C_TypeDef *I2Cx) {
    uart_log("I2C Bus Scan:\r\n");
    
    for (uint8_t addr = 1; addr < 128; addr++) {
        if (i2c_is_ready(I2Cx, addr) == I2C_OK) {
            uart_log("Device found at address: 0x");
            uart_write_hex(addr);
            uart_log("\r\n");
        }
    }
    
    uart_log("I2C scan complete.\r\n");
}
