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
    boot_info_t boot_info = {};
    int ret = rom_get_boot_info(&boot_info);
    if (ret) {
        // BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING will always be set, but check anyway
        if (boot_info.tbyb_and_update_info & BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING) {
            // Need to check flash_update_base is set to see if this is a TBYB update
            uint32_t flash_update_base = boot_info.reboot_params[0];
            if (flash_update_base) {
                printf("Perform self-check... ");
                if (1 == 1) {   // replace this with your actual self-check function
                    printf("passed\n");
                } else {
                    printf("failed - looping forever\n");
                    while (true) sleep_ms(1000);
                }
            }
            uint32_t buf_size = flash_update_base ? 4096 : 0;
            uint8_t* buffer = flash_update_base ? malloc(buf_size) : NULL;
            int ret = rom_explicit_buy(buffer, buf_size);
            assert(ret == 0);
            if (buffer) free(buffer);
        }
    }
#endif
    extern char secret_data[];

    while (true) {
        printf("Hello, world!\n");
        printf("I'm a self-decrypting binary\n");
        printf("My secret is...\n");
        sleep_ms(1000);
        printf(secret_data);
        sleep_ms(10000);
    }
}
