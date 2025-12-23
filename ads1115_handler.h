#ifndef ADS1115_HANDLER_H
#define ADS1115_HANDLER_H

#include "stm32f103xb.h"
#include "i2c_handler.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * ADS1115 - 16-bit ADC with I2C Interface
 * 
 * Key Features:
 * - 16-bit resolution ADC
 * - 4 single-ended or 2 differential inputs
 * - Programmable Gain Amplifier (PGA): ±0.256V to ±6.144V
 * - Data rate: 8 to 860 samples per second
 * - Internal voltage reference
 * - Comparator with ALERT/RDY pin
 * - Single-shot or Continuous conversion mode
 * 
 * I2C Address: 0x48 (ADDR to GND), 0x49 (ADDR to VDD),
 *              0x4A (ADDR to SDA), 0x4B (ADDR to SCL)
 *****************************************************************************/

// ============================================================================
// ADS1115 I2C Addresses
// ============================================================================
#define ADS1115_ADDR_GND    0x48  // ADDR pin connected to GND (default)
#define ADS1115_ADDR_VDD    0x49  // ADDR pin connected to VDD
#define ADS1115_ADDR_SDA    0x4A  // ADDR pin connected to SDA
#define ADS1115_ADDR_SCL    0x4B  // ADDR pin connected to SCL

// ============================================================================
// ADS1115 Register Pointers
// ============================================================================
#define ADS1115_REG_CONVERSION  0x00  // Conversion register (holds ADC result)
#define ADS1115_REG_CONFIG      0x01  // Config register (ADC configuration)
#define ADS1115_REG_LO_THRESH   0x02  // Low threshold register
#define ADS1115_REG_HI_THRESH   0x03  // High threshold register

// ============================================================================
// Config Register Bits
// ============================================================================

// OS: Operational Status/Single-shot conversion start (bit 15)
#define ADS1115_OS_IDLE         0x0000  // No operation (write has no effect)
#define ADS1115_OS_START        0x8000  // Start single conversion

// MUX: Input Multiplexer Configuration (bits 14-12)
// Select input channel: single-ended (vs GND) or differential
#define ADS1115_MUX_AIN0_AIN1   0x0000  // Differential: AIN0(+) - AIN1(-) [default]
#define ADS1115_MUX_AIN0_AIN3   0x1000  // Differential: AIN0(+) - AIN3(-)
#define ADS1115_MUX_AIN1_AIN3   0x2000  // Differential: AIN1(+) - AIN3(-)
#define ADS1115_MUX_AIN2_AIN3   0x3000  // Differential: AIN2(+) - AIN3(-)
#define ADS1115_MUX_AIN0_GND    0x4000  // Single-ended: AIN0 vs GND
#define ADS1115_MUX_AIN1_GND    0x5000  // Single-ended: AIN1 vs GND
#define ADS1115_MUX_AIN2_GND    0x6000  // Single-ended: AIN2 vs GND
#define ADS1115_MUX_AIN3_GND    0x7000  // Single-ended: AIN3 vs GND

// PGA: Programmable Gain Amplifier Configuration (bits 11-9)
// Configure gain and full-scale range
// Note: Input voltage must not exceed VDD+0.3V and GND-0.3V
#define ADS1115_PGA_6144        0x0000  // FSR = ±6.144V (1 bit = 0.1875mV)
#define ADS1115_PGA_4096        0x0200  // FSR = ±4.096V (1 bit = 0.125mV)
#define ADS1115_PGA_2048        0x0400  // FSR = ±2.048V (1 bit = 0.0625mV) [default]
#define ADS1115_PGA_1024        0x0600  // FSR = ±1.024V (1 bit = 0.03125mV)
#define ADS1115_PGA_0512        0x0800  // FSR = ±0.512V (1 bit = 0.015625mV)
#define ADS1115_PGA_0256        0x0A00  // FSR = ±0.256V (1 bit = 0.0078125mV)

// MODE: Device Operating Mode (bit 8)
#define ADS1115_MODE_CONTINUOUS 0x0000  // Continuous conversion mode
#define ADS1115_MODE_SINGLE     0x0100  // Single-shot mode [default]

