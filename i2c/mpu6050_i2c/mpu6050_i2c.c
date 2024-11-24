/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>
#include <stdint.h>
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "mpu6050_i2c.h"


// By default these devices are on bus address 0x68
static uint8_t bus_addr = 0x68;
// Register addresses in the MPU6050 to read and write data to
const uint8_t REG_ACCEL_OUT = 0x3B, REG_GYRO_OUT = 0x43, REG_TEMP_OUT = 0x41,
              REG_INT_PIN_CFG = 0x37, REG_INT_ENABLE = 0x38, REG_INT_STATUS = 0x3A,
              REG_SMPRT_DIV = 0x19, REG_SIGNAL_PATH_RESET = 0x68, REG_PWR_MGMT_1 = 0x6B, REG_WHO_AM_I = 0x75,
              REG_CONFIG = 0x1A, REG_GYRO_CONFIG = 0x1B, REG_ACCEL_CONFIG = 0x1C;

const float gyro_scale_deg[] = {250. / 0x7fff, 500. / 0x7fff, 1000. / 0x7fff, 2000. / 0x7fff};
const float gyro_scale_rad[] = {M_PI / 180. * 250. / 0x7fff, M_PI / 180. * 500. / 0x7fff,
                          M_PI / 180. *  1000. / 0x7fff, M_PI / 180. *  2000. / 0x7fff};
const float accel_scale_vals[] = {2 * 9.81 / 0x7fff, 4 * 9.81 / 0x7fff, 8 * 9.81 / 0x7fff, 16 * 9.81 / 0x7fff};

// timing data, indexed with lowpass_filter_cfg
const int accel_bandwidth_lookup[] = {260, 184, 94, 44, 21, 10, 5};
const float accel_delay_lookup[] = {0, 2.0, 3.0, 4.9, 8.5, 13.8, 19.0};
const int gyro_bandwidth_lookup[] = {256, 188, 98, 42, 20, 10, 5};
const float gyro_delay_lookup[] = {0.98, 1.9, 2.8, 4.8, 8.3, 13.4, 18.6};


////////////////////////////////////////////////////lower level functions for i2c
#ifdef i2c_default
void mpu6050_writereg(uint8_t reg, uint8_t value) {
    // write one byte to the register reg. Send register byte then data byte.
    uint8_t buf[] = {reg, value};
    i2c_write_blocking(i2c_default, bus_addr, buf, 2, false);  // False - finished with bus
}

void mpu6050_readreg(uint8_t reg, uint8_t *out, size_t length) {
    i2c_write_blocking(i2c_default, bus_addr, &reg, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, bus_addr, out, length, false);
}
#else
void mpu6050_writereg(uint8_t reg, uint8_t value) {};
void mpu6050_readreg(uint8_t reg, uint8_t *out, size_t length) {};
#endif

void mpu6050_readreg16(uint8_t reg, int16_t *out, size_t length) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.
    static uint8_t buffer[64];
    if (length > 32) length = 32;
    mpu6050_readreg(reg, buffer, length * 2);
    for (int i = 0; i < length; i++) {
        out[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }
}

void mpu6050_setbusaddr(uint8_t addr) {
    bus_addr = addr;
}


////////////////////////////////////////////////////higher level mpu6050 functions
void mpu6050_power(uint8_t CLKSEL, bool temp_disable, bool sleep, bool cycle) {
    uint8_t pwrval = (CLKSEL & 7) + ((uint8_t)temp_disable << 3)
                    + ((uint8_t)sleep << 6) + ((uint8_t)cycle << 5);
    mpu6050_writereg(REG_PWR_MGMT_1, pwrval);
}

void mpu6050_reset() {
    // Reset instructions come from register map doc
    mpu6050_writereg(REG_PWR_MGMT_1, 1 << 7); //DEVICE_RESET = 1
    sleep_ms(100);
    mpu6050_writereg(REG_SIGNAL_PATH_RESET, 7); //GYRO_RESET = ACCEL_RESET = TEMP_RESET = 1
    sleep_ms(100);
}

void mpu6050_read_accel_raw(int16_t accel[3]) {
    mpu6050_readreg16(REG_ACCEL_OUT, accel, 3);
}

void mpu6050_read_gyro_raw(int16_t gyro[3]) {
    mpu6050_readreg16(REG_GYRO_OUT, gyro, 3);
}

void mpu6050_read(float accel[3], float gyro_rad[3], float *temp, MPU6050_Scale accel_scale, MPU6050_Scale gyro_scale) {
    // reads acceleration [m/s], gyroscope [rad], and temperature [C] in one i2c interaction 
    int16_t buffer[7]; // 3 accel + 3 gyro + scalar temp
    mpu6050_readreg16(REG_ACCEL_OUT, buffer, 7);
    mpu6050_scale_accel(accel, buffer, accel_scale);
    if (temp) *temp = mpu6050_scale_temp(buffer[3]);
    mpu6050_scale_gyro_rad(gyro_rad, buffer + 4, gyro_scale);
}
void mpu6050_read_accel(float accel[3], MPU6050_Scale accel_scale) {
    int16_t buffer[3];
    mpu6050_readreg16(REG_ACCEL_OUT, buffer, 3);
    mpu6050_scale_accel(accel, buffer, accel_scale);
}
void mpu6050_read_gyro_rad(float gyro[3], MPU6050_Scale gyro_scale) {
    int16_t buffer[3];
    mpu6050_readreg16(REG_GYRO_OUT, buffer, 3);
    mpu6050_scale_gyro_rad(gyro, buffer, gyro_scale);
}
void mpu6050_read_gyro_deg(float gyro[3], MPU6050_Scale gyro_scale) {
    int16_t buffer[3];
    mpu6050_readreg16(REG_GYRO_OUT, buffer, 3);
    mpu6050_scale_gyro_deg(gyro, buffer, gyro_scale);
}

