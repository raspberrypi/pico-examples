/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"

int main() {
    stdio_init_all();
    static uint8_t flash_buffer[0x1000] = {};
    cflash_flags_t flags;
    flags.flags = (CFLASH_OP_VALUE_READ << CFLASH_OP_LSB) | (CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB);
    int ret;

    // Unpartitioned space only has bootloader permissions, so secure reads will fail
    printf("Attempt to read start of flash\n");
    ret = rom_flash_op(flags, XIP_BASE, sizeof(flash_buffer), flash_buffer);
    if (ret == BOOTROM_ERROR_NOT_PERMITTED) {
        printf("Not permitted, as expected\n");
    } else {
        printf("ERROR: ");
        if (ret) {
            printf("Flash OP failed with %d\n", ret);
        } else {
            printf("Flash OP succeeded, when shouldn't have been permitted");
        }
    }

    // Add a runtime partition at start of flash with all permissions, to allow access to that region of flash
    printf("Adding partition with all permissions\n");
    ret = rom_add_flash_runtime_partition(0, 0x1000, PICOBIN_PARTITION_PERMISSIONS_BITS);
    if (ret != 0) {
        printf("ERROR: Failed to add runtime partition\n");
    }

    // Reads from that partition will now succeed
    printf("Attempt to read start of flash\n");
    ret = rom_flash_op(flags, XIP_BASE, sizeof(flash_buffer), flash_buffer);
    if (ret == 0) {
        printf("Read successful - read first 2 words %08x %08x\n", ((uint32_t*)flash_buffer)[0], ((uint32_t*)flash_buffer)[1]);
    } else {
        printf("ERROR: Flash OP failed with %d\n", ret);
    }

    // But reads from unpartitioned space will still fail
    printf("Attempt to read later in flash\n");
    ret = rom_flash_op(flags, XIP_BASE + 0x1000, sizeof(flash_buffer), flash_buffer);
    if (ret == BOOTROM_ERROR_NOT_PERMITTED) {
        printf("Not permitted, as expected\n");
    } else {
        printf("ERROR: ");
        if (ret) {
            printf("Flash OP failed with %d\n", ret);
        } else {
            printf("Flash OP succeeded, when it shouldn't have been permitted - read first 2 words %08x %08x\n", ((uint32_t*)flash_buffer)[0], ((uint32_t*)flash_buffer)[1]);
        }
    }
}
