/**
   Example code to talk to a MPU6050 MEMS accelerometer and gyroscope

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefor I2C) cannot be used at 5v. You will need to use a level
   shifter on the I2C lines if you want to run the board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is GP4 (pin 6)) -> SDA on MPU6050 board
   GPIO PICO_DEFAULT_I2C_SCL_PIN (On Pico this is GP5 (pin 7)) -> SCL on MPU6050 board
   3.3v (pin 36) -> VCC on MPU6050 board
   GND (pin 38)  -> GND on MPU6050 board
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "mpu6050_i2c.h"


int main() {
    stdio_init_all();
    if (setup_MPU6050_i2c()) {
        printf("Hello, MPU6050! Reading raw data from registers...\n");
    }
    MPU6050_Scale testscale = MPU_FS_0;

    float acceleration[3], gyro[3], temperature;
    int16_t raw_accel[3], raw_gyro[3];

    mpu6050_reset();
    //setting CLKSEL = 1 gets better results than 0 if gyro is running and no sleep mode
    mpu6050_power(1, false, false, false);
    mpu6050_setscale_accel(testscale);
    mpu6050_setscale_gyro(testscale);
    mpu6050_set_timing(6, 99); // lowpass frequency = 5 Hz, sample rate = 10 Hz

    while (1) {
        while (!mpu6050_is_connected()) {
            printf("MPU6050 is not connected...\n");
            sleep_ms(1000);
        }
        uint64_t time_us = to_us_since_boot(get_absolute_time());
        mpu6050_read_accel_raw(raw_accel);
        mpu6050_read_gyro_raw(raw_gyro);
        time_us = to_us_since_boot(get_absolute_time()) - time_us;
        printf("Takes %llu us to read raw accel and gyro readings separately\n"
               "Acc X = %d, Y = %d, Z = %d\nGyro X = %d, Y = %d, Z = %d\n",
               time_us, raw_accel[0], raw_accel[1], raw_accel[2],
               raw_gyro[0], raw_gyro[1], raw_gyro[2]);

        time_us = to_us_since_boot(get_absolute_time());
        mpu6050_read_accel(acceleration, testscale);
        mpu6050_read_gyro_rad(gyro, testscale);
        time_us = to_us_since_boot(get_absolute_time()) - time_us;
        printf("Takes %llu us to read and scale accel and gyro readings separately\n"
               "Acc [m/s^2] X = %f, Y = %f, Z = %f\n"
               "Gyro [rad/s] X = %f, Y = %f, Z = %f\n",
               time_us, acceleration[0], acceleration[1], acceleration[2],
               gyro[0], gyro[1], gyro[2]);

        time_us = to_us_since_boot(get_absolute_time());
        mpu6050_read(acceleration, gyro, &temperature, testscale, testscale);
        time_us = to_us_since_boot(get_absolute_time()) - time_us;
        printf("Takes %llu us to read and scale all together\n"
               "Acc [m/s^2] X = %f, Y = %f, Z = %f\n"
               "Gyro [rad/s] X = %f, Y = %f, Z = %f\n"
               "Temp [C] = %f\n----------------------\n",
               time_us, acceleration[0], acceleration[1], acceleration[2],
               gyro[0], gyro[1], gyro[2], temperature);

        sleep_ms(1000);
    }
    return 0;
}
