#include "ads1115_handler.h"
#include <stddef.h>  // For NULL

// Default configuration: Single-shot, ±2.048V, 128SPS, comparator disabled
#define ADS1115_DEFAULT_CONFIG  (ADS1115_OS_IDLE | \
                                 ADS1115_MUX_AIN0_GND | \
                                 ADS1115_PGA_2048 | \
                                 ADS1115_MODE_SINGLE | \
                                 ADS1115_DR_128SPS | \
                                 ADS1115_COMP_TRADITIONAL | \
                                 ADS1115_COMP_POL_LOW | \
                                 ADS1115_COMP_NONLAT | \
                                 ADS1115_COMP_QUE_DISABLE)

// Voltage ranges for each PGA setting (in mV)
static const float PGA_RANGES[] = {
    6144.0f,  // ±6.144V
    4096.0f,  // ±4.096V
    2048.0f,  // ±2.048V
    1024.0f,  // ±1.024V
    512.0f,   // ±0.512V
    256.0f    // ±0.256V
};

// Initialize ADS1115 handle
void ads1115_init(ADS1115_Handle_t *handle, I2C_TypeDef *i2c, uint8_t address) {
    handle->i2c = i2c;
    handle->address = address;
    handle->config = ADS1115_DEFAULT_CONFIG;
    handle->alert_port = NULL;
    handle->alert_pin = 0;
    handle->alert_mode = ADS1115_ALERT_DISABLE;
}

// Set default configuration
void ads1115_set_default_config(ADS1115_Handle_t *handle) {
    handle->config = ADS1115_DEFAULT_CONFIG;
    ads1115_set_config(handle, handle->config);
}

// Write to ADS1115 register
uint8_t ads1115_write_register(ADS1115_Handle_t *handle, uint8_t reg, uint16_t value) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (value >> 8) & 0xFF;  // MSB
    data[2] = value & 0xFF;          // LSB
    
    return i2c_write(handle->i2c, handle->address, data, 3);
}

// Read from ADS1115 register
uint8_t ads1115_read_register(ADS1115_Handle_t *handle, uint8_t reg, uint16_t *value) {
    uint8_t result;
    uint8_t data[2];
    
    // Write register pointer
    result = i2c_write(handle->i2c, handle->address, &reg, 1);
    if (result != I2C_OK) {
        return result;
    }
    
    // Read register value
    result = i2c_read(handle->i2c, handle->address, data, 2);
    if (result != I2C_OK) {
        return result;
    }
    
    *value = ((uint16_t)data[0] << 8) | data[1];
    return ADS1115_OK;
}

// Set configuration register
uint8_t ads1115_set_config(ADS1115_Handle_t *handle, uint16_t config) {
    handle->config = config;
    return ads1115_write_register(handle, ADS1115_REG_CONFIG, config);
}

// Get configuration register
uint8_t ads1115_get_config(ADS1115_Handle_t *handle, uint16_t *config) {
    return ads1115_read_register(handle, ADS1115_REG_CONFIG, config);
}

// Set input multiplexer
uint8_t ads1115_set_mux(ADS1115_Handle_t *handle, uint16_t mux) {
    handle->config = (handle->config & 0x8FFF) | (mux & 0x7000);
    return ads1115_set_config(handle, handle->config);
}

// Set programmable gain amplifier
uint8_t ads1115_set_pga(ADS1115_Handle_t *handle, uint16_t pga) {
    handle->config = (handle->config & 0xF1FF) | (pga & 0x0E00);
    return ads1115_set_config(handle, handle->config);
}

// Set operating mode
uint8_t ads1115_set_mode(ADS1115_Handle_t *handle, uint16_t mode) {
    handle->config = (handle->config & 0xFEFF) | (mode & 0x0100);
    return ads1115_set_config(handle, handle->config);
}

// Set data rate
uint8_t ads1115_set_data_rate(ADS1115_Handle_t *handle, uint16_t rate) {
    handle->config = (handle->config & 0xFF1F) | (rate & 0x00E0);
    return ads1115_set_config(handle, handle->config);
}

// Start conversion (single-shot mode)
uint8_t ads1115_start_conversion(ADS1115_Handle_t *handle) {
    uint16_t config = handle->config | ADS1115_OS_START;
    return ads1115_write_register(handle, ADS1115_REG_CONFIG, config);
}

// Check if conversion is ready
uint8_t ads1115_is_conversion_ready(ADS1115_Handle_t *handle) {
    uint16_t config;
    uint8_t result = ads1115_get_config(handle, &config);
    
    if (result != ADS1115_OK) {
        return ADS1115_ERROR;
    }
    
    // In single-shot mode, OS bit goes high when conversion is complete
    if (config & ADS1115_CONFIG_OS_MASK) {
        return ADS1115_OK;
    }
    
    return ADS1115_NOT_READY;
}

