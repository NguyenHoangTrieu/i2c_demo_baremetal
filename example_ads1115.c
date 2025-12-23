/**
 * @file ads1115_example.c
 * @brief ADS1115 driver usage examples for STM32F103C8T6
 * 
 * Hardware setup:
 * - I2C1: PB6 (SCL), PB7 (SDA)
 * - USART1: PA9 (TX), PA10 (RX)
 * - ADS1115 ADDR pin: GND (address 0x48)
 * - ADS1115 ALERT/RDY pin: PA5 (optional, for interrupt)
 * - VDD: 3.3V
 * 
 * @author Your Name
 * @date 2025-12-23
 */

#include "stm32f103xb.h"
#include "i2c_handler.h"
#include "ads1115_handler.h"
#include "uart_handler.h"

// ============================================================================
// Global Variables
// ============================================================================
ADS1115_Handle_t ads_handle;
volatile uint8_t new_data_ready = 0;  // Flag for interrupt

// ============================================================================
// Delay Functions
// ============================================================================
void delay_ms(uint32_t ms) {
    for(uint32_t i = 0; i < ms * 8000; i++) {
        __NOP();
    }
}

// ============================================================================
// Helper function to print integer
// ============================================================================
void uart_print_int(int32_t value) {
    if (value < 0) {
        uart_write(USART1, '-');
        value = -value;
    }
    
    char buffer[12];
    int idx = 0;
    
    if (value == 0) {
        uart_write(USART1, '0');
        return;
    }
    
    while (value > 0) {
        buffer[idx++] = '0' + (value % 10);
        value /= 10;
    }
    
    for (int i = idx - 1; i >= 0; i--) {
        uart_write(USART1, buffer[i]);
    }
}

// ============================================================================
// Helper function to print voltage with unit
// ============================================================================
void uart_print_voltage_mv(float voltage) {
    uart_write_float(voltage);
    uart_log(" mV");
}

void uart_print_voltage_v(float voltage) {
    uart_write_float(voltage / 1000.0f);
    uart_log(" V");
}

// ============================================================================
// EXTI Interrupt Handler for ALERT/RDY pin
// ============================================================================
void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR5) {  // PA5
        EXTI->PR = EXTI_PR_PR5;    // Clear pending bit
        new_data_ready = 1;         // Set flag
    }
}

// ============================================================================
// EXTI Configuration for ALERT/RDY pin
// ============================================================================
void configure_exti_pa5(void) {
    // Enable AFIO clock
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    
    // Configure PA5 for EXTI
    AFIO->EXTICR[1] &= ~AFIO_EXTICR2_EXTI5;  // Clear
    AFIO->EXTICR[1] |= AFIO_EXTICR2_EXTI5_PA; // PA5
    
    // Configure EXTI5 line
    EXTI->IMR |= EXTI_IMR_MR5;     // Unmask interrupt
    EXTI->FTSR |= EXTI_FTSR_TR5;   // Falling edge trigger
    
    // Enable EXTI9_5 interrupt in NVIC
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_SetPriority(EXTI9_5_IRQn, 2);
}

// ============================================================================
// EXAMPLE 1: Basic Single-ended Reading
// ============================================================================
void example_01_single_ended_basic(void) {
    uart_log("\r\n=== Example 1: Single-ended Basic ===\r\n");
    
    int16_t raw_value;
    float voltage;
    
    // Read AIN0 (channel 0)
    if (ads1115_read_single_ended(&ads_handle, 0, &raw_value) == ADS1115_OK) {
        voltage = ads1115_raw_to_voltage(raw_value, ADS1115_PGA_4096);
        
        uart_log("AIN0 Raw: ");
        uart_print_int(raw_value);
        uart_log("\r\n");
        
        uart_log("AIN0 Voltage: ");
        uart_print_voltage_mv(voltage);
        uart_log("\r\n");
        
        uart_log("AIN0 Voltage: ");
        uart_print_voltage_v(voltage);
        uart_log("\r\n");
    } else {
        uart_log("Error reading AIN0\r\n");
    }
}

