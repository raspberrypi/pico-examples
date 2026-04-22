/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "pico/aon_timer.h"
#include "pico/status_led.h"

#ifdef PICO_RP2350
#include "hardware/powman.h"
#endif

// How long to wait
#define AWAKE_TIME_MS 10000
#define SLEEP_TIME_MS 10000

#ifndef LOW_POWER_CLKSRC_GPIO_IN
#define LOW_POWER_CLKSRC_GPIO_IN 20
#endif

// Got to sleep and wakeup after 5 seconds
// The example will repeatedly wait 10 seconds then switch off for 10 seconds
// The debugger will appear to be unresponsive while the device is off
int main() {
    stdio_init_all();
#if AWAKE_TIME_MS < 10000
    // pause for at least 10s to allow the debugger to attach on power up to allow the device to be re-programmed
    printf("Waiting a bit to allow debugger to attach\n");
    sleep_ms(10000 - AWAKE_TIME_MS);
#endif

    hard_assert(status_led_init());

    struct timespec ts = { .tv_sec = 1723124088, .tv_nsec = 0 };
    aon_timer_start(&ts);

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
        absolute_time_t start_time = aon_timer_get_absolute_time();
        absolute_time_t wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);

        int rc = low_power_dormant_until_aon_timer(wakeup_time,
#if PICO_RP2040
            DORMANT_CLOCK_SOURCE_XOSC, RTC_CLOCK_FREQ_HZ,
#else
            DORMANT_CLOCK_SOURCE_LPOSC, XOSC_HZ,
#endif
            LOW_POWER_CLKSRC_GPIO_IN, NULL);
        status_led_set_state(true);
        if (rc != PICO_OK) {
            printf("low_power_dormant_until_aon_timer returned error %d\n", rc);
            hard_assert(false);
        }
    }
    return 0;
}
