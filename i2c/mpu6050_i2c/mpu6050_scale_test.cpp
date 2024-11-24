/**
 * mpu6050 I2c C++ example demonstrating the mpu6050's scaling feature
 * Scaled Accelerometer and Gyroscope readings
 * Should be roughly equal before and after scale changes
 */

#include <stdio.h>
#include "mpu6050.hpp"
#include "pico/stdlib.h"

const uint8_t READ_DELAY_MS = 100, READS_PER_SCALE = 50, PRINTS_PER_SCALE = 3;

int main() {
    stdio_init_all();
    float accel[3] = {0}, gyro[3] = {0};
    MPU6050 *IMU = new MPU6050(accel, gyro);
    IMU->reset();
    //setting CLKSEL = 1 gets better results than 0 if gyro is running and no sleep mode
    IMU->power(1, false, false, false);
    IMU->set_timing(6, READ_DELAY_MS - 1); // DLPF = 5 Hz
    int accel_scale = 0, gyro_scale = 2;
    while (true) {
        while (!IMU->is_connected()) {
            printf("MPU6050 is not connected...\n");
            sleep_ms(1000);
        }
        printf("\nAccelerometer scale: %d, Gyroscope scale: %d\n", accel_scale, gyro_scale);
        for (int i = 0; i < READS_PER_SCALE; i++) {
            IMU->read();
            printf("%+6f %+6f %+6f | %+6f %+6f %+6f\r",
                   accel[0], accel[1], accel[2], gyro[0], gyro[1], gyro[2]);
            if (i < PRINTS_PER_SCALE - 1) { printf("\n"); }
            sleep_ms(READ_DELAY_MS);
        }
        accel_scale = (accel_scale + 1) % 4;
        gyro_scale = (gyro_scale + 1) % 4;
        IMU->setscale_accel((MPU6050::Scale)(accel_scale));
        IMU->setscale_gyro((MPU6050::Scale)(gyro_scale));
    }
}
