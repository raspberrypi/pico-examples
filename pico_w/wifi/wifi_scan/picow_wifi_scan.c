/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) {
        printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
            result->ssid, result->rssi, result->channel,
            result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
            result->auth_mode);
    }
    return 0;
}

// Start a wifi scan
static void scan_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    cyw43_wifi_scan_options_t scan_options = {0};
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);
    if (err == 0) {
        bool *scan_started = (bool*)worker->user_data;
        *scan_started = true;
        printf("\nPerforming wifi scan\n");
    } else {
        printf("Failed to start scan: %d\n", err);
    }
}

#include "hardware/vreg.h"
#include "hardware/clocks.h"

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }

    printf("Press 'q' to quit\n");
    cyw43_arch_enable_sta_mode();

    // Start a scan immediately
    bool scan_started = false;
    async_at_time_worker_t scan_worker = { .do_work = scan_worker_fn, .user_data = &scan_started };
    hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &scan_worker, 0));

    bool exit = false;
    while(!exit) {
        int key = getchar_timeout_us(0);
        if (key == 'q' || key == 'Q') {
            exit = true;
        }
        if (!cyw43_wifi_scan_active(&cyw43_state) && scan_started) {
            // Start a scan in 10s
            scan_started = false;
            hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &scan_worker, 10000));
        }
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(at_the_end_of_time);
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }

    cyw43_arch_deinit();
    return 0;
}
