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

enum BOARD_TYPE {
    BOARD_TYPE_PICO,    // Pico-series board
    BOARD_TYPE_PICO_W,  // Pico W-series board
    BOARD_TYPE_UNKNOWN,
};

// Detects if PICO_VSYS_PIN is actually connected to the VSYS voltage divider,
// to determine the board type.
// Also checks that the LED pin is low, which should be the case for both
// Pico-series and Pico W-series boards.
// This will work provided that the board is being powered from VSYS (i.e. it
// is using the onboard voltage regulator).
// This method is documented in section 2.4 of Connecting to the Internet with
// Raspberry Pi Pico W-series (https://pip.raspberrypi.com/documents/RP-008257-DS).
enum BOARD_TYPE detect_board_type(void) {
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
        // Pico W-series board
        return BOARD_TYPE_PICO_W;
    } else if (value == 0) {
        // Pico-series board
        return BOARD_TYPE_PICO;
    } else {
        // Unknown board
        return BOARD_TYPE_UNKNOWN;
    }
}

// Perform initialisation
int pico_led_init(enum BOARD_TYPE board_type) {
    if (board_type == BOARD_TYPE_PICO_W) {
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
void pico_set_led(bool led_on, enum BOARD_TYPE board_type) {
    if (board_type == BOARD_TYPE_PICO_W) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    } else {
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
    }
}

int main() {
    enum BOARD_TYPE board_type = detect_board_type();
    int rc = pico_led_init(board_type);
    hard_assert(rc == PICO_OK);
    while (true) {
        pico_set_led(true, board_type);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false, board_type);
        sleep_ms(LED_DELAY_MS);
    }
}
