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

// How long to wait
#define AWAKE_TIME_MS 10000
#define SLEEP_TIME_MS 10000

#define RTC_GPIO 22

// Got to sleep and wakeup after 5 seconds
// The example will repeatedly wait 10 seconds then switch off for 10 seconds
// The debugger will appear to be unresponsive while the device is off
int main() {

    stdio_init_all();
    hard_assert(status_led_init());

    //struct timespec ts = { .tv_sec = 1723124088, .tv_nsec = 0 };
    //aon_timer_start(&ts);
    //powman_timer_set_1khz_tick_source_xosc();

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

        /*clock_dest_bitset_t keep_enabled = clock_dest_bitset_none();
        clock_dest_bitset_add(&keep_enabled, CLK_DEST_REF_TICKS);
        clock_dest_bitset_add(&keep_enabled, CLK_DEST_SYS_TIMER0);*/
        
        int rc = low_power_sleep_until_default_timer(wakeup_time, NULL, true);
        status_led_set_state(true);
        printf("low_power_sleep_until_default_timer returned error %d\n", rc);
    }
    return 0;
}
