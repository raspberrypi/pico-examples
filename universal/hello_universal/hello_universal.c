/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "boot/picoboot.h"

int main() {
    stdio_init_all();
#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    bool toggle = true;
    gpio_put(PICO_DEFAULT_LED_PIN, toggle);
#endif
    uint reboot_count = 0;
    while (reboot_count < 10) {
        printf("Hello, world!\n");
    #if PICO_RP2350
        printf("I'm an RP2350 ");
        #ifdef __riscv
            printf("running RISC-V\n");
        #else
            printf("running ARM\n");
        #endif
        reboot_count++;
    #else
        printf("I'm an RP2040\n");
    #endif

    #ifdef PICO_DEFAULT_LED_PIN
        toggle = !toggle;
        gpio_put(PICO_DEFAULT_LED_PIN, toggle);
        sleep_ms(1000);
    #endif
    }

#if PICO_RP2350
    #ifdef __riscv
    rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_NORMAL | REBOOT2_FLAG_REBOOT_TO_ARM, 1000, 0, 0);
    #else
    rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_NORMAL | REBOOT2_FLAG_REBOOT_TO_RISCV, 1000, 0, 0);
    #endif
    printf("Rebooting to other architecture\n");
#endif
}
