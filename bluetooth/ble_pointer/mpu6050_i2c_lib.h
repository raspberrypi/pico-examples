#include <stdint.h>

#ifndef MPU6050_I2C_LIB
#define MPU6050_I2C_LIB

void mpu6050_reset(void);
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp);
int mpu6050_initialise(int SDA_pin, int SCL_pin, int GYRO_FS, int ACCEL_FS);
void mpu6050_get_gyro_offset(int num, float *roll_offset, float *pitch_offset, float *yaw_offset, int gyro_fs);
void mpu6050_fusion_output(float roll_offset, float pitch_offset, float yaw_offset, float alpha, int delta_ms, float *roll, float *pitch, float *yaw, int gyro_fs);

#endif