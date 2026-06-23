/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "pico/status_led.h"

// How long to wait
#ifndef AWAKE_TIME_MS
#define AWAKE_TIME_MS 10000
#endif
#ifndef SLEEP_TIME_MS
#define SLEEP_TIME_MS 10000
#endif

// Got to sleep and wakeup after 10 seconds
int main() {

    stdio_init_all();
    hard_assert(status_led_init());

    uint32_t count = 1;
    while(true) {
        status_led_set_state(true);

        printf("Wake up, test run: %u\n", count++);

        // Stay awake for a few seconds
        printf("Awake for %dms\n", AWAKE_TIME_MS);
        sleep_ms(AWAKE_TIME_MS);

        // go to sleep
        printf("Sleeping for %dms\n", SLEEP_TIME_MS);
        status_led_set_state(false);
        int rc = low_power_sleep_for_ms(SLEEP_TIME_MS, NULL, true);
        status_led_set_state(true);
        if (rc != PICO_OK) {
            printf("low_power_sleep_for_ms returned error %d\n", rc);
            hard_assert(false);
        }
    }
    return 0;
}
