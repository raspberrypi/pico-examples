/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * 8-channel Radio Control PPM example
 * Output on GPIO3
 *

 *  synchro     /  ch0 value 1-2ms  \/  ch0 value 1-2ms  \   /  ch8 value 1-2ms  \
 *  >=5ms       _______    -500ms     _______                 _______              _______
 * _______...__| 0.5ms |_____________| 0.5ms |___________|...| 0.5ms |____________| 0.5ms |_
 */
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "ppm.pio.h"

void set_value_and_log(uint channel, uint value_usec) {
    printf("Channel %d is set to value %5.3f ms\r\n", channel, (float)value_usec / 1000.0f);
    ppm_set_value(channel, value_usec);

}
int main() {
    setup_default_uart();
    uint pin = 3;
    ppm_program_init(pio0, pin);
    while (1) {
        set_value_and_log(1, 1100);
        set_value_and_log(8, 1800);
        sleep_ms(1000);
        set_value_and_log(1, 1700);
        set_value_and_log(8, 1200);
        sleep_ms(1000);
    }
}

