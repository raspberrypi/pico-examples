/* mpu6050 functions in pico-examples */
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stddef.h"

typedef enum MPU6050_Scale {MPU_FS_0 = 0, MPU_FS_1, MPU_FS_2, MPU_FS_3} MPU6050_Scale;

typedef struct {
    int bandwidth; // lowpass filter bandwidth [Hz]
    float delay; // lowpass filter delay [ms]
    float sample_rate; // rate of new data loading in the register [Hz]
} mpu6050_timing_params_t;

bool setup_MPU6050_i2c(); //call this before using any other functions
void mpu6050_setbusaddr(uint8_t addr); //set the i2c bus address for communication. MPU6050 must already have this value.

/* Set the power and clock settings.
    CLKSEL is clock source, see docs. Recommended CLKSEL = 1 if gyro is enabled.
    temp_disable disables temperature, sleep enables sleep mode, cycle wakes up only when converting. */
void mpu6050_power(uint8_t CLKSEL, bool temp_disable, bool sleep, bool cycle);
// MPU6050 returns to default settings and enters sleep mode. Includes 200ms of wait.
void mpu6050_reset();
// Check whether MPU6050 is connected using the read-only WHO_AM_I register, which is always 0x68
bool mpu6050_is_connected();
// turn on the mpu6050's accelerometer self test. Turn it off after a few 100ms with mpu6050_setscale_accel
void mpu6050_accel_selftest_on();  // TODO: find out what "self test" does
// turn on the mpu6050's gyroscope self test. Turn it off after a few 100ms with mpu6050_setscale_gyro
void mpu6050_gyro_selftest_on();

// configure lowpass filter and sample rate. Higher filter_cfg -> slower change, higher sample_rate_div -> slower sampling
void mpu6050_set_timing(uint8_t lowpass_filter_cfg, uint8_t sample_rate_div);
void mpu6050_read_timing(mpu6050_timing_params_t *accel_timing, mpu6050_timing_params_t *gyro_timing);
void mpu6050_calc_timing(uint8_t filter_cfg, uint8_t sample_rate_div,
                         mpu6050_timing_params_t *accel_timing, mpu6050_timing_params_t *gyro_timing);

void mpu6050_configure_interrupt(bool active_low, // Whether the INT pin is active low or active high
                                 bool open_drain, // Whether the INT pin is push-pull or open-drain
                                 bool latch_pin,  // Whether the INT pin latches or pulses for 50us
                                 bool read_clear, // Whether interrupt status bits are cleared by reading interrupt status (default) or on any read
                                 bool enable);    // Turn interrupts on or off
uint8_t mpu6050_read_interrupt_status(); // 0 =  no interrupts set, 1 = data ready

//set and use scaling. The first read() after setscale() might not have the updated scaling.
void mpu6050_setscale_accel(MPU6050_Scale accel_scale);
void mpu6050_setscale_gyro(MPU6050_Scale gyro_scale);
extern const float gyro_scale_deg[4], gyro_scale_rad[4], accel_scale_vals[4]; // scale constants, index with MPU6050_Scale
void mpu6050_scale_accel(float accel[3], int16_t accel_raw[3], MPU6050_Scale scale); //sets accel[3] in m/s^2 from accel_raw[3]
void mpu6050_scale_gyro_deg(float gyro[3], int16_t gyro_raw[3], MPU6050_Scale scale); //sets gyro[3] in degrees/s from gyro_raw[3]
void mpu6050_scale_gyro_rad(float gyro[3], int16_t gyro_raw[3], MPU6050_Scale scale); //sets gyro[3] in radians/s from gyro_raw[3]
float mpu6050_scale_temp(float temp_raw);

// core sensor data reading functions
void mpu6050_read_accel_raw(int16_t accel[3]);
void mpu6050_read_gyro_raw(int16_t gyro[3]);
void mpu6050_read(float accel[3], float gyro_rad[3], float *temp,
                  MPU6050_Scale accel_scale, MPU6050_Scale gyro_scale); //reads all at same sample time, converts. temp can be NULL.
void mpu6050_read_accel(float accel[3], MPU6050_Scale accel_scale);
void mpu6050_read_gyro_rad(float gyro[3], MPU6050_Scale gyro_scale);
void mpu6050_read_gyro_deg(float gyro[3], MPU6050_Scale gyro_scale);

#ifdef __cplusplus
}
#endif
