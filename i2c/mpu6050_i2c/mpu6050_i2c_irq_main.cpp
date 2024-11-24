/*
 * MPU6050 I2C example measuring the interrupt pin and lowpass filter.
 */

#include <stdio.h>
#include <memory>
#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "mpu6050.hpp"

const bool USE_READ_CLEAR = true;  // true => fewer reads required, eliminates a failure mode (forgetting to clear irq)
const unsigned READ_TIME_MS = 8000;
const uint8_t IRQ_PIN = 6;
const uint8_t SAMPLE_RATE_DIVS[] = {99, 9};
const uint8_t LOWPASS_FILTER_CFGS[] = {5, 1};
volatile bool got_irq;

static std::unique_ptr<MPU6050> IMU;
static float accel[3] = {0}, gyro[3] = {0};

void print_SensorTimingParams(const MPU6050SensorTimingParams &params) {
    printf("Sample Rate: %.2f, Bandwidth: %i, Delay: %.1f\n", params.sample_rate, params.bandwidth, params.delay);
}

void print_TimingParams(const MPU6050TimingParams &params) {
    printf("Accelerometer ");
    print_SensorTimingParams(params.accel_timing);
    printf("Gyroscope ");
    print_SensorTimingParams(params.gyro_timing);
}

void irq_callback(uint gpio, uint32_t events) {
    static unsigned num_reads = 0, sample_rate_div_idx = 0, lowpass_filter_cfg_idx = 0;
    static float last_accel[3] = {0}, last_gyro[3] = {0};
    static uint64_t last_time_us = 0;
    static float avg_change_acc = 0, avg_change_gyro = 0;
    got_irq = true;
    if (!USE_READ_CLEAR) {
        IMU->read_interrupt_status();
    }
    IMU->read();
    float change_acc = 0, change_gyro = 0;
    for (int i = 0; i < 3; i++) {
        change_acc += (accel[i] - last_accel[i]) * (accel[i] - last_accel[i]);
        change_gyro += (gyro[i] - last_gyro[i]) * (gyro[i] - last_gyro[i]);
        last_accel[i] = accel[i];
        last_gyro[i] = gyro[i];
    }
    avg_change_acc += sqrtf(change_acc);
    avg_change_gyro += sqrtf(change_gyro);
    if (++num_reads > READ_TIME_MS / (SAMPLE_RATE_DIVS[sample_rate_div_idx] + 1)) {
        uint64_t time_us = to_us_since_boot(get_absolute_time());
        printf("Measured cycle rate: %f Hz\n", (1e6 * num_reads) / (time_us - last_time_us));
        last_time_us = time_us;
        printf("Average change between readings:\n\t accel: %f mm/s2 gyro: %f mrad/s\n\n",
               1000 * avg_change_acc / num_reads, 1000 * avg_change_gyro / num_reads);
        avg_change_acc = 0;
        avg_change_gyro = 0;
        num_reads = 0;
        if (lowpass_filter_cfg_idx == 1)
            sample_rate_div_idx = 1 - sample_rate_div_idx;
        lowpass_filter_cfg_idx = 1 - lowpass_filter_cfg_idx;
        IMU->set_timing(LOWPASS_FILTER_CFGS[lowpass_filter_cfg_idx], SAMPLE_RATE_DIVS[sample_rate_div_idx]);
        print_TimingParams(IMU->read_timing());
    }
}

int main() {
    stdio_init_all();
    bi_decl(bi_1pin_with_name(IRQ_PIN, "IMU IRQ pin 1"));
    IMU = std::make_unique<MPU6050>(accel, gyro);
    IMU->reset();
    //setting CLKSEL = 1 gets better results than 0 if gyro is running and no sleep mode
    IMU->power(1, false, false, false);
    // push-pull is faster, latch allows debugging with the INT pin
    IMU->configure_interrupt(false, false, true, USE_READ_CLEAR, true);
    IMU->set_timing(LOWPASS_FILTER_CFGS[0], SAMPLE_RATE_DIVS[0]);
    print_TimingParams(IMU->read_timing());
    gpio_set_irq_enabled_with_callback(IRQ_PIN, GPIO_IRQ_LEVEL_HIGH, true, &irq_callback);
    while (true) {
        got_irq = false;
        sleep_ms(3000);
        if (!IMU->is_connected()) {
            printf("MPU6050 is not connected...\n");
        } else if (!got_irq){
            printf("MPU6050 is connected, but didn't trigger an interrupt\n");
            IMU->read();
            printf("Read accel and gyro without IRQ:\n"
                   "%+6f %+6f %+6f | %+6f %+6f %+6f\n",
                   accel[0], accel[1], accel[2], gyro[0], gyro[1], gyro[2]);
        }
    }
}
