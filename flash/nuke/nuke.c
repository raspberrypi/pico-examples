/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Obliterate the contents of flash. This is a silly thing to do if you are
// trying to run this program from flash, so you should really load and run
// directly from SRAM. You can enable RAM-only builds for all targets by doing:
//
// cmake -DPICO_NO_FLASH=1 ..
//
// in your build directory. We've also forced no-flash builds for this app in
// particular by adding:
//
// pico_set_binary_type(flash_nuke no_flash)
//
// To the CMakeLists.txt app for this file. Just to be sure, we can check the
// define:
#if !PICO_NO_FLASH && !PICO_COPY_TO_RAM
#error "This example must be built to run from SRAM!"
#endif

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "pico/bootrom.h"

#define MIN_FLASH_SIZE (1024 * 1024 * 2)

int main() {
#ifdef PICO_DEFAULT_LED_PIN
    // Turn on LED to show erase starting
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
#endif

    uint flash_size_bytes = 0;
    uint8_t txbuf[4];
    uint8_t rxbuf[4];
    txbuf[0] = 0x9f;
    flash_do_cmd(txbuf, rxbuf, 4);
    flash_size_bytes = 1u << rxbuf[3];
    if (flash_size_bytes < MIN_FLASH_SIZE || flash_size_bytes > PICO_FLASH_SIZE_BYTES) {
        flash_size_bytes = PICO_FLASH_SIZE_BYTES;
    }
    flash_range_erase(0, flash_size_bytes);
    // Leave an eyecatcher pattern in the first page of flash so picotool can
    // more easily check the size:
    static const uint8_t eyecatcher[FLASH_PAGE_SIZE] = "NUKE";
    flash_range_program(0, eyecatcher, FLASH_PAGE_SIZE);

#ifdef PICO_DEFAULT_LED_PIN
    // Flash LED for success
    for (int i = 0; i < 3; ++i) {
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(100);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(100);
    }
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
#endif

    // Pop back up as an MSD drive
    reset_usb_boot(0, 0);
}