// functions to set and calculate with sensitivity
void mpu6050_setscale_gyro(MPU6050_Scale gyro_scale) {
    mpu6050_writereg(REG_GYRO_CONFIG, (uint8_t)gyro_scale << 3);
}
void mpu6050_setscale_accel(MPU6050_Scale accel_scale) {
    mpu6050_writereg(REG_ACCEL_CONFIG, (uint8_t)accel_scale << 3);
}
void mpu6050_scale_accel(float accel[3], int16_t accel_raw[3], MPU6050_Scale scale) {
    for (int i = 0; i < 3; i++) {
        accel[i] = accel_raw[i] * accel_scale_vals[scale];
    }
}
void mpu6050_scale_gyro_deg(float gyro[3], int16_t gyro_raw[3], MPU6050_Scale scale) {
    for (int i = 0; i < 3; i++) {
        gyro[i] = gyro_raw[i] * gyro_scale_deg[scale];
    }
}
void mpu6050_scale_gyro_rad(float gyro[3], int16_t gyro_raw[3], MPU6050_Scale scale) {
    for (int i = 0; i < 3; i++) {
        gyro[i] = gyro_raw[i] * gyro_scale_rad[scale];
    }
}
float mpu6050_scale_temp(float temp_raw) {
    return (temp_raw / 340.0) + 36.53;
}

void mpu6050_gyro_selftest_on(void) {
    uint8_t regdata = 0b11100000 & ((uint8_t)MPU_FS_0 << 3);
    mpu6050_writereg(REG_GYRO_CONFIG, regdata);
}
void mpu6050_accel_selftest_on(void) {
    uint8_t regdata = 0b11100000 & ((uint8_t)MPU_FS_2 << 3);
    mpu6050_writereg(REG_ACCEL_CONFIG, regdata);
}

void mpu6050_set_timing(uint8_t lowpass_filter_cfg, uint8_t sample_rate_div) {
    mpu6050_writereg(REG_SMPRT_DIV, sample_rate_div);
    mpu6050_writereg(REG_CONFIG, lowpass_filter_cfg & 7);
}
void mpu6050_read_timing(mpu6050_timing_params_t *accel_timing, mpu6050_timing_params_t *gyro_timing) {
    uint8_t reg_data[2];
    mpu6050_readreg(REG_SMPRT_DIV, reg_data, 2);
    mpu6050_calc_timing(reg_data[1], reg_data[0], accel_timing, gyro_timing);
}
void mpu6050_calc_timing(uint8_t filter_cfg, uint8_t sample_rate_div,
                    mpu6050_timing_params_t *accel_timing, mpu6050_timing_params_t *gyro_timing) {
    int i = filter_cfg > 6 ? 0 : filter_cfg;
    float gyro_measure_rate = (i == 0) ? 8000 : 1000;
    if (accel_timing) {
        accel_timing->bandwidth = accel_bandwidth_lookup[i];
        accel_timing->delay = accel_delay_lookup[i];
        accel_timing->sample_rate = fmin(1000, gyro_measure_rate / (1 + sample_rate_div));
    }
    if (gyro_timing) {
        gyro_timing->bandwidth = gyro_bandwidth_lookup[i];
        gyro_timing->delay = gyro_delay_lookup[i];
        gyro_timing->sample_rate = gyro_measure_rate / (1 + sample_rate_div);
    }
}

void mpu6050_configure_interrupt(bool active_low, bool open_drain, bool latch_pin, bool read_clear, bool enable) {
    mpu6050_writereg(REG_INT_PIN_CFG,   ((uint8_t)active_low << 7) + ((uint8_t)open_drain << 6)
                                       + ((uint8_t)latch_pin << 5) + ((uint8_t)read_clear << 4));
    mpu6050_writereg(REG_INT_ENABLE, (uint8_t)enable);
}
uint8_t mpu6050_read_interrupt_status() {
    uint8_t interrupt_status;
    mpu6050_readreg(REG_INT_STATUS, &interrupt_status, 1);
    return interrupt_status;
}

bool mpu6050_is_connected(void) {
    uint8_t who_are_you = 0;
    mpu6050_readreg(REG_WHO_AM_I, &who_are_you, 1);
    return who_are_you == 0x68;
}


bool setup_MPU6050_i2c() {
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/mpu6050_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
    return false;
#else
    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 400 * 1000); // Max bus speed 400 kHz
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    return true;
#endif
}
