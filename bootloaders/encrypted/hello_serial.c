/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"

int main() {
    enable_interrupts();
    stdio_init_all();
    while (true) {
        printf("Hello, world!\n");
        printf("I'm an encrypted binary\n");
        sleep_ms(1000);
    }
}
