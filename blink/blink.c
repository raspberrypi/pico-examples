/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include <stdio.h>

int main() {
    stdio_init_all();
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        printf("LED ON!\n");
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        printf("LED OFF!\n");
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}

