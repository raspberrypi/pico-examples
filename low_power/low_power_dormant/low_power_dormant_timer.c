/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
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
#ifndef AWAKE_TIME_MS
#define AWAKE_TIME_MS 10000
#endif
#ifndef SLEEP_TIME_MS
#define SLEEP_TIME_MS 10000
#endif

#if PICO_RP2040
#ifndef RTC_CLOCK_SRC_GPIO_IN
#define RTC_CLOCK_SRC_GPIO_IN 20
#endif
#endif

#ifndef CLOCK_SOURCE
#if PICO_RP2040
#define CLOCK_SOURCE DORMANT_CLOCK_SOURCE_XOSC
#else
#define CLOCK_SOURCE DORMANT_CLOCK_SOURCE_LPOSC
#endif
#endif

// Got to sleep and wakeup after 5 seconds
// The example will repeatedly wait 10 seconds then switch off for 10 seconds
// The debugger will appear to be unresponsive while the device is off
int main() {
    stdio_init_all();
    // Must start aon timer
    low_power_start_aon_timer_at_time_ms(1776858754000);
#if AWAKE_TIME_MS < 10000
    // pause for at least 10s to allow the debugger to attach on power up to allow the device to be re-programmed
    printf("Waiting a bit to allow debugger to attach\n");
    sleep_ms(10000 - AWAKE_TIME_MS);
#endif
    hard_assert(status_led_init());
    uint32_t count = 1;
    while(true) {
        status_led_set_state(true);

        printf("Wake up, test run: %u\n", count++);

        // Stay awake for a few seconds
        printf("Awake for %dms\n", AWAKE_TIME_MS);
        sleep_ms(AWAKE_TIME_MS);

        // go dormant
        printf("Dormant for %dms\n", SLEEP_TIME_MS);
        status_led_set_state(false);

#if PICO_RP2040
        low_power_set_external_clock_source(RTC_CLOCK_FREQ_HZ, RTC_CLOCK_SRC_GPIO_IN);
#endif
        int rc = low_power_dormant_for_ms(SLEEP_TIME_MS, CLOCK_SOURCE, NULL);
        status_led_set_state(true);
        if (rc != PICO_OK) {
            printf("low_power_dormant_for_ms returned error %d\n", rc);
            hard_assert(false);
        }
    }
    return 0;
}
