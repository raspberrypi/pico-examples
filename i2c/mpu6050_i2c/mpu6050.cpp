#include "mpu6050.hpp"
#include "mpu6050_i2c.h"

/*MPU6050::MPU6050() :  //TODO: smart pointers
chip_temp(*temp) {
    accel = {0., 0., 0.};
    gyro = {0., 0., 0.};
    temp = {};
    MPU6050(accel, gyro, temp);
} */

MPU6050::MPU6050(float *accel_out, float *gyro_out, uint8_t addr) :
 chip_temp(temp), accel(accel_out),  gyro(gyro_out), accel_scale(Scale_0), gyro_scale(Scale_0), bus_addr(addr) {
    setup_MPU6050_i2c();
}

void MPU6050::power(uint8_t CLKSEL, bool temp_disable, bool sleep, bool cycle) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_power(CLKSEL, temp_disable, sleep, cycle);
}

void MPU6050::reset(void) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_reset();
}

void MPU6050::setscale_accel(MPU6050::Scale scale) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_setscale_accel((MPU6050_Scale)scale);
    accel_scale = scale;
}

void MPU6050::setscale_gyro(MPU6050::Scale scale) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_setscale_gyro((MPU6050_Scale)scale);
    gyro_scale = scale;
}

void MPU6050::read(void) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_read(accel, gyro, &temp, (MPU6050_Scale)accel_scale, (MPU6050_Scale)gyro_scale);
}

bool MPU6050::is_connected() {
    mpu6050_setbusaddr(bus_addr);
    return mpu6050_is_connected();
}
    