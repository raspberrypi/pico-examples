#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/status_led.h"
#include "pico/binary_info.h"

#define I2C_ADDRESS  0x40

// INA219 register addresses
#define REG_CONFIG      0x00
#define REG_SHUNTVOLT   0x01
#define REG_BUSVOLT     0x02
#define REG_POWER       0x03
#define REG_CURRENT     0x04
#define REG_CALIBRATION 0x05

// CONFIG: 16V bus range, gain ÷1 (±40mV shunt), 12-bit 32-sample averaging, continuous bus+shunt
// bit13=0 (16V), bits[12:11]=00 (÷1), bits[10:7]=0xD (32S), bits[6:3]=0xD (32S), bits[2:0]=7
#define CONFIG_VALUE 0x06EFu

// Calibration for 0.1 Ω shunt resistor (Adafruit INA219 breakout default), 400 mA maximum current
// CAL = 0.04096 / (current_lsb_A × rshunt) = 0.04096 / (0.0001 × 0.1) = 4096
#define CALIBRATION_VALUE 4096u
#define CURRENT_LSB_MA    0.1f   // 0.04096 / (4096 × 0.1 Ω) × 1000 mA/LSB
#define POWER_LSB_MW      2.0f   // 20 × CURRENT_LSB_MA mW/LSB

// Shunt voltage: signed 16-bit, 10 µV/LSB = 0.01 mV/LSB
// Bus voltage: bits [15:3], 4 mV/LSB; bit 1 = conversion ready, bit 0 = math overflow
#define VSHUNT_LSB_MV  0.01f
#define VBUS_LSB_MV    4.0f
#define BUSVOLT_OVF    0x01u

static void write_reg(uint8_t reg, uint16_t value) {
    uint8_t buf[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    int ret = i2c_write_blocking(i2c_default, I2C_ADDRESS, buf, 3, false);
    assert(ret == 3);
}

static uint16_t read_reg(uint8_t reg) {
    int ret = i2c_write_blocking(i2c_default, I2C_ADDRESS, &reg, 1, true);
    assert(ret == 1);
    if (ret != 1) return 0;
    uint8_t buf[2];
    ret = i2c_read_blocking(i2c_default, I2C_ADDRESS, buf, 2, false);
    assert(ret == 2);
    if (ret != 2) return 0;
    return ((uint16_t)buf[0] << 8) | buf[1];
}

int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c / ina219_i2c example requires a board with I2C pins
    panic("Default I2C pins were not defined");
#endif
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("INA219 I2C example for the Raspberry Pi Pico"));

    printf("INA219 example\n");

    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    write_reg(REG_CONFIG, CONFIG_VALUE);
    write_reg(REG_CALIBRATION, CALIBRATION_VALUE);

    hard_assert(status_led_init());
    while (true) {
        status_led_set_state(true);

        // Re-write calibration each iteration: a sharp load transient can reset the INA219,
        // clearing the calibration register and making CURRENT and POWER read zero
        write_reg(REG_CALIBRATION, CALIBRATION_VALUE);

        // Shunt voltage: 10 µV/LSB, signed
        float vshunt_mv = (int16_t)read_reg(REG_SHUNTVOLT) * VSHUNT_LSB_MV;

        // Bus voltage: bits [15:3] at 4 mV/LSB; bit 0 is the math overflow flag
        uint16_t bus_raw = read_reg(REG_BUSVOLT);
        float vbus_v = (bus_raw >> 3) * VBUS_LSB_MV / 1000.0f;

        // Current: signed, CURRENT_LSB_MA mA/LSB
        float ma = (int16_t)read_reg(REG_CURRENT) * CURRENT_LSB_MA;

        // Power: unsigned, POWER_LSB_MW mW/LSB
        float mw = read_reg(REG_POWER) * POWER_LSB_MW;

        // INA219 measures on the high side: VIN+ is the supply, VIN- is the load
        float vin_v = vbus_v + vshunt_mv / 1000.0f;

        printf("VIN+: %.3f V  VIN-: %.3f V  shunt: %.3f mV  current: %.2f mA  power: %.2f mW\n",
               vin_v, vbus_v, vshunt_mv, ma, mw);

        if (bus_raw & BUSVOLT_OVF) {
            printf("Math overflow - measurement out of range\n");
        }

        status_led_set_state(false);
        sleep_ms(1000);
    }
    return 0;
}