// DR: Data Rate (bits 7-5)
// ADC sampling rate (samples per second)
#define ADS1115_DR_8SPS         0x0000  // 8 samples/second
#define ADS1115_DR_16SPS        0x0020  // 16 SPS
#define ADS1115_DR_32SPS        0x0040  // 32 SPS
#define ADS1115_DR_64SPS        0x0060  // 64 SPS
#define ADS1115_DR_128SPS       0x0080  // 128 SPS [default]
#define ADS1115_DR_250SPS       0x00A0  // 250 SPS
#define ADS1115_DR_475SPS       0x00C0  // 475 SPS
#define ADS1115_DR_860SPS       0x00E0  // 860 SPS (maximum speed)

// COMP_MODE: Comparator Mode (bit 4)
#define ADS1115_COMP_TRADITIONAL 0x0000  // Traditional comparator [default]
#define ADS1115_COMP_WINDOW      0x0010  // Window comparator

// COMP_POL: Comparator Polarity (bit 3)
// Polarity of ALERT/RDY pin
#define ADS1115_COMP_POL_LOW     0x0000  // Active low [default]
#define ADS1115_COMP_POL_HIGH    0x0008  // Active high

// COMP_LAT: Latching Comparator (bit 2)
#define ADS1115_COMP_NONLAT      0x0000  // Non-latching comparator [default]
#define ADS1115_COMP_LAT         0x0004  // Latching comparator (must read data to clear)

// COMP_QUE: Comparator Queue and Disable (bits 1-0)
// Number of conversions exceeding threshold before asserting ALERT
#define ADS1115_COMP_QUE_1       0x0000  // Assert after 1 conversion
#define ADS1115_COMP_QUE_2       0x0001  // Assert after 2 conversions
#define ADS1115_COMP_QUE_4       0x0002  // Assert after 4 conversions
#define ADS1115_COMP_QUE_DISABLE 0x0003  // Disable comparator [default]

// Conversion ready mask
#define ADS1115_CONFIG_OS_MASK   0x8000

// ============================================================================
// Return/Result Codes
// ============================================================================
#define ADS1115_OK               0  // Success
#define ADS1115_ERROR            1  // I2C error or invalid parameter
#define ADS1115_TIMEOUT          2  // Timeout waiting for conversion
#define ADS1115_NOT_READY        3  // Conversion not yet complete

// ============================================================================
// ALERT/RDY Pin Configuration
// ============================================================================
typedef enum {
    ADS1115_ALERT_DISABLE = 0,      // Don't use ALERT pin
    ADS1115_ALERT_ENABLE,            // Use as comparator output
    ADS1115_ALERT_RDY_ENABLE         // Use as conversion ready signal
} ADS1115_AlertMode_t;

// ============================================================================
// ADS1115 Configuration Structure
// ============================================================================
typedef struct {
    I2C_TypeDef *i2c;               // I2C peripheral (I2C1 or I2C2)
    uint8_t address;                // I2C address of ADS1115 (0x48-0x4B)
    uint16_t config;                // Current config register value
    GPIO_TypeDef *alert_port;       // GPIO port for ALERT/RDY pin
    uint16_t alert_pin;             // GPIO pin for ALERT/RDY (GPIO_PIN_x)
    ADS1115_AlertMode_t alert_mode; // ALERT pin operating mode
} ADS1115_Handle_t;

// ============================================================================
// Function Prototypes
// ============================================================================

/**
 * @brief Initialize ADS1115 handle structure
 * 
 * @param handle    Pointer to ADS1115_Handle_t structure
 * @param i2c       I2C peripheral (I2C1 or I2C2)
 * @param address   I2C address of ADS1115 (0x48-0x4B)
 * 
 * @note This function only initializes the structure, not the hardware.
 *       Call i2c_init() first and ads1115_set_default_config() after.
 */
void ads1115_init(ADS1115_Handle_t *handle, I2C_TypeDef *i2c, uint8_t address);

/**
 * @brief Set ADS1115 to default configuration
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * 
 * Default configuration:
 * - Single-shot mode
 * - AIN0 single-ended
 * - ±2.048V range
 * - 128 SPS
 * - Comparator disabled
 */
void ads1115_set_default_config(ADS1115_Handle_t *handle);

// ============================================================================
// Low-level Register Access
// ============================================================================