// ============================================================================
// EXAMPLE 2: Read All 4 Channels
// ============================================================================
void example_02_read_all_channels(void) {
    uart_log("\r\n=== Example 2: Read All 4 Channels ===\r\n");
    
    int16_t raw_values[4];
    float voltages[4];
    
    // Configure PGA and data rate
    ads1115_set_pga(&ads_handle, ADS1115_PGA_4096);  // ±4.096V
    ads1115_set_data_rate(&ads_handle, ADS1115_DR_128SPS);
    
    // Read all 4 channels
    for (uint8_t ch = 0; ch < 4; ch++) {
        if (ads1115_read_single_ended(&ads_handle, ch, &raw_values[ch]) == ADS1115_OK) {
            voltages[ch] = ads1115_raw_to_voltage(raw_values[ch], ADS1115_PGA_4096);
            
            uart_log("AIN");
            uart_print_int(ch);
            uart_log(": ");
            uart_print_int(raw_values[ch]);
            uart_log(" raw = ");
            uart_print_voltage_mv(voltages[ch]);
            uart_log(" = ");
            uart_print_voltage_v(voltages[ch]);
            uart_log("\r\n");
        } else {
            uart_log("Error reading AIN");
            uart_print_int(ch);
            uart_log("\r\n");
        }
        
        delay_ms(10);  // Small delay between readings
    }
}

// ============================================================================
// EXAMPLE 3: Differential Reading
// ============================================================================
void example_03_differential_reading(void) {
    uart_log("\r\n=== Example 3: Differential Reading ===\r\n");
    
    int16_t raw_value;
    float voltage;
    
    // Set PGA appropriate for differential voltage
    ads1115_set_pga(&ads_handle, ADS1115_PGA_2048);  // ±2.048V
    
    // Read AIN0 - AIN1 (differential)
    if (ads1115_read_differential(&ads_handle, 0, 1, &raw_value) == ADS1115_OK) {
        voltage = ads1115_raw_to_voltage(raw_value, ADS1115_PGA_2048);
        
        uart_log("AIN0-AIN1: ");
        uart_print_int(raw_value);
        uart_log(" raw = ");
        uart_print_voltage_mv(voltage);
        uart_log("\r\n");
        
        uart_log("(Positive value: AIN0 > AIN1)\r\n");
        uart_log("(Negative value: AIN0 < AIN1)\r\n");
    }
    
    delay_ms(100);
    
    // Read AIN2 - AIN3
    if (ads1115_read_differential(&ads_handle, 2, 3, &raw_value) == ADS1115_OK) {
        voltage = ads1115_raw_to_voltage(raw_value, ADS1115_PGA_2048);
        
        uart_log("AIN2-AIN3: ");
        uart_print_int(raw_value);
        uart_log(" raw = ");
        uart_print_voltage_mv(voltage);
        uart_log("\r\n");
    }
}

// ============================================================================
// EXAMPLE 4: High Resolution Reading (Low Noise)
// ============================================================================
void example_04_high_resolution(void) {
    uart_log("\r\n=== Example 4: High Resolution (Low Noise) ===\r\n");
    
    // Configure for high accuracy
    ads1115_set_pga(&ads_handle, ADS1115_PGA_0256);   // ±0.256V (resolution: 7.8125 µV)
    ads1115_set_data_rate(&ads_handle, ADS1115_DR_8SPS);  // Slow = less noise
    
    uart_log("Reading with highest resolution...\r\n");
    uart_log("PGA: ±0.256V, Data Rate: 8 SPS\r\n");
    uart_log("Resolution: 7.8125 µV per bit\r\n\r\n");
    
    // Take multiple readings and average
    int32_t sum = 0;
    const uint8_t samples = 10;
    
    for (uint8_t i = 0; i < samples; i++) {
        int16_t raw;
        if (ads1115_read_single_ended(&ads_handle, 0, &raw) == ADS1115_OK) {
            sum += raw;
            uart_log("Sample ");
            uart_print_int(i + 1);
            uart_log(": ");
            uart_print_int(raw);
            uart_log("\r\n");
        }
        delay_ms(200);  // Wait for slow data rate
    }
    
    int16_t average = sum / samples;
    float voltage = ads1115_raw_to_voltage(average, ADS1115_PGA_0256);
    
    uart_log("\r\nAverage: ");
    uart_print_int(average);
    uart_log(" raw = ");
    uart_print_voltage_mv(voltage);
    uart_log("\r\n");
}

