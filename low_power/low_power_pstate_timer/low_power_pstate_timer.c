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

// Got to sleep and wakeup after 5 seconds
// The example will repeatedly wait 10 seconds then switch off for 10 seconds
// The debugger will appear to be unresponsive while the device is off
int main() {

    stdio_init_all();
    hard_assert(status_led_init());
    status_led_set_state(true);

    // Scratch register survives power down
    printf("Wake up, test run: %u\n", powman_hw->scratch[0]++);
    printf("Current power state: 0x%x\n", powman_get_power_state());

    // Stay awake for a few seconds
    printf("Awake for %dms\n", AWAKE_TIME_MS);
    sleep_ms(AWAKE_TIME_MS);

    powman_set_debug_power_request_ignored(true);
    powman_timer_start();

    // power off
    printf("Sleep for %dms\n", SLEEP_TIME_MS);
    status_led_set_state(false);
    absolute_time_t start_time = aon_timer_get_absolute_time();
    absolute_time_t wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    int rc = low_power_pstate_until_aon_timer(wakeup_time, NULL, NULL);
    status_led_set_state(true);
    printf("low_power_pstate_until_aon_timer returned error %d\n", rc);
    hard_assert(false); // should never get here!
    return 0;
}
