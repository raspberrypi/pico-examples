/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "pico/status_led.h"

// GPIO to wait to go high, connect the other end to 3V3 OUT
#ifndef LOW_POWER_WAKE_GPIO
#define LOW_POWER_WAKE_GPIO 15
#endif

#ifndef CLOCK_SOURCE
#if PICO_RP2040
#define CLOCK_SOURCE DORMANT_CLOCK_SOURCE_XOSC
#else
#define CLOCK_SOURCE DORMANT_CLOCK_SOURCE_LPOSC
#endif
#endif

// Got to sleep and wakeup when gpio goes high
int main() {

    stdio_init_all();
    hard_assert(status_led_init());
    gpio_init(LOW_POWER_WAKE_GPIO);
    printf("State of gpio %u is %u\n", LOW_POWER_WAKE_GPIO, gpio_get(LOW_POWER_WAKE_GPIO));

    uint32_t count = 1;
    while(true) {
        status_led_set_state(true);

        printf("Wake up, test run: %u\n", count++);

        // Wait for gpio to go low
        if (gpio_get(LOW_POWER_WAKE_GPIO)) {
            printf("Awake until gpio %u goes low\n", LOW_POWER_WAKE_GPIO);
            while(gpio_get(LOW_POWER_WAKE_GPIO)) {
                tight_loop_contents();
            }
        }

        // go dormant
        printf("Dormant until gpio %u goes high\n", LOW_POWER_WAKE_GPIO);
        status_led_set_state(false);
        
        int rc = low_power_dormant_until_gpio_pin_state(LOW_POWER_WAKE_GPIO, true, true, CLOCK_SOURCE, NULL);
        status_led_set_state(true);
        if (rc != PICO_OK) {
            printf("low_power_dormant_until_aon_timer returned error %d\n", rc);
            hard_assert(false);
        }
    }
    return 0;
}