// ============================================================================
// EXAMPLE 5: Continuous Mode with Polling
// ============================================================================
void example_05_continuous_mode_polling(void) {
    uart_log("\r\n=== Example 5: Continuous Mode (Polling) ===\r\n");
    
    int16_t raw_value;
    float voltage;
    
    // Configuration
    ads1115_set_pga(&ads_handle, ADS1115_PGA_4096);
    ads1115_set_data_rate(&ads_handle, ADS1115_DR_128SPS);  // 128 samples/sec
    
    // Start continuous mode on AIN0
    if (ads1115_start_continuous_mode(&ads_handle, 0) != ADS1115_OK) {
        uart_log("Error starting continuous mode\r\n");
        return;
    }
    
    uart_log("Continuous mode started on AIN0\r\n");
    uart_log("Reading 20 samples...\r\n\r\n");
    
    // Read 20 samples
    for (uint8_t i = 0; i < 20; i++) {
        delay_ms(10);  // Delay ~8ms for 128 SPS
        
        if (ads1115_read_continuous(&ads_handle, &raw_value) == ADS1115_OK) {
            voltage = ads1115_raw_to_voltage(raw_value, ADS1115_PGA_4096);
            
            uart_print_int(i + 1);
            uart_log(": ");
            uart_print_int(raw_value);
            uart_log(" raw = ");
            uart_print_voltage_mv(voltage);
            uart_log("\r\n");
        }
    }
    
    // Stop continuous mode
    ads1115_stop_continuous_mode(&ads_handle);
    uart_log("\r\nContinuous mode stopped\r\n");
}

// ============================================================================
// EXAMPLE 6: Continuous Mode with ALERT/RDY Interrupt
// ============================================================================
void example_06_continuous_with_interrupt(void) {
    uart_log("\r\n=== Example 6: Continuous Mode with Interrupt ===\r\n");
    
    int16_t raw_value;
    float voltage;
    uint16_t sample_count = 0;
    
    // Initialize ALERT/RDY pin (PA5) with interrupt
    ads1115_alert_gpio_init(&ads_handle, GPIOA, (1 << 5), ADS1115_ALERT_RDY_ENABLE);
    configure_exti_pa5();  // Configure EXTI interrupt
    
    // Configure ADS1115 for conversion ready mode
    ads1115_enable_alert_ready_pin(&ads_handle);
    
    // Configure and start continuous mode
    ads1115_set_pga(&ads_handle, ADS1115_PGA_4096);
    ads1115_set_data_rate(&ads_handle, ADS1115_DR_250SPS);  // 250 samples/sec
    
    if (ads1115_start_continuous_mode(&ads_handle, 0) != ADS1115_OK) {
        uart_log("Error starting continuous mode\r\n");
        return;
    }
    
    uart_log("Continuous mode with interrupt started\r\n");
    uart_log("ALERT/RDY pin: PA5 (falling edge interrupt)\r\n");
    uart_log("Collecting 50 samples...\r\n\r\n");
    
    new_data_ready = 0;
    
    // Collect 50 samples
    while (sample_count < 50) {
        // Wait for interrupt from ALERT/RDY pin
        if (new_data_ready) {
            new_data_ready = 0;  // Clear flag
            
            // Read data immediately when interrupt received
            if (ads1115_read_continuous(&ads_handle, &raw_value) == ADS1115_OK) {
                voltage = ads1115_raw_to_voltage(raw_value, ADS1115_PGA_4096);
                
                sample_count++;
                uart_print_int(sample_count);
                uart_log(": ");
                uart_print_int(raw_value);
                uart_log(" raw = ");
                uart_print_voltage_mv(voltage);
                uart_log("\r\n");
            }
        }
        
        // Can do other work here instead of waiting
        __WFI();  // Wait for interrupt (power saving)
    }
    
    // Stop continuous mode
    ads1115_stop_continuous_mode(&ads_handle);
    uart_log("\r\nContinuous mode stopped\r\n");
}

