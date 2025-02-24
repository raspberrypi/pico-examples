/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/sync.h"

int main() {
    enable_interrupts();
    stdio_init_all();

#if PICO_CRT0_IMAGE_TYPE_TBYB
    // If TBYB image, then buy it
    uint8_t* buffer = malloc(4096);
    rom_explicit_buy(buffer, 4096);
    free(buffer);
#endif

    while (true) {
        printf("Hello, world!\n");
        printf("I'm a self-decrypting binary\n");
        printf("My secret is...\n");
        sleep_ms(1000);
    }
}
