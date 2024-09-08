/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

// Initialize the GPIO for the LED
void pico_led_init(void) {
#ifdef PICO_DEFAULT_LED_PIN
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

// Turn the LED on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#endif
}

int main() {
    pico_led_init();
    // note these frequencies are approximate
#if PICO_RP2040
    const int cpu_freq = 6500000;
#else
    const int cpu_freq = 12000000;
#endif
    while (true) {
        pico_set_led(true);
        // don't have clocks initialized wo wait based on CPU cycles
        busy_wait_at_least_cycles((cpu_freq / 1000) * LED_DELAY_MS);
        pico_set_led(false);
        busy_wait_at_least_cycles((cpu_freq / 1000) * LED_DELAY_MS);
    }
}