// ============================================================================
// EXAMPLE 7: Comparator - Overvoltage Detection
// ============================================================================
void example_07_comparator_overvoltage(void) {
    uart_log("\r\n=== Example 7: Comparator (Overvoltage Detection) ===\r\n");
    
    int16_t raw_value;
    float voltage;
    
    // Initialize ALERT pin
    ads1115_alert_gpio_init(&ads_handle, GPIOA, (1 << 5), ADS1115_ALERT_ENABLE);
    
    // Configure PGA
    ads1115_set_pga(&ads_handle, ADS1115_PGA_4096);  // ±4.096V
    ads1115_set_data_rate(&ads_handle, ADS1115_DR_128SPS);
    
    // Calculate threshold for 3.0V overvoltage
    // Formula: threshold = (voltage_mv * 32768) / full_scale_range_mv
    float threshold_voltage = 3000.0f;  // 3.0V in mV
    int16_t high_threshold = (int16_t)((threshold_voltage * 32768.0f) / 4096.0f);
    int16_t low_threshold = (int16_t)0x8000;  // Minimum value
    
    uart_log("Overvoltage detection threshold: 3.0V\r\n");
    uart_log("High threshold: ");
    uart_print_int(high_threshold);
    uart_log(" raw\r\n");
    
    // Set threshold
    ads1115_set_threshold(&ads_handle, low_threshold, high_threshold);
    
    // Configure comparator: traditional mode, active low, non-latching, assert after 1 conv
    ads1115_config_comparator(&ads_handle, 
                             ADS1115_COMP_TRADITIONAL,
                             ADS1115_COMP_POL_LOW,
                             ADS1115_COMP_NONLAT,
                             ADS1115_COMP_QUE_1);
    
    uart_log("Comparator configured. Monitoring AIN0...\r\n");
    uart_log("ALERT pin will go LOW when voltage > 3.0V\r\n\r\n");
    
    // Monitor voltage and ALERT pin
    for (uint8_t i = 0; i < 20; i++) {
        if (ads1115_read_single_ended(&ads_handle, 0, &raw_value) == ADS1115_OK) {
            voltage = ads1115_raw_to_voltage(raw_value, ADS1115_PGA_4096);
            uint8_t alert_status = ads1115_read_alert_pin(&ads_handle);
            
            uart_log("Voltage: ");
            uart_print_voltage_mv(voltage);
            uart_log(" | ALERT pin: ");
            uart_log(alert_status ? "HIGH" : "LOW (OVERVOLTAGE!)");
            uart_log("\r\n");
            
            if (!alert_status) {
                uart_log(">>> ALERT: Overvoltage detected! <<<\r\n");
            }
        }
        
        delay_ms(500);
    }
    
    // Disable comparator
    ads1115_config_comparator(&ads_handle, 
                             ADS1115_COMP_TRADITIONAL,
                             ADS1115_COMP_POL_LOW,
                             ADS1115_COMP_NONLAT,
                             ADS1115_COMP_QUE_DISABLE);
    uart_log("\r\nComparator disabled\r\n");
}

