#ifndef I2C_HANDLER_H
#define I2C_HANDLER_H

#include "stm32f103xb.h"
#include <stdint.h>

// I2C result codes
#define I2C_OK       0
#define I2C_ERROR    1
#define I2C_TIMEOUT  2

// I2C function prototypes
void i2c_init(I2C_TypeDef *I2Cx, uint32_t speed);
uint8_t i2c_write(I2C_TypeDef *I2Cx, uint8_t device_addr, uint8_t *data, uint16_t length);
uint8_t i2c_read(I2C_TypeDef *I2Cx, uint8_t device_addr, uint8_t *data, uint16_t length);
uint8_t i2c_is_ready(I2C_TypeDef *I2Cx, uint8_t device_addr);
void i2c_scan(I2C_TypeDef *I2Cx);

#endif // I2C_HANDLER_H
