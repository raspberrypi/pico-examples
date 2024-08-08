/* class for using an MPU6050 IMU sensor on pi pico.
An abstraction of pico example C code. 
TODO:
    smart ptrs for results
    set timing
    set and read filter cutoff
    interrupts
 */
#include "stdint.h"

class MPU6050 {
public:
    const float &chip_temp; // [C]

    MPU6050(); //TODO
    MPU6050(float *accel_out, float *gyro_out, uint8_t i2c_addr=0x68); // [m/s^2], [rad/s]

    void power(uint8_t CLKSEL, bool temp_disable, bool sleep, bool cycle);
    void reset(void);

    enum Scale {Scale_0 = 0, Scale_1, Scale_2, Scale_3};
    // Warning: The first call to read() after setscale() might not have the updated scaling.
    void setscale_accel(Scale scale); //scale 0-3 is 2g, 4g, 8g, or 16g
    void setscale_gyro(Scale scale); // scale 0-3 is 250, 500, 1000, or 2000 deg/s
    void read(void);
    bool is_connected(void);
private:
    float *accel, *gyro, temp;
    enum Scale accel_scale, gyro_scale;
    uint8_t bus_addr;
};