/**
 * @brief Write value to ADS1115 register
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param reg       Register address (0x00-0x03)
 * @param value     16-bit value to write
 * @return uint8_t  ADS1115_OK if successful
 */
uint8_t ads1115_write_register(ADS1115_Handle_t *handle, uint8_t reg, uint16_t value);

/**
 * @brief Read value from ADS1115 register
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param reg       Register address (0x00-0x03)
 * @param value     Pointer to store read value
 * @return uint8_t  ADS1115_OK if successful
 */
uint8_t ads1115_read_register(ADS1115_Handle_t *handle, uint8_t reg, uint16_t *value);

// ============================================================================
// Configuration Functions
// ============================================================================

/**
 * @brief Write complete config register
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param config    16-bit config value (combination of ADS1115_xxx defines)
 * @return uint8_t  ADS1115_OK if successful
 * 
 * @example
 * uint16_t config = ADS1115_MUX_AIN0_GND | ADS1115_PGA_4096 | 
 *                   ADS1115_MODE_SINGLE | ADS1115_DR_250SPS |
 *                   ADS1115_COMP_QUE_DISABLE;
 * ads1115_set_config(&handle, config);
 */
uint8_t ads1115_set_config(ADS1115_Handle_t *handle, uint16_t config);

/**
 * @brief Read current config register
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param config    Pointer to store config value
 * @return uint8_t  ADS1115_OK if successful
 */
uint8_t ads1115_get_config(ADS1115_Handle_t *handle, uint16_t *config);

/**
 * @brief Set input multiplexer (select input channel)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param mux       ADS1115_MUX_xxx (single-ended or differential)
 * @return uint8_t  ADS1115_OK if successful
 */
uint8_t ads1115_set_mux(ADS1115_Handle_t *handle, uint16_t mux);

/**
 * @brief Set Programmable Gain Amplifier (gain setting)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param pga       ADS1115_PGA_xxx (±0.256V to ±6.144V)
 * @return uint8_t  ADS1115_OK if successful
 * 
 * @note Choose PGA appropriate for input voltage to optimize resolution.
 *       Example: For 0-3.3V measurement, use PGA_4096 (±4.096V)
 */
uint8_t ads1115_set_pga(ADS1115_Handle_t *handle, uint16_t pga);

/**
 * @brief Set operating mode (single-shot or continuous)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param mode      ADS1115_MODE_SINGLE or ADS1115_MODE_CONTINUOUS
 * @return uint8_t  ADS1115_OK if successful
 */
uint8_t ads1115_set_mode(ADS1115_Handle_t *handle, uint16_t mode);

/**
 * @brief Set sampling rate (data rate)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param rate      ADS1115_DR_xxx (8 to 860 SPS)
 * @return uint8_t  ADS1115_OK if successful
 * 
 * @note Higher speed = more noise, lower speed = better accuracy
 */
uint8_t ads1115_set_data_rate(ADS1115_Handle_t *handle, uint16_t rate);

// ============================================================================
// Conversion Functions
// ============================================================================

/**
 * @brief Start conversion (single-shot mode)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @return uint8_t  ADS1115_OK if successful
 * 
 * @note After calling this, wait for conversion to complete by calling
 *       ads1115_is_conversion_ready() or monitoring ALERT/RDY pin
 */
uint8_t ads1115_start_conversion(ADS1115_Handle_t *handle);

/**
 * @brief Check if conversion is complete
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @return uint8_t  ADS1115_OK if complete, ADS1115_NOT_READY if not
 */
uint8_t ads1115_is_conversion_ready(ADS1115_Handle_t *handle);

/**
 * @brief Read conversion result (raw 16-bit value)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param value     Pointer to store ADC value (int16_t, -32768 to +32767)
 * @return uint8_t  ADS1115_OK if successful
 * 
 * @note Raw value needs to be converted to voltage using ads1115_raw_to_voltage()
 */
uint8_t ads1115_read_conversion(ADS1115_Handle_t *handle, int16_t *value);

/**
 * @brief Read single-ended channel (vs GND)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param channel   Channel number (0-3 for AIN0-AIN3)
 * @param value     Pointer to store ADC value
 * @return uint8_t  ADS1115_OK if successful
 * 
 * @note This function automatically configures MUX, starts conversion, waits
 *       for completion, and reads result. Use for single-shot mode.
 * 
 * @example
 * int16_t adc_value;
 * ads1115_read_single_ended(&handle, 0, &adc_value);  // Read AIN0
 * float voltage = ads1115_raw_to_voltage(adc_value, ADS1115_PGA_4096);
 */
