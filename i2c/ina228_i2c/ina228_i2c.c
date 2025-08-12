#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <math.h>

// LED GPIO
#define EXT_LED_GPIO 2

// INA228 breakout board shunt resistance in Ohms
#define R_SHUNT 0.015

// Max expected current through shunt resistor in Amps
#define MAX_EXPECTED_CURRENT 0.1
#define I2C_ADDR 0x40

// Macros for converting arrays of bytes to decimal integer
#define COALESCE_2_BYTES(a2) ((a2[0] << 8) | a2[1])
#define COALESCE_3_BYTES(a3) ((a3[0] << 16) | (a3[1] << 8) | a3[2])
#define COALESCE_RESERVED_3_BYTES(a3) ((a3[0] << 12) | (a3[1] << 4) | (a3[2] >> 4))
#define COALESCE_5_BYTES(a5) (((uint64_t)a5[0] << 32) | (a5[1] << 24) | (a5[2] << 16) | (a5[3] << 8) | a5[4])

// ina228 registers (see datasheet)
uint8_t VSHUNT_REG = 0x04;
uint8_t VBUS_REG = 0x05;
uint8_t DIETEMP_REG = 0x06;
uint8_t CURRENT_REG = 0x07;
uint8_t POWER_REG = 0x08;
uint8_t ENERGY_REG = 0x09;
uint8_t CHARGE_REG = 0x0A;

uint8_t ADC_CONFIG_REG = 0x01;
uint8_t SHUNT_CAL_REG = 0x02;

// conversion factors (see datasheet)
// note: VSHUNT_FACTOR assumes ADCRANGE = 0 
const double VSHUNT_FACTOR = 312.5 * 1e-9;
const double VBUS_FACTOR = 195.3125 * 1e-6;
const double DIETEMP_FACTOR  = 7.8125 * 1e-3;
const double CURRENT_FACTOR = MAX_EXPECTED_CURRENT / (1<<19);
const double POWER_FACTOR = 3.2 * CURRENT_FACTOR ;
const double ENERGY_FACTOR  = 16 * 3.2 * CURRENT_FACTOR;
const double CHARGE_FACTOR = CURRENT_FACTOR;

const uint16_t SHUNT_CAL = 13107.2 * 1e6 * CURRENT_FACTOR * R_SHUNT;

float vshunt, vbus, dietemp, current, power, energy, charge;

static void ina228_init() {
    // Write to shunt register 
    uint8_t shunt_msb = (SHUNT_CAL >> 8) & 0xFF;
    uint8_t shunt_lsb = SHUNT_CAL & 0xFF;
    uint8_t shunt_buf[3] = {SHUNT_CAL_REG, shunt_msb, shunt_lsb};
    i2c_write_blocking(i2c_default, I2C_ADDR, shunt_buf, 3, false);

    // Write to ADC config register (need to enable continuous mode to access accumulation variables energy and charge)
    uint8_t adc_msb = 0xF0;
    uint8_t adc_lsb = 0x00;
    uint8_t adc_buf[3] = {ADC_CONFIG_REG, adc_msb, adc_lsb};
    i2c_write_blocking(i2c_default, I2C_ADDR, adc_buf, 3, false);
}

static void ina228_read(float *vshunt, float *vbus, float *dietemp, float *current, float *power, float *energy, float *charge) {
    // Buffers for writing register measurements to
    // Some registers are 2 bytes, some are 3 bytes, the accumulation registers are 5 bytes
    uint8_t buffer_2[2];
    uint8_t buffer_3[3];
    uint8_t buffer_5[5];

    // For the vshunt, vbus and current registers the 4 least significant bits are reserved and always read zero, so use variant of coalesce macro to extract value
    i2c_write_blocking(i2c_default, I2C_ADDR, &VSHUNT_REG, 1, true);
    i2c_read_blocking(i2c_default, I2C_ADDR, buffer_3, 3, false);
    *vshunt = COALESCE_RESERVED_3_BYTES(buffer_3) * VSHUNT_FACTOR;

    i2c_write_blocking(i2c_default, I2C_ADDR, &VBUS_REG, 1, true);
    i2c_read_blocking(i2c_default, I2C_ADDR, buffer_3, 3, false);
    *vbus = COALESCE_RESERVED_3_BYTES(buffer_3) * VBUS_FACTOR;

    i2c_write_blocking(i2c_default, I2C_ADDR, &DIETEMP_REG, 1, true);
    i2c_read_blocking(i2c_default, I2C_ADDR, buffer_2, 2, false);
    *dietemp = COALESCE_2_BYTES(buffer_2) * DIETEMP_FACTOR;

    i2c_write_blocking(i2c_default, I2C_ADDR, &CURRENT_REG, 1, true);
    i2c_read_blocking(i2c_default, I2C_ADDR, buffer_3, 3, false);
    *current = COALESCE_RESERVED_3_BYTES(buffer_3) * CURRENT_FACTOR;

    i2c_write_blocking(i2c_default, I2C_ADDR, &POWER_REG, 1, true);
    i2c_read_blocking(i2c_default, I2C_ADDR, buffer_3, 3, false);
    *power = COALESCE_3_BYTES(buffer_3) * POWER_FACTOR;

    i2c_write_blocking(i2c_default, I2C_ADDR, &ENERGY_REG, 1, true);
    i2c_read_blocking(i2c_default, I2C_ADDR, buffer_5, 5, false);
    *energy = COALESCE_5_BYTES(buffer_5) * ENERGY_FACTOR;

    i2c_write_blocking(i2c_default, I2C_ADDR, &CHARGE_REG, 1, true);
    i2c_read_blocking(i2c_default, I2C_ADDR, buffer_5, 5, false);
    *charge = COALESCE_5_BYTES(buffer_5) * CHARGE_FACTOR;
}

int main()
{
    stdio_init_all();

    // Initialise external LED and turn it on (so we can measure some current)
    gpio_init(EXT_LED_GPIO);
    gpio_set_dir(EXT_LED_GPIO, GPIO_OUT);
    gpio_put(EXT_LED_GPIO, true);

    // I2C initialisation
    i2c_init(i2c_default, 400*1000);
    
    // GPIO initialisation
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Initialise ina228
    ina228_init();

    while (true) {
        ina228_read(&vshunt, &vbus, &dietemp, &current, &power, &energy, &charge);
        printf("INA228 Measurements:\nVSHUNT: %f V\nVBUS: %f V\nDIETEMP: %f °C\nCURRENT: %f A\nPOWER: %f W\nENERGY: %f J\nCHARGE: %f C\n-----------------\n", vshunt, vbus, dietemp, current, power, energy, charge);
        sleep_ms(1000);
    }
}
