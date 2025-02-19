/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"

int main() {
    restore_interrupts_from_disabled(0);
    stdio_init_all();
    while (true) {
        printf("Hello, world!\n");
        printf("I'm an encrypted binary\n");
        sleep_ms(1000);
    }
}
