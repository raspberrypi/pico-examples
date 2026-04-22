/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "pico/status_led.h"

// How long to wait
#define AWAKE_TIME_MS 10000
#define SLEEP_TIME_MS 10000

// Got to sleep and wakeup after 10 seconds
int main() {

    stdio_init_all();
    hard_assert(status_led_init());

    uint32_t count = 1;
    while(true) {
        status_led_set_state(true);

        // Scratch register survives power down
        printf("Wake up, test run: %u\n", count++);

        // Stay awake for a few seconds
        printf("Awake for %dms\n", AWAKE_TIME_MS);
        sleep_ms(AWAKE_TIME_MS);

        // power off
        printf("Sleep for %dms\n", SLEEP_TIME_MS);
        status_led_set_state(false);
        absolute_time_t start_time = get_absolute_time();
        absolute_time_t wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
        
        int rc = low_power_sleep_until_default_timer(wakeup_time, NULL, true);
        status_led_set_state(true);
        if (rc != PICO_OK) {
            printf("low_power_sleep_until_default_timer returned error %d\n", rc);
            hard_assert(false);
        }
    }
    return 0;
}
