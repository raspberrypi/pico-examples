/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * 8-channel Radio Control PPM example
 * Output on GPIO3
 *
 * 0.5ms   5ms         0.5ms      1-2mc      0.5ms      1-2mc
 * strobe  synchro    strobe     ch0 value  strobe     ch1 value
 * ______              _______               _______
 *       |_______...__|       |_____________|       |___________...
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

