/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/status_led.h"

#if !PICO_COLORED_STATUS_LED_AVAILABLE
#warning The color_blink example requires a board with a WS2812 LED
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

int main() {
    bool rc = status_led_init();
    hard_assert(rc);
    hard_assert(colored_status_led_supported()); // This assert fails if your board does not have WS2812 support
    uint32_t count = 0;
    while (true) {
        // flash red then green then blue
        uint32_t color = PICO_COLORED_STATUS_LED_COLOR_FROM_RGB(count % 3 == 0 ? 0xaa : 0, count % 3 == 1 ? 0xaa : 0, count % 3 == 2 ? 0xaa : 0);
        colored_status_led_set_on_with_color(color);
        count++;
        sleep_ms(LED_DELAY_MS);
        assert(colored_status_led_get_state());
        colored_status_led_set_state(false);
        sleep_ms(LED_DELAY_MS);
        assert(!colored_status_led_get_state());
    }
    status_led_deinit();
}
