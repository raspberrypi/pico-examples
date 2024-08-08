/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "mpu6050_i2c.h"

/* Example code to talk to a MPU6050 MEMS accelerometer and gyroscope

   This is taking to simple approach of simply reading registers. It's perfectly
   possible to link up an interrupt line and set things up to read from the
   inbuilt FIFO to make it more useful.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefor I2C) cannot be used at 5v. You will need to use a level
   shifter on the I2C lines if you want to run the board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is GP4 (pin 6)) -> SDA on MPU6050 board
   GPIO PICO_DEFAULT_I2C_SCL_PIN (On Pico this is GP5 (pin 7)) -> SCL on MPU6050 board
   3.3v (pin 36) -> VCC on MPU6050 board
   GND (pin 38)  -> GND on MPU6050 board
*/

// By default these devices are on bus address 0x68
static uint8_t bus_addr = 0x68;
// Register addresses in the MPU6050 to read and write data to
const uint8_t REG_PWR_MGMT_1 = 0x6B, REG_ACCEL_OUT = 0x3B, REG_GYRO_OUT = 0x43, REG_TEMP_OUT = 0x41,
             REG_SIGNAL_PATH_RESET = 0x68, REG_GYRO_CONFIG = 0x1B, REG_ACCEL_CONFIG = 0x1C, REG_WHO_AM_I = 0x75;

const float gyro_scale_deg[] = {250. / 0x7fff, 500. / 0x7fff, 1000. / 0x7fff, 2000. / 0x7fff};
const float gyro_scale_rad[] = {M_PI / 180. * 250. / 0x7fff, M_PI / 180. * 500. / 0x7fff,
                          M_PI / 180. *  1000. / 0x7fff, M_PI / 180. *  2000. / 0x7fff};
const float accel_scale_vals[] = {2 * 9.81 / 0x7fff, 4 * 9.81 / 0x7fff, 8 * 9.81 / 0x7fff, 16 * 9.81 / 0x7fff};


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

void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
    mpu6050_readreg16(REG_ACCEL_OUT, accel, 3);
    mpu6050_readreg16(REG_GYRO_OUT, gyro, 3);
    mpu6050_readreg16(REG_TEMP_OUT, temp, 1);
}

void mpu6050_read(float accel[3], float gyro_rad[3], float *temp, MPU6050_Scale accel_scale, MPU6050_Scale gyro_scale) {
    // reads acceleration [m/s], gyroscope [rad], and temperature [C] in one i2c interaction 
    int16_t buffer[7]; // 3 accel + 3 gyro + scalar temp
    mpu6050_readreg16(REG_ACCEL_OUT, buffer, 7);
    mpu6050_scale_accel(accel, buffer, accel_scale);
    if (temp) *temp = mpu6050_scale_temp(buffer[3]);
    mpu6050_scale_gyro_rad(gyro_rad, buffer + 4, gyro_scale);
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

int run_MPU6050_demo() {
    MPU6050_Scale testscale = MPU_FS_0;
    mpu6050_setscale_accel(testscale);
    mpu6050_setscale_gyro(testscale);

    int16_t accel_raw[3], gyro_raw[3], temp_raw;
    float acceleration[3], gyro[3];

    mpu6050_reset();
    //setting CLKSEL = 1 gets better results than 0 if gyro is running and no sleep mode
    mpu6050_power(1, false, false, false);

    while (1) {
        while (!mpu6050_is_connected()) {
            printf("MPU6050 is not connected...\n");
            sleep_ms(1000);
        }
        uint64_t time_us = to_us_since_boot(get_absolute_time());
        mpu6050_read_raw(accel_raw, gyro_raw, &temp_raw);
        time_us = to_us_since_boot(get_absolute_time()) - time_us;

        // These are the raw numbers from the chip
        printf("Raw Acc. X = %d, Y = %d, Z = %d\n", accel_raw[0], accel_raw[1], accel_raw[2]);
        printf("Raw Gyro. X = %d, Y = %d, Z = %d\n", gyro_raw[0], gyro_raw[1], gyro_raw[2]);
        // This is chip temperature.
        printf("Temp [C] = %f\n", mpu6050_scale_temp(temp_raw));
        printf("Read time: %llu us; \t\t Accel and Gyro Scale = %d\n", time_us, (int)testscale);
        mpu6050_scale_accel(acceleration, accel_raw, testscale);
        mpu6050_scale_gyro_rad(gyro, gyro_raw, testscale);
        printf("Acc [m/s^2] X = %f, Y = %f, Z = %f\n", acceleration[0], acceleration[1], acceleration[2]);
        printf("Gyro [rad/s] X = %f, Y = %f, Z = %f\n", gyro[0], gyro[1], gyro[2]);

        sleep_ms(100);
    }
    return 0;
}
