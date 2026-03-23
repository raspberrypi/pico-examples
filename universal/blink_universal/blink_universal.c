/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

bool detect_is_w_using_adc(void) {
    adc_init();
    adc_gpio_init(PICO_VSYS_PIN);
    adc_select_input(PICO_VSYS_PIN - ADC_BASE_PIN);
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    float voltage = result * conversion_factor;

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_IN);
    bool value = gpio_get(PICO_DEFAULT_LED_PIN);

    if (value == 0 && voltage < 0.1) {
        return true;
    } else {
        return false;
    }
}

// Perform initialisation
int pico_led_init(bool is_w) {
    if (is_w) {
        return cyw43_arch_init();
    } else {
        // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
        // so we can use normal GPIO functionality to turn the led on and off
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
        return PICO_OK;
    }
}

// Turn the led on or off
void pico_set_led(bool led_on, bool is_w) {
    if (is_w) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    } else {
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
    }
}

int main() {
    bool is_w = detect_is_w_using_adc();
    int rc = pico_led_init(is_w);
    hard_assert(rc == PICO_OK);
    while (true) {
        pico_set_led(true, is_w);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false, is_w);
        sleep_ms(LED_DELAY_MS);
    }
}
