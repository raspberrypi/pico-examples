/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const uint LED_PIN = 25;
const uint DOT_PERIOD_MS = 100;

const char *morse_letters[] = {
        ".-",    // A
        "-...",  // B
        "-.-.",  // C
        "-..",   // D
        ".",     // E
        "..-.",  // F
        "--.",   // G
        "....",  // H
        "..",    // I
        ".---",  // J
        "-.-",   // K
        ".-..",  // L
        "--",    // M
        "-.",    // N
        "---",   // O
        ".--.",  // P
        "--.-",  // Q
        ".-.",   // R
        "...",   // S
        "-",     // T
        "..-",   // U
        "...-",  // V
        ".--",   // W
        "-..-",  // X
        "-.--",  // Y
        "--.."   // Z
};

void put_morse_letter(const char *pattern) {
    for (; *pattern; ++pattern) {
        gpio_put(LED_PIN, 1);
        if (*pattern == '.')
            sleep_ms(DOT_PERIOD_MS);
        else
            sleep_ms(DOT_PERIOD_MS * 3);
        gpio_put(LED_PIN, 0);
        sleep_ms(DOT_PERIOD_MS * 1);
    }
    sleep_ms(DOT_PERIOD_MS * 2);
}

void put_morse_str(const char *str) {
    for (; *str; ++str) {
        if (*str >= 'A' && *str < 'Z') {
            put_morse_letter(morse_letters[*str - 'A']);
        } else if (*str >= 'a' && *str < 'z') {
            put_morse_letter(morse_letters[*str - 'a']);
        } else if (*str == ' ') {
            sleep_ms(DOT_PERIOD_MS * 4);
        }
    }
}

int main() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        put_morse_str("Hello world");
        sleep_ms(1000);
    }
}
