/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"

// This is a regular old LED blinking example, however it is linked with the
// `pico_bootsel_via_double_reset` library, so resetting the board twice
// quickly will enter the USB bootloader. This is useful for boards which have
// a reset button but no BOOTSEL, as long as you remember to always link the
// `pico_bootsel_via_double_reset` library!

int main() {
#ifdef PICO_DEFAULT_LED_PIN
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
#else
    while (true) {
        sleep_ms(250);
    }
#endif
}
