#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "pico/status_led.h"
#include "pico/binary_info.h"

#define UART_PASSTHROUGH      uart1
#define UART_PASSTHROUGH_BAUD 115200
#define UART_PASSTHROUGH_RX   5

#define I2C_ADDRESS  0x40

// INA237 register addresses
#define REG_CONFIG   0x00
#define REG_ADCCFG   0x01
#define REG_SHUNTCAL 0x02
#define REG_VSHUNT   0x04
#define REG_VBUS     0x05
#define REG_DIETEMP  0x06
#define REG_CURRENT  0x07
#define REG_POWER    0x08
#define REG_DIAG     0x0B

// Calibration for 15 mΩ shunt resistor (Adafruit INA237 breakout default), 250 mA maximum expected current
#define SHUNT_RES_OHMS  0.015f
#define MAX_CURRENT_A   0.25f
#define CURRENT_LSB_A   (MAX_CURRENT_A / 32768.0f)
// SHUNT_CAL = 819.2e6 × CURRENT_LSB × RSHUNT × 4  (ADC range 1, scale = 4)
#define SHUNT_CAL_VALUE ((uint16_t)(819.2e6f * CURRENT_LSB_A * SHUNT_RES_OHMS * 4))

// ADCCFG: triggered temp+bus+shunt (mode=0x7); 1052 µs conversion time for bus, shunt & temp (=0x5); 16 sample average (=0x2)
// bits [15:12]=0xF, bits [11:9]=0x5, bits [8:6]=0x5, bits [5:3]=0x5, bits [2:0]=0x2
#define ADCCFG_VALUE 0x7B6Au

static void write_reg(uint8_t reg, uint16_t value) {
    uint8_t buf[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    int ret = i2c_write_blocking(i2c_default, I2C_ADDRESS, buf, 3, false);
    assert(ret == 3);
}

// Read 16-bit register (the usual)
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

// Read 24-bit register (the power register)
static uint32_t read_reg3(uint8_t reg) {
    int ret = i2c_write_blocking(i2c_default, I2C_ADDRESS, &reg, 1, true);
    assert(ret == 1);
    if (ret != 1) return 0;
    uint8_t buf[3];
    ret = i2c_read_blocking(i2c_default, I2C_ADDRESS, buf, 3, false);
    assert(ret == 3);
    if (ret != 3) return 0;
    return ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
}

static void print_uart_passthrough(void) {
    // Interrupts will go off until the uart is read, so disable them
    uart_set_irqs_enabled(UART_PASSTHROUGH, false, false);
    while (uart_is_readable(UART_PASSTHROUGH))
        putchar(uart_getc(UART_PASSTHROUGH));
    uart_set_irqs_enabled(UART_PASSTHROUGH, true, false);
}

int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c / ina237_i2c example requires a board with I2C pins
    panic("Default I2C pins were not defined");
#endif
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_1pin_with_func(UART_PASSTHROUGH_RX, GPIO_FUNC_UART));
    bi_decl(bi_program_description("INA237 I2C example for the Raspberry Pi Pico"));

    printf("INA237 example\n");

    uart_init(UART_PASSTHROUGH, UART_PASSTHROUGH_BAUD);
    gpio_set_function(UART_PASSTHROUGH_RX, GPIO_FUNC_UART);

    uint irq_num = UART_IRQ_NUM(UART_PASSTHROUGH);
    irq_set_exclusive_handler(irq_num, print_uart_passthrough);
    irq_set_enabled(irq_num, true);
    uart_set_irqs_enabled(UART_PASSTHROUGH, true, false);

    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Set ADC range 1: ±40.96 mV full scale (bit 4 of CONFIG), giving 1.25 µV/LSB shunt resolution
    write_reg(REG_CONFIG, 0x0010u);
    // Write shunt calibration so that CURRENT and POWER registers are scaled correctly
    write_reg(REG_SHUNTCAL, SHUNT_CAL_VALUE);

    hard_assert(status_led_init());
    while (true) {
        status_led_set_state(true);

        // Trigger single conversion of bus voltage, shunt voltage and temperature
        // This ensures all the values will correspond to the same measurement
        write_reg(REG_ADCCFG, ADCCFG_VALUE);

        // Wait for conversion ready
        while (!(read_reg(REG_DIAG) & (1 << 1))) tight_loop_contents();

        // Bus voltage: 3.125 mV/LSB, unsigned
        float v = read_reg(REG_VBUS) * 0.003125f;

        // Shunt voltage: 1.25 µV/LSB (ADC range 1, ±40.96 mV full scale), signed
        float vshunt_mv = (int16_t)read_reg(REG_VSHUNT) * 1.25f / 1000.0f;

        // Current: CURRENT_LSB A/LSB, signed
        float ma = (int16_t)read_reg(REG_CURRENT) * CURRENT_LSB_A * 1000.0f;

        // Power: 20 × CURRENT_LSB W/LSB, unsigned
        float mw = read_reg3(REG_POWER) * 0.2f * CURRENT_LSB_A * 1000.0f;

        float mw_calc = v * ma;

        // Die temperature: bits [15:4] are the 12-bit signed value, 125 m°C/LSB
        float temp_c = ((int16_t)read_reg(REG_DIETEMP) >> 4) * 0.125f;

        printf("bus: %.3f V  shunt: %.3f mV  current: %.2f mA  power: %.2f mW (calc: %.2f mW)  temp: %.2f C\n",
               v, vshunt_mv, ma, mw, mw_calc, temp_c);

        status_led_set_state(false);

        // Wait 1s before taking next measurement
        sleep_ms(1000);
    }
    return 0;
}