// Read conversion result
uint8_t ads1115_read_conversion(ADS1115_Handle_t *handle, int16_t *value) {
    uint16_t raw_value;
    uint8_t result = ads1115_read_register(handle, ADS1115_REG_CONVERSION, &raw_value);
    
    if (result == ADS1115_OK) {
        *value = (int16_t)raw_value;
    }
    
    return result;
}

// Read single-ended input
uint8_t ads1115_read_single_ended(ADS1115_Handle_t *handle, uint8_t channel, int16_t *value) {
    uint16_t mux_config;
    uint8_t result;
    uint32_t timeout = 100000;
    
    // Set MUX for single-ended input
    switch (channel) {
        case 0: mux_config = ADS1115_MUX_AIN0_GND; break;
        case 1: mux_config = ADS1115_MUX_AIN1_GND; break;
        case 2: mux_config = ADS1115_MUX_AIN2_GND; break;
        case 3: mux_config = ADS1115_MUX_AIN3_GND; break;
        default: return ADS1115_ERROR;
    }
    
    // Configure and start conversion
    result = ads1115_set_mux(handle, mux_config);
    if (result != ADS1115_OK) return result;
    
    result = ads1115_start_conversion(handle);
    if (result != ADS1115_OK) return result;
    
    // Wait for conversion to complete
    while (timeout--) {
        if (ads1115_is_conversion_ready(handle) == ADS1115_OK) {
            return ads1115_read_conversion(handle, value);
        }
    }
    
    return ADS1115_TIMEOUT;
}

// Read differential input
uint8_t ads1115_read_differential(ADS1115_Handle_t *handle, uint8_t pos_channel, uint8_t neg_channel, int16_t *value) {
    uint16_t mux_config;
    uint8_t result;
    uint32_t timeout = 100000;
    
    // Set MUX for differential input
    if (pos_channel == 0 && neg_channel == 1) {
        mux_config = ADS1115_MUX_AIN0_AIN1;
    } else if (pos_channel == 0 && neg_channel == 3) {
        mux_config = ADS1115_MUX_AIN0_AIN3;
    } else if (pos_channel == 1 && neg_channel == 3) {
        mux_config = ADS1115_MUX_AIN1_AIN3;
    } else if (pos_channel == 2 && neg_channel == 3) {
        mux_config = ADS1115_MUX_AIN2_AIN3;
    } else {
        return ADS1115_ERROR;
    }
    
    // Configure and start conversion
    result = ads1115_set_mux(handle, mux_config);
    if (result != ADS1115_OK) return result;
    
    result = ads1115_start_conversion(handle);
    if (result != ADS1115_OK) return result;
    
    // Wait for conversion to complete
    while (timeout--) {
        if (ads1115_is_conversion_ready(handle) == ADS1115_OK) {
            return ads1115_read_conversion(handle, value);
        }
    }
    
    return ADS1115_TIMEOUT;
}

// Convert raw ADC value to voltage (in mV)
float ads1115_raw_to_voltage(int16_t raw_value, uint16_t pga) {
    uint8_t pga_index = (pga >> 9) & 0x07;
    if (pga_index > 5) pga_index = 5;
    
    float voltage_range = PGA_RANGES[pga_index];
    return (raw_value * voltage_range) / 32768.0f;
}

// Set threshold values for comparator
uint8_t ads1115_set_threshold(ADS1115_Handle_t *handle, int16_t low_thresh, int16_t high_thresh) {
    uint8_t result;
    
    result = ads1115_write_register(handle, ADS1115_REG_LO_THRESH, (uint16_t)low_thresh);
    if (result != ADS1115_OK) return result;
    
    result = ads1115_write_register(handle, ADS1115_REG_HI_THRESH, (uint16_t)high_thresh);
    return result;
}

// Get threshold values
uint8_t ads1115_get_threshold(ADS1115_Handle_t *handle, int16_t *low_thresh, int16_t *high_thresh) {
    uint8_t result;
    uint16_t value;
    
    result = ads1115_read_register(handle, ADS1115_REG_LO_THRESH, &value);
    if (result != ADS1115_OK) return result;
    *low_thresh = (int16_t)value;
    
    result = ads1115_read_register(handle, ADS1115_REG_HI_THRESH, &value);
    if (result != ADS1115_OK) return result;
    *high_thresh = (int16_t)value;
    
    return ADS1115_OK;
}