uint8_t ads1115_read_single_ended(ADS1115_Handle_t *handle, uint8_t channel, int16_t *value);

/**
 * @brief Read differential between two channels
 * 
 * @param handle        Pointer to ADS1115_Handle_t
 * @param pos_channel   Positive channel (0-2)
 * @param neg_channel   Negative channel (1 or 3)
 * @param value         Pointer to store ADC value
 * @return uint8_t      ADS1115_OK if successful
 * 
 * Valid pairs:
 * - (0,1): AIN0 - AIN1
 * - (0,3): AIN0 - AIN3
 * - (1,3): AIN1 - AIN3
 * - (2,3): AIN2 - AIN3
 * 
 * @note Use differential mode to measure voltage difference between two points,
 *       helps reject common-mode noise
 */
uint8_t ads1115_read_differential(ADS1115_Handle_t *handle, uint8_t pos_channel, 
                                  uint8_t neg_channel, int16_t *value);

/**
 * @brief Convert raw ADC value to voltage (mV)
 * 
 * @param raw_value     Raw ADC value (int16_t)
 * @param pga           PGA setting used (ADS1115_PGA_xxx)
 * @return float        Voltage in millivolts
 * 
 * @example
 * int16_t raw;
 * ads1115_read_single_ended(&handle, 0, &raw);
 * float voltage_mv = ads1115_raw_to_voltage(raw, ADS1115_PGA_2048);
 * float voltage_v = voltage_mv / 1000.0f;
 */
float ads1115_raw_to_voltage(int16_t raw_value, uint16_t pga);

// ============================================================================
// Comparator/Threshold Functions
// ============================================================================

/**
 * @brief Set high and low threshold for comparator
 * 
 * @param handle        Pointer to ADS1115_Handle_t
 * @param low_thresh    Low threshold (int16_t raw value)
 * @param high_thresh   High threshold (int16_t raw value)
 * @return uint8_t      ADS1115_OK if successful
 * 
 * Traditional comparator:
 * - ALERT asserts when ADC > high_thresh
 * - ALERT de-asserts when ADC < low_thresh
 * 
 * Window comparator:
 * - ALERT asserts when ADC < low_thresh OR ADC > high_thresh
 * 
 * @example Detect overvoltage (>3.0V with PGA ±4.096V)
 * int16_t threshold = (int16_t)(3000.0f * 32768.0f / 4096.0f);
 * ads1115_set_threshold(&handle, 0x8000, threshold);
 */
uint8_t ads1115_set_threshold(ADS1115_Handle_t *handle, int16_t low_thresh, int16_t high_thresh);

/**
 * @brief Read current threshold values
 * 
 * @param handle        Pointer to ADS1115_Handle_t
 * @param low_thresh    Pointer to store low threshold
 * @param high_thresh   Pointer to store high threshold
 * @return uint8_t      ADS1115_OK if successful
 */
uint8_t ads1115_get_threshold(ADS1115_Handle_t *handle, int16_t *low_thresh, int16_t *high_thresh);

/**
 * @brief Configure comparator
 * 
 * @param handle        Pointer to ADS1115_Handle_t
 * @param comp_mode     ADS1115_COMP_TRADITIONAL or ADS1115_COMP_WINDOW
 * @param comp_pol      ADS1115_COMP_POL_LOW or ADS1115_COMP_POL_HIGH
 * @param comp_lat      ADS1115_COMP_NONLAT or ADS1115_COMP_LAT
 * @param comp_que      ADS1115_COMP_QUE_x (1,2,4) or ADS1115_COMP_QUE_DISABLE
 * @return uint8_t      ADS1115_OK if successful
 * 
 * @note Latching mode: ALERT pin stays asserted until data is read
 *       Non-latching: ALERT auto de-asserts when condition clears
 */
uint8_t ads1115_config_comparator(ADS1115_Handle_t *handle, uint16_t comp_mode, 
                                  uint16_t comp_pol, uint16_t comp_lat, uint16_t comp_que);

// ============================================================================
// ALERT/RDY Pin Functions
// ============================================================================

