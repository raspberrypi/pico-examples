/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "mpu6050_i2c_lib.h"
#include <math.h>

#define RAD_TO_DEG(r) (r) * 57.3

// Defualt address 
static int addr = 0x68;

void mpu6050_reset(void) {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(i2c_default, addr, buf, 2, false);
    sleep_ms(100); // Allow device to reset and stabilize

    // Clear sleep mode (0x6B register, 0x00 value)
    buf[1] = 0x00;  // Clear sleep mode by writing 0x00 to the 0x6B register
    i2c_write_blocking(i2c_default, addr, buf, 2, false); 
    sleep_ms(10); // Allow stabilization after waking up
}

void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    // accel measurments are from 0x3B to 0x40
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c_default, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        // register 3B contains accel_xout[15:8]
        // next register contains accel_xout[7:0]
        // so we bit shift 3B contents by 8 and concatinate with contents of resister 3C
        // repeat for each pair of registers
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Now gyro data from reg 0x43 for 6 bytes
    // The register is auto incrementing on each read
    val = 0x43;
    i2c_write_blocking(i2c_default, addr, &val, 1, true);
    i2c_read_blocking(i2c_default, addr, buffer, 6, false);  // False - finished with bus

    for (int i = 0; i < 3; i++) {
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);;
    }

    // Now temperature from reg 0x41 for 2 bytes
    // The register is auto incrementing on each read
    val = 0x41;
    i2c_write_blocking(i2c_default, addr, &val, 1, true);
    i2c_read_blocking(i2c_default, addr, buffer, 2, false);  // False - finished with bus

    *temp = buffer[0] << 8 | buffer[1];
}

// FS values are 0, 1, 2, or 3
// For gyro, correspond to += 250, 500, 1000, 2000 deg/s
// For accel, correspond to += 2, 4, 8, 16g
int mpu6050_initialise(int sda_pin, int scl_pin, int gyro_fs, int accel_fs) {
    // Set the i2c pins
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    // set full scale ranges for gyro and accel
    // register 1B is gyro config - bit 4 and bit 3 are used for full scale select
    uint8_t buffer[1];
    const uint8_t gyro_config = 0x1B;

    // first read
    i2c_write_blocking(i2c_default, addr, &gyro_config, 1, true);
    i2c_read_blocking(i2c_default, addr, buffer, 1, false); 

    uint8_t to_write = buffer[0] | ((gyro_fs & 3) << 3); // Set gyro bits

    uint8_t write_buf[2] = {gyro_config, to_write};
    i2c_write_blocking(i2c_default, addr, write_buf, 1, false);

    // register 1C is for accel config
    const uint8_t accel_config = 0x1C;

    // first read
    i2c_write_blocking(i2c_default, addr, &accel_config, 1, true);
    i2c_read_blocking(i2c_default, addr, buffer, 1, false); 

    to_write = buffer[0] | ((accel_fs & 3) << 3); // Set accel bits

    write_buf[0] = accel_config;
    write_buf[1] = to_write;
    i2c_write_blocking(i2c_default, addr, write_buf, 1, false);

    return 0;
}

// function takes num sensor readings to get gyro offsets
// sets roll_offset, pitch_offset and yaw_offset
void mpu6050_get_gyro_offset(int num, float *roll_offset, float *pitch_offset, float *yaw_offset, int gyro_fs) {
    float r_offset = 0;
    float p_offset = 0;
    float y_offset = 0;
    int16_t acceleration[3], gyro[3], temp;

    // can calculate gyro full scale range from the setting using 250 * 2^setting
    int range = 250 * pow(2, gyro_fs);

    const float scale_factor = 1 / 32768.0 * range;
    for (int i = 0; i < num; i++) {
        mpu6050_read_raw(acceleration, gyro, &temp);
        r_offset += gyro[0] * scale_factor;
        p_offset += gyro[1] * scale_factor;
        y_offset += gyro[2] * scale_factor;
    }
    r_offset /= num;
    p_offset /= num;
    y_offset /= num;

    printf("roll offset %f\n", r_offset);
    printf("pitch offset %f\n", p_offset);
    printf("yaw offset is %f\n", y_offset);

    *roll_offset = r_offset;
    *pitch_offset = p_offset;
    *yaw_offset = y_offset;
}

// function outputs roll, pitch using sensor fusion
void mpu6050_fusion_output(float roll_offset, float pitch_offset, float yaw_offset, float alpha, int delta_ms, float *roll, float *pitch, float *yaw, int gyro_fs) {
    int16_t acceleration[3], gyro[3], temp;
    mpu6050_read_raw(acceleration, gyro, &temp);
    // calculate pitch and roll from accelerometer output
    // this calculation is independent of full scale
    float accel_roll = RAD_TO_DEG(atan2(acceleration[1] , acceleration[2]));
    float accel_pitch = RAD_TO_DEG(atan2((- acceleration[0]) , hypot(acceleration[1], acceleration[2])));

    // can calculate gyro full scale range from the setting using 250 * 2^setting
    int range = 250 * pow(2, gyro_fs);
    float roll_rate = gyro[0] / 32768.0 * range - roll_offset;
    float pitch_rate = gyro[1] / 32768.0 * range - pitch_offset;
    float yaw_rate = gyro[2] / 32768.0 * range - yaw_offset;

    // calculate pitch and roll using sensor fusion (complimentary filter)
    *roll = (1 - alpha) * (*roll + roll_rate * delta_ms/1000) + alpha * accel_roll;
    *pitch = (1 - alpha) * (*pitch + pitch_rate * delta_ms/1000) + alpha * accel_pitch;
    *yaw = *yaw + yaw_rate * delta_ms/1000;   
}