// ============================================================================
// EXAMPLE 8: Window Comparator
// ============================================================================
void example_08_window_comparator(void) {
    uart_log("\r\n=== Example 8: Window Comparator ===\r\n");
    
    int16_t raw_value;
    float voltage;
    
    // Initialize ALERT pin
    ads1115_alert_gpio_init(&ads_handle, GPIOA, (1 << 5), ADS1115_ALERT_ENABLE);
    
    // Configuration
    ads1115_set_pga(&ads_handle, ADS1115_PGA_4096);  // ±4.096V
    
    // Set window: 1.0V - 3.0V (alert if outside this range)
    float low_voltage = 1000.0f;   // 1.0V in mV
    float high_voltage = 3000.0f;  // 3.0V in mV
    
    int16_t low_threshold = (int16_t)((low_voltage * 32768.0f) / 4096.0f);
    int16_t high_threshold = (int16_t)((high_voltage * 32768.0f) / 4096.0f);
    
    uart_log("Window comparator: 1.0V - 3.0V\r\n");
    uart_log("ALERT if voltage < 1.0V OR voltage > 3.0V\r\n\r\n");
    
    uart_log("Low threshold: ");
    uart_print_int(low_threshold);
    uart_log(" raw (");
    uart_write_float(low_voltage);
    uart_log(" mV)\r\n");
    
    uart_log("High threshold: ");
    uart_print_int(high_threshold);
    uart_log(" raw (");
    uart_write_float(high_voltage);
    uart_log(" mV)\r\n\r\n");
    
    // Set threshold
    ads1115_set_threshold(&ads_handle, low_threshold, high_threshold);
    
    // Configure window comparator
    ads1115_config_comparator(&ads_handle, 
                             ADS1115_COMP_WINDOW,      // Window mode
                             ADS1115_COMP_POL_LOW,
                             ADS1115_COMP_NONLAT,
                             ADS1115_COMP_QUE_1);
    
    // Monitor
    for (uint8_t i = 0; i < 20; i++) {
        if (ads1115_read_single_ended(&ads_handle, 0, &raw_value) == ADS1115_OK) {
            voltage = ads1115_raw_to_voltage(raw_value, ADS1115_PGA_4096);
            uint8_t alert_status = ads1115_read_alert_pin(&ads_handle);
            
            uart_log("Voltage: ");
            uart_print_voltage_mv(voltage);
            uart_log(" | ALERT: ");
            uart_log(alert_status ? "HIGH (OK)" : "LOW");
            
            if (!alert_status) {
                if (voltage < low_voltage) {
                    uart_log(" - TOO LOW!\r\n");
                } else if (voltage > high_voltage) {
                    uart_log(" - TOO HIGH!\r\n");
                }
            } else {
                uart_log(" - In range\r\n");
            }
        }
        
        delay_ms(500);
    }
    
    // Disable comparator
    ads1115_config_comparator(&ads_handle, 
                             ADS1115_COMP_TRADITIONAL,
                             ADS1115_COMP_POL_LOW,
                             ADS1115_COMP_NONLAT,
                             ADS1115_COMP_QUE_DISABLE);
}

// ============================================================================
// EXAMPLE 9: Fast Sampling (High Speed)
// ============================================================================
void example_09_fast_sampling(void) {
    uart_log("\r\n=== Example 9: Fast Sampling (860 SPS) ===\r\n");
    
    int16_t raw_values[100];
    uint32_t sample_count = 0;
    
    // Configure for maximum speed
    ads1115_set_pga(&ads_handle, ADS1115_PGA_4096);
    ads1115_set_data_rate(&ads_handle, ADS1115_DR_860SPS);  // Maximum speed
    
    uart_log("Collecting 100 samples at 860 SPS...\r\n");
    
    // Start continuous mode
    ads1115_start_continuous_mode(&ads_handle, 0);
    
    delay_ms(5);  // Wait for first conversion
    
    // Collect samples as fast as possible
    while (sample_count < 100) {
        if (ads1115_read_continuous(&ads_handle, &raw_values[sample_count]) == ADS1115_OK) {
            sample_count++;
        }
        // Minimum delay (~1.16ms for 860 SPS)
        delay_ms(2);
    }
    
    ads1115_stop_continuous_mode(&ads_handle);
    
    // Display results
    uart_log("\r\nSamples collected:\r\n");
    for (uint8_t i = 0; i < 100; i++) {
        uart_print_int(i + 1);
        uart_log(": ");
        uart_print_int(raw_values[i]);
        uart_log("\r\n");
        
        if ((i + 1) % 10 == 0) {
            delay_ms(100);  // Pause every 10 samples for UART
        }
    }
    
    uart_log("\r\nTotal samples: ");
    uart_print_int(sample_count);
    uart_log("\r\n");
}

