/* header for accessing mpu6050 functions in pico-examples

Niraj Patel March 2022 */
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stddef.h"

typedef enum MPU6050_Scale {MPU_FS_0, MPU_FS_1, MPU_FS_2, MPU_FS_3} MPU6050_Scale;

// lower level functions for i2c
void mpu6050_writereg(uint8_t reg, uint8_t value); //write one byte to a register
//read length 8-bit integers from the register at reg, store them in *out.
void mpu6050_readreg(uint8_t reg, uint8_t *out, size_t length);
//read length 16-bit integers from the register at reg, store them in *out. Max length 32 (= 64 bytes)
void mpu6050_readreg16(uint8_t reg, int16_t *out, size_t length);
void mpu6050_setbusaddr(uint8_t addr); //set the i2c bus address for communication. MPU6050 must already have this value.

// higher level mpu6050 functions
/* Set the power and clock settings.
    CLKSEL is clock source, see docs. Recommended CLKSEL = 1 if gyro is enabled.
    temp_disable disables temperature, sleep enables sleep mode, cycle wakes up only when converting. */
void mpu6050_power(uint8_t CLKSEL, bool temp_disable, bool sleep, bool cycle);
// Reset power management and signal path registers. MPU6050 returns to default settings. Includes 200ms of wait.
void mpu6050_reset();
// Check whether MPU6050 is connected using the read-only WHO_AM_I register, which is always 0x68
bool mpu6050_is_connected();
// turn on the mpu6050's accelerometer self test. Turn it off after a few 100ms with mpu6050_setscale_accel
void mpu6050_accel_selftest_on();  // TODO: find out what "self test" does
// turn on the mpu6050's gyroscope self test. Turn it off after a few 100ms with mpu6050_setscale_gyro
void mpu6050_gyro_selftest_on();
//set and use scaling. The first read() after setscale() might not have the updated scaling.
void mpu6050_setscale_accel(MPU6050_Scale accel_scale);
void mpu6050_setscale_gyro(MPU6050_Scale gyro_scale);
void mpu6050_scale_accel(float accel[3], int16_t accel_raw[3], MPU6050_Scale scale); //sets accel[3] in m/s^2 from accel_raw[3]
void mpu6050_scale_gyro_deg(float gyro[3], int16_t gyro_raw[3], MPU6050_Scale scale); //sets gyro[3] in degrees from gyro_raw[3]
void mpu6050_scale_gyro_rad(float gyro[3], int16_t gyro_raw[3], MPU6050_Scale scale); //sets gyro[3] in radians from gyro_raw[3]
float mpu6050_scale_temp(float temp_raw);

// core IMU reading functions
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp); //read raw data values. Could read different samples.
void mpu6050_read(float accel[3], float gyro[3], float *temp,
                  MPU6050_Scale accel_scale, MPU6050_Scale gyro_scale); //reads all at same sample time, converts. temp can be NULL.

bool setup_MPU6050_i2c(); //call this before using any other functions
int run_MPU6050_demo();

#ifdef __cplusplus
}
#endif
