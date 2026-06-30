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

#define CONFIG_REGISTER 0x00

#ifndef RTC_CLOCK_SRC_GPIO_OUT
#define RTC_CLOCK_SRC_GPIO_OUT 21
#endif

// Averaging: number of samples the INA260 averages in hardware before updating
// its registers (CONFIG register AVG field, bits 11:9). More = quieter readings
// but slower updates. Helpful for low sleep currents.
// Allowed values: 1, 4, 16, 64, 128, 256, 512, 1024.
#define AVG_SAMPLES 128

#if   AVG_SAMPLES == 1
  #define AVG_FIELD 0
#elif AVG_SAMPLES == 4
  #define AVG_FIELD 1
#elif AVG_SAMPLES == 16
  #define AVG_FIELD 2
#elif AVG_SAMPLES == 64
  #define AVG_FIELD 3
#elif AVG_SAMPLES == 128
  #define AVG_FIELD 4
#elif AVG_SAMPLES == 256
  #define AVG_FIELD 5
#elif AVG_SAMPLES == 512
  #define AVG_FIELD 6
#elif AVG_SAMPLES == 1024
  #define AVG_FIELD 7
#else
  #error "AVG_SAMPLES must be one of: 1, 4, 16, 64, 128, 256, 512, 1024"
#endif

// CONFIG register value:
//   bit 15      reset (0)
//   bits 11:9   AVG = samples averaged
//   bits 8:6    VBUSCT, 5:3 ISHCT = conversion times (left at default 0b100 = 1.1 ms)
//   bits 2:0    MODE = 0b111 (continuous shunt + bus)
#define CONFIG_VALUE ( ((AVG_FIELD & 0x7) << 9) \
                     | (0x4 << 6)  /* VBUSCT 1.1 ms */ \
                     | (0x4 << 3)  /* ISHCT  1.1 ms */ \
                     | 0x7 )       /* continuous shunt + bus */

// Swap the byte order of a 16-bit value (INA260 returns big-endian over I2C).
static inline uint16_t byte_swap(uint16_t u) {
    return (uint16_t)((u << 8) | (u >> 8));
}

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
    return byte_swap(data);
}

// Write a 16-bit register value
static void write_reg(uint8_t reg, uint16_t value) {
    uint8_t buf[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    int ret = i2c_write_blocking(i2c_default, I2C_ADDRESS, buf, 3, false);
    assert(ret == 3);
    (void)ret;
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

    // Configure averaging + continuous mode (see CONFIG_VALUE above).
    write_reg(CONFIG_REGISTER, CONFIG_VALUE);

    hard_assert(status_led_init());
    while(true) {
        status_led_set_state(true);

        // Read current and convert to mA. The CURRENT register is signed
        // (two's complement), so cast through int16_t before scaling -- otherwise
        // small negative or near-zero readings wrap to a huge positive number.
        float ma = (int16_t)read_reg(CURRENT_REGISTER) * 1.25f;
        // Read the voltage (1.25 mV/LSB, unsigned)
        float v = read_reg(VOLTAGE_REGISTER) * 0.00125f;
        // Read power and convert to mW (10 mW/LSB). Use 32-bit: register x 10
        // exceeds uint16_t range.
        uint32_t mw = (uint32_t)read_reg(POWER_REGISTER) * 10;

        // Display results
        printf("current: %.2f mA voltage: %.2f V power: %lu mW\n", ma, v, (unsigned long)mw);

        status_led_set_state(false);
        sleep_ms(1000);
    }
    return 0;
}