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

// The example will repeatedly wait 10 seconds then switch off for 10 seconds
// The debugger will appear to be unresponsive while the device is off
int main() {
    stdio_init_all();
    // Must start the aon timer if needed
    if (!aon_timer_is_running()) {
        struct timespec ts = { .tv_sec = 1776858754, .tv_nsec = 0 };
        hard_assert(aon_timer_start(&ts));
    }
#if AWAKE_TIME_MS < 10000
    // pause for at least 10s to allow the debugger to attach on power up to allow the device to be re-programmed
    printf("Waiting a bit to allow debugger to attach\n");
    sleep_ms(10000 - AWAKE_TIME_MS);
#endif
    hard_assert(status_led_init());
    status_led_set_state(true);

    // Scratch register survives power down
    printf("Wake up, test run: %u\n", powman_hw->scratch[0]++);

    // Stay awake for a few seconds
    printf("Awake for %dms\n", AWAKE_TIME_MS);
    sleep_ms(AWAKE_TIME_MS);

    // power off
    printf("Low power for %dms\n", SLEEP_TIME_MS);
    status_led_set_state(false);
    absolute_time_t wakeup_time = delayed_by_ms(aon_timer_get_absolute_time(), SLEEP_TIME_MS); // note: MUST use aon_timer_get_absolute_time
    int rc = low_power_pstate_until_aon_timer(wakeup_time, NULL, NULL);
    status_led_set_state(true);
    printf("low_power_pstate_until_aon_timer returned error %d\n", rc);
    hard_assert(false); // should never get here!
    return 0;
}
