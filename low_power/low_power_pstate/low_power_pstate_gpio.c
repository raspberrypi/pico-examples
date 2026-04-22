/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/powman.h"
#include "pico/low_power.h"
#include "pico/aon_timer.h"
#include "pico/status_led.h"

// GPIO to wait to go high, connect the other end to 3V3 OUT
#ifndef LOW_POWER_WAKE_GPIO
#define LOW_POWER_WAKE_GPIO 15
#endif

uint32_t __persistent_data(run_count);

// The example will repeatedly wait 10 seconds then switch off for 10 seconds
// The debugger will appear to be unresponsive while the device is off
int main() {
    stdio_init_all();

    hard_assert(status_led_init());
    status_led_set_state(true);

    // Scratch register survives power down
    printf("Wake up, test run: %u\n", run_count++);

    // Wait for gpio to go low
    if (gpio_get(LOW_POWER_WAKE_GPIO)) {
        printf("Awake until gpio %u goes low\n", LOW_POWER_WAKE_GPIO);
        while(gpio_get(LOW_POWER_WAKE_GPIO)) {
            tight_loop_contents();
        }
    }

    // power off
    printf("Low power until gpio %u goes high\n", LOW_POWER_WAKE_GPIO);
    status_led_set_state(false);
    int rc = low_power_pstate_until_gpio_pin_state(LOW_POWER_WAKE_GPIO, true, true, NULL, NULL);
    status_led_set_state(true);
    printf("low_power_pstate_until_aon_timer returned error %d\n", rc);
    hard_assert(false); // should never get here!
    return 0;
}