// Configure comparator
uint8_t ads1115_config_comparator(ADS1115_Handle_t *handle, uint16_t comp_mode, 
                                  uint16_t comp_pol, uint16_t comp_lat, uint16_t comp_que) {
    // Clear comparator bits
    handle->config &= 0xFFE0;
    
    // Set new comparator configuration
    handle->config |= (comp_mode & 0x0010) | (comp_pol & 0x0008) | 
                      (comp_lat & 0x0004) | (comp_que & 0x0003);
    
    return ads1115_set_config(handle, handle->config);
}

// Initialize ALERT/RDY GPIO pin
void ads1115_alert_gpio_init(ADS1115_Handle_t *handle, GPIO_TypeDef *port, uint16_t pin, ADS1115_AlertMode_t mode) {
    handle->alert_port = port;
    handle->alert_pin = pin;
    handle->alert_mode = mode;
    
    // Enable GPIO clock
    if (port == GPIOA) {
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    } else if (port == GPIOB) {
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    } else if (port == GPIOC) {
        RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    }
    
    // Configure pin as input with pull-up
    uint8_t pin_pos = 0;
    while (!(pin & (1 << pin_pos))) pin_pos++;
    
    if (pin_pos < 8) {
        // Configure CRL register
        port->CRL &= ~(0xF << (pin_pos * 4));
        port->CRL |= (0x8 << (pin_pos * 4));  // Input with pull-up/pull-down
        port->ODR |= (1 << pin_pos);           // Enable pull-up
    } else {
        // Configure CRH register
        pin_pos -= 8;
        port->CRH &= ~(0xF << (pin_pos * 4));
        port->CRH |= (0x8 << (pin_pos * 4));  // Input with pull-up/pull-down
        port->ODR |= (1 << (pin_pos + 8));     // Enable pull-up
    }
}

// Read ALERT/RDY pin state
uint8_t ads1115_read_alert_pin(ADS1115_Handle_t *handle) {
    if (handle->alert_port == NULL) {
        return ADS1115_ERROR;
    }
    
    return (handle->alert_port->IDR & handle->alert_pin) ? 1 : 0;
}

// Enable ALERT/RDY pin as conversion ready signal
void ads1115_enable_alert_ready_pin(ADS1115_Handle_t *handle) {
    // Set thresholds to trigger on every conversion
    // High threshold = 0x8000 (max negative)
    // Low threshold = 0x7FFF (max positive)
    ads1115_set_threshold(handle, 0x7FFF, (int16_t)0x8000);
    
    // Configure comparator for conversion ready mode
    ads1115_config_comparator(handle, ADS1115_COMP_TRADITIONAL, 
                              ADS1115_COMP_POL_LOW, 
                              ADS1115_COMP_NONLAT, 
                              ADS1115_COMP_QUE_1);
}

// Clear ALERT by reading conversion register
void ads1115_clear_alert(ADS1115_Handle_t *handle) {
    int16_t dummy;
    ads1115_read_conversion(handle, &dummy);
}

// Start continuous conversion mode
uint8_t ads1115_start_continuous_mode(ADS1115_Handle_t *handle, uint8_t channel) {
    uint16_t mux_config;
    uint8_t result;
    
    // Set MUX for selected channel
    switch (channel) {
        case 0: mux_config = ADS1115_MUX_AIN0_GND; break;
        case 1: mux_config = ADS1115_MUX_AIN1_GND; break;
        case 2: mux_config = ADS1115_MUX_AIN2_GND; break;
        case 3: mux_config = ADS1115_MUX_AIN3_GND; break;
        default: return ADS1115_ERROR;
    }
    
    result = ads1115_set_mux(handle, mux_config);
    if (result != ADS1115_OK) return result;
    
    // Set continuous mode
    result = ads1115_set_mode(handle, ADS1115_MODE_CONTINUOUS);
    if (result != ADS1115_OK) return result;
    
    // Start conversion
    return ads1115_start_conversion(handle);
}

// Stop continuous mode (return to single-shot)
uint8_t ads1115_stop_continuous_mode(ADS1115_Handle_t *handle) {
    return ads1115_set_mode(handle, ADS1115_MODE_SINGLE);
}

// Read value in continuous mode
uint8_t ads1115_read_continuous(ADS1115_Handle_t *handle, int16_t *value) {
    return ads1115_read_conversion(handle, value);
}

// Check if ADS1115 is connected
uint8_t ads1115_is_connected(ADS1115_Handle_t *handle) {
    return i2c_is_ready(handle->i2c, handle->address);
}

// Reset ADS1115 to default configuration
uint8_t ads1115_reset(ADS1115_Handle_t *handle) {
    ads1115_set_default_config(handle);
    return ads1115_set_threshold(handle, (int16_t)0x8000, 0x7FFF);
}
