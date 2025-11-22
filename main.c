#include "stm32f103xb.h"
#include "system_stm32f1xx.h"
#include "i2c_handler.h"
#include "uart_handler.h"

#define AHT20_ADDRESS     0x38
#define AHT20_CMD_INIT    0xBE
#define AHT20_CMD_TRIGGER 0xAC
#define AHT20_CMD_STATUS  0x71

// Delay function for 72MHz system clock
void delay_ms(uint32_t ms) {
    // 72MHz = 72 cycles per microsecond
    // For 1ms delay: 72000 cycles
    for (uint32_t i = 0; i < ms; i++) {
        for (uint32_t j = 0; j < 9000; j++) {
            __asm__("nop");
        }
    }
}

void aht20_init(void) {
    uint8_t init_cmd[3] = {AHT20_CMD_INIT, 0x08, 0x00};
    
    uart_log("Initializing AHT20...\r\n");
    
    // Wait for sensor to be ready
    delay_ms(40);
    
    // Send initialization command
    if (i2c_write(I2C1, AHT20_ADDRESS, init_cmd, 3) == I2C_OK) {
        uart_log("AHT20 initialized successfully.\r\n");
    } else {
        uart_log("AHT20 initialization failed!\r\n");
    }
    
    delay_ms(10);
}

void aht20_read_data(float *temperature, float *humidity) {
    uint8_t trigger_cmd[3] = {AHT20_CMD_TRIGGER, 0x33, 0x00};
    uint8_t data[7];
    
    // Trigger measurement
    i2c_write(I2C1, AHT20_ADDRESS, trigger_cmd, 3);
    
    // Wait for measurement to complete (80ms typical)
    delay_ms(100);
    
    // Read measurement data
    if (i2c_read(I2C1, AHT20_ADDRESS, data, 7) == I2C_OK) {
        // Extract humidity (20 bits)
        uint32_t humidity_raw = ((uint32_t)data[1] << 12) | 
                                ((uint32_t)data[2] << 4) | 
                                ((uint32_t)data[3] >> 4);
        
        // Extract temperature (20 bits)
        uint32_t temp_raw = (((uint32_t)data[3] & 0x0F) << 16) | 
                            ((uint32_t)data[4] << 8) | 
                            (uint32_t)data[5];
        
        // Calculate actual values
        *humidity = ((float)humidity_raw / 1048576.0f) * 100.0f;
        *temperature = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;
    }
}

int main(void) {
    float temperature = 0.0f;
    float humidity = 0.0f;
    
    // SystemInit() is already called by startup code before main()
    // System is now running at 72MHz
    
    // Initialize UART for debug output (115200 baud, APB2 = 72MHz)
    uart_init(USART1, 115200);
    
    uart_log("\r\n=================================\r\n");
    uart_log("STM32F103 AHT20 Sensor Reader\r\n");
    uart_log("System Clock: 72MHz\r\n");
    uart_log("=================================\r\n\r\n");
    
    // Initialize I2C (APB1 = 36MHz)
    i2c_init(I2C1, 100000);  // 100kHz
    
    // Scan I2C bus
    i2c_scan(I2C1);
    
    uart_log("\r\n");
    
    // Initialize AHT20 sensor
    aht20_init();
    
    uart_log("\r\nStarting measurements...\r\n\r\n");
    
    // Main loop
    while (1) {
        // Read sensor data
        aht20_read_data(&temperature, &humidity);
        
        // Log data via UART
        uart_log("Temperature: ");
        uart_write_float(temperature);
        uart_log(" C, Humidity: ");
        uart_write_float(humidity);
        uart_log(" %\r\n");
        
        // Wait 2 seconds before next reading
        delay_ms(2000);
    }
    
    return 0;
}
