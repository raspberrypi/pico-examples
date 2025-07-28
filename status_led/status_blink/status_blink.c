/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/status_led.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

int main() {
    bool rc = status_led_init();
    hard_assert(rc);
    while (true) {
        status_led_set_state(true);
        sleep_ms(LED_DELAY_MS);
        assert(status_led_get_state());
        status_led_set_state(false);
        sleep_ms(LED_DELAY_MS);
        assert(!status_led_get_state());
    }
    status_led_deinit();
}
