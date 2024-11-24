#include "mpu6050.hpp"
#include "mpu6050_i2c.h"


MPU6050SensorTimingParams::MPU6050SensorTimingParams(int _bandwidth, float _delay, float _sample_rate)
    : bandwidth(_bandwidth), delay(_delay), sample_rate(_sample_rate) {}

inline const MPU6050SensorTimingParams convert(mpu6050_timing_params_t *c_struct) {  // unfortunate awkwardness of wrapping a C lib
    return MPU6050SensorTimingParams(c_struct->bandwidth, c_struct->delay, c_struct->sample_rate);
}

MPU6050TimingParams::MPU6050TimingParams(uint8_t lowpass_filter_cfg, uint8_t sample_rate_div) {
    mpu6050_timing_params_t accel_timing_c, gyro_timing_c;
    mpu6050_calc_timing(lowpass_filter_cfg, sample_rate_div, &accel_timing_c, &gyro_timing_c);
    accel_timing = convert(&accel_timing_c);
    gyro_timing = convert(&gyro_timing_c);
}

MPU6050TimingParams::MPU6050TimingParams(const MPU6050SensorTimingParams &_accel_timing,
                                         const MPU6050SensorTimingParams &_gyro_timing)
    : accel_timing(_accel_timing), gyro_timing(_gyro_timing) {}


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

void MPU6050::read_accel(void) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_read_accel(accel, (MPU6050_Scale)accel_scale);
}

void MPU6050::read_gyro(void) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_read_gyro_rad(gyro, (MPU6050_Scale)gyro_scale);
}

bool MPU6050::is_connected() {
    mpu6050_setbusaddr(bus_addr);
    return mpu6050_is_connected();
}

void MPU6050::set_timing(uint8_t lowpass_filter_cfg, uint8_t sample_rate_div) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_set_timing(lowpass_filter_cfg, sample_rate_div);
}

MPU6050TimingParams MPU6050::read_timing(void) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_timing_params_t accel_timing_c, gyro_timing_c;
    mpu6050_read_timing(&accel_timing_c, &gyro_timing_c);
    return MPU6050TimingParams(convert(&accel_timing_c), convert(&gyro_timing_c));
}

void MPU6050::configure_interrupt(bool active_low, bool open_drain, bool latch_pin, bool read_clear, bool enable) {
    mpu6050_setbusaddr(bus_addr);
    mpu6050_configure_interrupt(active_low, open_drain, latch_pin, read_clear, enable);
}

uint8_t MPU6050::read_interrupt_status() {
    mpu6050_setbusaddr(bus_addr);
    return mpu6050_read_interrupt_status();
}


float MPU6050_max_accel(MPU6050::Scale scale) {
    return accel_scale_vals[(int)scale];
}
float MPU6050_max_gyro_rad(MPU6050::Scale scale) {
    return gyro_scale_rad[(int)scale];
}
