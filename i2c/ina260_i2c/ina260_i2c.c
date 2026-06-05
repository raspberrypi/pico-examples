#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "pico/status_led.h"
#include "pico/binary_info.h"

#define CURRENT_REGISTER 0x01
#define VOLTAGE_REGISTER 0x02
#define POWER_REGISTER 0x03
#define I2C_ADDRESS 0x40

#define ByteSwap(u)  (uint16_t)((u << 8)|(u >> 8))

#ifndef RTC_CLOCK_SRC_GPIO_OUT
#define RTC_CLOCK_SRC_GPIO_OUT 21
#endif

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
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c / ina260_i2c example requires a board with I2C pins
    panic("Default I2C pins were not defined");
#endif
    // useful information for picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("INA260 I2C example for the Raspberry Pi Pico"));
    bi_decl(bi_1pin_with_func(RTC_CLOCK_SRC_GPIO_OUT, GPIO_FUNC_GPCK));

    printf("ina260 example\n");

    // output a clock GP21 that can be used for dormant testing with RP2040
    clock_gpio_init(RTC_CLOCK_SRC_GPIO_OUT, CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLK_USB, 1024); // 48kHz

    // Initialise i2c
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    hard_assert(status_led_init());
    while(true) {
        status_led_set_state(true);

        // Read current and convert to mA
        float ma = read_reg(CURRENT_REGISTER) * 1.250f;
        if (ma > 15000) ma = 0;
        // Read the voltage
        float v = read_reg(VOLTAGE_REGISTER) * 0.00125f;
        // Read power and convert to mW
        uint16_t mw = read_reg(POWER_REGISTER) * 10;
    
        // Display results
        printf("current: %.2f mA voltage: %.2f V power: %u mW\n", ma, v, mw);

        status_led_set_state(false);
        sleep_ms(1000);
    }
    return 0;
}
