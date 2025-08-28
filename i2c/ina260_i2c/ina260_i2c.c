#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define CURRENT_REGISTER 0x01
#define VOLTAGE_REGISTER 0x02
#define POWER_REGISTER 0x03
#define I2C_ADDRESS 0x40

#define ByteSwap(u)  (uint16_t)((u << 8)|(u >> 8))

// Read register value
static uint16_t read_reg(uint8_t reg) {
    // Set the register address
    int ret = i2c_write_blocking(i2c_default, I2C_ADDRESS, &reg, 1, true); // no stop is set
    assert(ret == 1);
    if (ret != 1) return 0;
    // Read the value
    uint16_t data;
    ret = i2c_read_blocking(i2c_default, I2C_ADDRESS, (uint8_t*)&data, 2, false);
    assert(ret == 2);
    if (ret != 2) return 0;
    return ByteSwap(data);
}

int main() {
    setup_default_uart();
    printf("ina260 test\n");

    // Initialise i2c
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    while(true) {

        // Read current and convert to mA
        float ma = read_reg(CURRENT_REGISTER) * 1.250f;
        if (ma > 15000) ma = 0;
        // Read the voltage
        float v = read_reg(VOLTAGE_REGISTER) * 0.00125f;
        // Read power and convert to mW
        uint16_t mw = read_reg(POWER_REGISTER) * 10;
    
        // Display results
        printf("current: %.2f mA voltage: %.2f v power: %u mW\n", ma, v, mw);
        sleep_ms(1000);
    }
    return 0;
}