/**
 * @brief Initialize GPIO for ALERT/RDY pin
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param port      GPIO port (GPIOA, GPIOB, GPIOC)
 * @param pin       GPIO pin (1, 2, 4, 8, ... 0x8000 for bit position)
 * @param mode      ADS1115_ALERT_DISABLE, ENABLE, or RDY_ENABLE
 * 
 * @note Pin is configured as input with pull-up.
 *       Usually connect ADS1115 ALERT/RDY to pin with EXTI interrupt
 * 
 * @example
 * // Connect ADS1115 ALERT/RDY pin to PA5
 * ads1115_alert_gpio_init(&handle, GPIOA, (1<<5), ADS1115_ALERT_RDY_ENABLE);
 */
void ads1115_alert_gpio_init(ADS1115_Handle_t *handle, GPIO_TypeDef *port, 
                            uint16_t pin, ADS1115_AlertMode_t mode);

/**
 * @brief Read ALERT/RDY pin state
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @return uint8_t  1 if HIGH, 0 if LOW, ADS1115_ERROR if not initialized
 * 
 * @note In conversion ready mode, pin goes LOW when new data available
 */
uint8_t ads1115_read_alert_pin(ADS1115_Handle_t *handle);

/**
 * @brief Configure ALERT/RDY pin as conversion ready signal
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * 
 * @note After calling this, ALERT/RDY pin generates pulse (LOW) whenever
 *       new conversion completes. Very useful for continuous mode.
 *       Recommend using EXTI interrupt to catch falling edge.
 * 
 * @example Use with continuous mode and interrupt
 * ads1115_enable_alert_ready_pin(&handle);
 * ads1115_start_continuous_mode(&handle, 0);
 * // In ISR: read value when interrupt received from ALERT/RDY pin
 */
void ads1115_enable_alert_ready_pin(ADS1115_Handle_t *handle);

/**
 * @brief Clear ALERT flag (in latching mode)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * 
 * @note In latching comparator mode, must read conversion register
 *       to clear ALERT pin. This function reads data to clear alert.
 */
void ads1115_clear_alert(ADS1115_Handle_t *handle);

// ============================================================================
// Continuous Mode Functions
// ============================================================================

/**
 * @brief Start continuous conversion mode
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param channel   Channel to read continuously (0-3)
 * @return uint8_t  ADS1115_OK if successful
 * 
 * @note After starting, ADS1115 continuously converts at set data rate.
 *       Use ads1115_read_continuous() to read latest value.
 *       Recommend using ALERT/RDY pin to know when new data available.
 * 
 * @example
 * ads1115_set_data_rate(&handle, ADS1115_DR_860SPS);
 * ads1115_start_continuous_mode(&handle, 0);  // Continuously read AIN0
 * while(1) {
 *     int16_t value;
 *     ads1115_read_continuous(&handle, &value);
 *     // Process value...
 *     delay_ms(2);  // Delay appropriate for data rate
 * }
 */
uint8_t ads1115_start_continuous_mode(ADS1115_Handle_t *handle, uint8_t channel);

/**
 * @brief Stop continuous mode (return to single-shot)
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @return uint8_t  ADS1115_OK if successful
 */
uint8_t ads1115_stop_continuous_mode(ADS1115_Handle_t *handle);

/**
 * @brief Read value in continuous mode
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @param value     Pointer to store ADC value
 * @return uint8_t  ADS1115_OK if successful
 * 
 * @note This function only reads conversion register, doesn't wait for new conversion.
 *       Should call when signal received from ALERT/RDY pin or after appropriate
 *       time interval matching data rate.
 */
uint8_t ads1115_read_continuous(ADS1115_Handle_t *handle, int16_t *value);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Check if ADS1115 is connected
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @return uint8_t  ADS1115_OK if connected, ADS1115_ERROR if not
 * 
 * @note Use to verify I2C communication and correct address
 */
uint8_t ads1115_is_connected(ADS1115_Handle_t *handle);

/**
 * @brief Reset ADS1115 to default configuration
 * 
 * @param handle    Pointer to ADS1115_Handle_t
 * @return uint8_t  ADS1115_OK if successful
 */
uint8_t ads1115_reset(ADS1115_Handle_t *handle);

#endif // ADS1115_HANDLER_H
