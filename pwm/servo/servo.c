#include "servo.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include <stdbool.h>

#define DUTY_MIN 2400
#define DUTY_MAX 8000

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void servo_put(int gpio, int angle, bool wait_write)
{
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint channel = pwm_gpio_to_channel(gpio);
    uint32_t top = pwm_hw->slice[slice].top;
    int duty_u16 = map(angle, 0, 180, DUTY_MIN, DUTY_MAX);
    uint32_t cc = duty_u16 * (top + 1) / 65535;
    pwm_set_chan_level(slice, channel, cc);
    pwm_set_enabled(slice, true);

    if(wait_write)
        sleep_ms(1000);
}

void servo_init(int gpio, int base_frequency)
{
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint channel = pwm_gpio_to_channel(gpio);


    uint32_t source_hz = clock_get_hz(clk_sys);
        #define TOP_MAX 65534
        uint freq = 50;
        uint32_t div16_top = 16 * source_hz / freq;
        uint32_t top = 1;
        for (;;) {
            if (div16_top >= 16 * 5 && div16_top % 5 == 0 && top * 5 <= TOP_MAX) {
                div16_top /= 5;
                top *= 5;
            } else if (div16_top >= 16 * 3 && div16_top % 3 == 0 && top * 3 <= TOP_MAX) {
                div16_top /= 3;
                top *= 3;
            } else if (div16_top >= 16 * 2 && top * 2 <= TOP_MAX) {
                div16_top /= 2;
                top *= 2;
            } else {
                break;
            }
        }
        pwm_hw->slice[slice].div = div16_top;
        pwm_hw->slice[slice].top = top;
}