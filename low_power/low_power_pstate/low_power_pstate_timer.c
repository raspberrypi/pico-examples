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

// How long to wait
#ifndef AWAKE_TIME_MS
#define AWAKE_TIME_MS 10000
#endif
#ifndef SLEEP_TIME_MS
#define SLEEP_TIME_MS 10000
#endif

static uint32_t __persistent_data(run_count);
static bool __persistent_data(aon_timer_started);

// The example will repeatedly wait 10 seconds then switch off for 10 seconds
// The debugger will appear to be unresponsive while the device is off
int main() {
    stdio_init_all();
    // Must start the aon timer if needed
    printf("Current time: %llu\n", aon_timer_get_absolute_time());
    if (!aon_timer_started) {
        low_power_start_aon_timer_at_time_ms(1776858754000);
        aon_timer_started = true;
    }
#if AWAKE_TIME_MS < 10000
    // pause for at least 10s to allow the debugger to attach on power up to allow the device to be re-programmed
    printf("Waiting a bit to allow debugger to attach\n");
    sleep_ms(10000 - AWAKE_TIME_MS);
#endif
    hard_assert(status_led_init());
    status_led_set_state(true);

    // Scratch register survives power down
    printf("Wake up, test run: %u\n", run_count++);

    // Stay awake for a few seconds
    printf("Awake for %dms\n", AWAKE_TIME_MS);
    sleep_ms(AWAKE_TIME_MS);

    // power off
    printf("Low power for %dms\n", SLEEP_TIME_MS);
    status_led_set_state(false);
    int rc = low_power_pstate_for_ms(SLEEP_TIME_MS, NULL, NULL);
    status_led_set_state(true);
    printf("low_power_pstate_for_ms returned error %d\n", rc);
    hard_assert(false); // should never get here!
    return 0;
}