// ============================================================================
// EXAMPLE 10: Multi-Channel Scanning
// ============================================================================
void example_10_multi_channel_scan(void) {
    uart_log("\r\n=== Example 10: Multi-Channel Scanning ===\r\n");
    
    int16_t raw_values[4];
    float voltages[4];
    
    // Configuration
    ads1115_set_pga(&ads_handle, ADS1115_PGA_4096);
    ads1115_set_data_rate(&ads_handle, ADS1115_DR_250SPS);
    
    uart_log("Scanning all 4 channels continuously...\r\n");
    uart_log("Press any key to stop (if implemented)\r\n\r\n");
    
    // Scan 10 cycles
    for (uint8_t cycle = 0; cycle < 10; cycle++) {
        uart_log("--- Cycle ");
        uart_print_int(cycle + 1);
        uart_log(" ---\r\n");
        
        // Read all 4 channels
        for (uint8_t ch = 0; ch < 4; ch++) {
            if (ads1115_read_single_ended(&ads_handle, ch, &raw_values[ch]) == ADS1115_OK) {
                voltages[ch] = ads1115_raw_to_voltage(raw_values[ch], ADS1115_PGA_4096);
                
                uart_log("  AIN");
                uart_print_int(ch);
                uart_log(": ");
                uart_print_int(raw_values[ch]);
                uart_log(" raw = ");
                uart_print_voltage_mv(voltages[ch]);
                uart_log(" = ");
                uart_print_voltage_v(voltages[ch]);
                uart_log("\r\n");
            }
        }
        
        uart_log("\r\n");
        delay_ms(1000);  // 1 second between cycles
    }
}

// ============================================================================
// Main Function
// ============================================================================
int main(void) {
    // System initialization
    SystemInit();
    
    // UART initialization (for debug output)
    uart_init(USART1, 115200);  // 115200 baud
    uart_log("\r\n\r\n");
    uart_log("==================================================\r\n");
    uart_log("  ADS1115 Driver Examples for STM32F103C8T6\r\n");
    uart_log("==================================================\r\n");
    
    // I2C initialization
    i2c_init(I2C1, 100000);  // 100kHz I2C
    uart_log("I2C1 initialized (100kHz)\r\n");
    
    // ADS1115 initialization
    ads1115_init(&ads_handle, I2C1, ADS1115_ADDR_GND);
    uart_log("ADS1115 initialized (Address: 0x48)\r\n");
    
    // Check connection
    if (ads1115_is_connected(&ads_handle) == ADS1115_OK) {
        uart_log("ADS1115 connected successfully!\r\n");
    } else {
        uart_log("ERROR: ADS1115 not found!\r\n");
        uart_log("Check I2C connections and address\r\n");
        while(1);  // Stop here
    }
    
    // Set default configuration
    ads1115_set_default_config(&ads_handle);
    uart_log("ADS1115 default configuration set\r\n");
    
    delay_ms(1000);
    
    // ========================================================================
    // Run Examples
    // ========================================================================
    
    // Example 1: Basic single-ended reading
    example_01_single_ended_basic();
    delay_ms(2000);
    
    // Example 2: Read all channels
    example_02_read_all_channels();
    delay_ms(2000);
    
    // Example 3: Differential reading
    example_03_differential_reading();
    delay_ms(2000);
    
    // Example 4: High resolution (low noise)
    example_04_high_resolution();
    delay_ms(2000);
    
    // Example 5: Continuous mode with polling
    example_05_continuous_mode_polling();
    delay_ms(2000);
    
    // Example 6: Continuous mode with interrupt (requires PA5 connected to ALERT/RDY)
    // example_06_continuous_with_interrupt();
    // delay_ms(2000);
    
    // Example 7: Overvoltage detection (requires PA5 connected to ALERT/RDY)
    // example_07_comparator_overvoltage();
    // delay_ms(2000);
    
    // Example 8: Window comparator (requires PA5 connected to ALERT/RDY)
    // example_08_window_comparator();
    // delay_ms(2000);
    
    // Example 9: Fast sampling
    example_09_fast_sampling();
    delay_ms(2000);
    
    // Example 10: Multi-channel scanning
    example_10_multi_channel_scan();
    
    uart_log("\r\n");
    uart_log("==================================================\r\n");
    uart_log("  All examples completed!\r\n");
    uart_log("==================================================\r\n");
    
    // Main loop
    while(1) {
        // Your application code here
        delay_ms(1000);
    }
}
