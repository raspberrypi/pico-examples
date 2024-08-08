/**
 * Boilerplate main for mpu6050 I2c example
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "mpu6050_i2c.h"


int main() {
    stdio_init_all();
    if (setup_MPU6050_i2c()) {
        printf("Hello, MPU6050! Reading raw data from registers...\n");
    }
    return run_MPU6050_demo();
}
