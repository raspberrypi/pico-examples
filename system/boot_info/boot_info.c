/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"

static void print_partition_name(int8_t partition) {
    switch (partition) {
        case BOOT_PARTITION_NONE: {
            printf("\tnone\n");
            break;
        }
        case BOOT_PARTITION_SLOT0: {
            printf("\tslot0\n");
            break;
        }
        case BOOT_PARTITION_SLOT1: {
            printf("\tslot1\n");
            break;
        }
        case BOOT_PARTITION_WINDOW: {
            printf("\twindow\n");
            break;
        }
        default: {
            printf("\tunknown\n");
            break;
        }
    }
}

static void print_boot_type_name(uint8_t boot_type) {
    switch (boot_type & ~BOOT_TYPE_CHAINED_FLAG) {
        case BOOT_TYPE_NORMAL: {
            printf("\tnormal\n");
            break;
        }
        case BOOT_TYPE_BOOTSEL: {
            printf("\tbootsel\n");
            break;
        }
        case BOOT_TYPE_RAM_IMAGE: {
            printf("\tram image\n");
            break;
        }
        case BOOT_TYPE_FLASH_UPDATE: {
            printf("\tflash update\n");
            break;
        }
        case BOOT_TYPE_PC_SP: {
            printf("\tpc_sp\n");
            break;
        }
        default: {
            printf("\tunknown\n");
            break;
        }
    }
    if (boot_type & BOOT_TYPE_CHAINED_FLAG) {
        printf("\tchained\n");
    }
}

static void print_try_before_you_buy_name(uint8_t tbyb) {
    assert((tbyb & BOOT_TBYB_AND_UPDATE_FLAG_OTP_VERSION_APPLIED) || !(tbyb & BOOT_TBYB_AND_UPDATE_FLAG_OTHER_ERASED));
    switch (tbyb & ~BOOT_TBYB_AND_UPDATE_FLAG_OTHER_ERASED) {
        case 0: {
            printf("\tnone\n"); 
            break;       
        }
        case BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING: {
            printf("\tpending\n");
            break;
        }
        case BOOT_TBYB_AND_UPDATE_FLAG_OTP_VERSION_APPLIED: {
            printf("\tapplied\n");
            if (tbyb & BOOT_TBYB_AND_UPDATE_FLAG_OTHER_ERASED) {
                printf("\tother erased\n");
            }
            break;
        }
        default: {
            printf("\tunknow\n");
            break;
        }
    }
}

static void print_boot_diagnostics(uint32_t boot_diagnostic) {
    if (boot_diagnostic & BOOT_DIAGNOSTIC_WINDOW_SEARCHED) {
        printf("\twindow searched\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_INVALID_BLOCK_LOOP) {
        printf("\tinvalid block loop\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_VALID_BLOCK_LOOP) {
        printf("\tvalid block loop\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_VALID_IMAGE_DEF) {
        printf("\tvalid image def\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_HAS_PARTITION_TABLE) {
        printf("\thas partition table\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_CONSIDERED) {
        printf("\tconsidered\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_CHOSEN) {
        printf("\tchosen\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_PARTITION_TABLE_MATCHING_KEY_FOR_VERIFY) {
        printf("\tpartition table matching key for verify\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_PARTITION_TABLE_HASH_FOR_VERIFY) {
        printf("\tpartition table hash for verify\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_PARTITION_TABLE_VERIFIED_OK) {
        printf("\tpartition table verified ok\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_IMAGE_DEF_MATCHING_KEY_FOR_VERIFY) {
        printf("\timage def matching key for verify\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_IMAGE_DEF_HASH_FOR_VERIFY) {
        printf("\timage def hash for verify\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_IMAGE_DEF_VERIFIED_OK) {
        printf("\timage def verified ok\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_LOAD_MAP_ENTRIES_LOADED) {
        printf("\tload map entries loaded\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_IMAGE_LAUNCHED) {
        printf("\timage launched\n");
    }
    if (boot_diagnostic & BOOT_DIAGNOSTIC_IMAGE_CONDITION_FAILURE) {
        printf("\timage condition failure\n");
    }
}

int main() {
    stdio_init_all();

    boot_info_t boot_info;
    hard_assert(rom_get_boot_info(&boot_info));

    // This information is also available via rom_get_last_boot_type or rom_get_last_boot_type_with_chained_flag
    printf("boot_type: %u\n", boot_info.boot_type); print_boot_type_name(boot_info.boot_type);
    printf("reboot_params: 0x%08x:0x%08x\n", boot_info.reboot_params[0], boot_info.reboot_params[1]);
    printf("diagnostic_partition_index: %d\n", boot_info.diagnostic_partition_index); print_partition_name(boot_info.diagnostic_partition_index);
    printf("recent_boot_partition: %d\n", boot_info.partition); print_partition_name(boot_info.partition);
    printf("recent_boot_tbyb_and_update_info: %u\n", boot_info.tbyb_and_update_info); print_try_before_you_buy_name(boot_info.tbyb_and_update_info);
    printf("boot_diagnostics 0x%08x:\n", boot_info.boot_diagnostic); print_boot_diagnostics(boot_info.boot_diagnostic);
    printf("watchdog reboot %d (0x%x)\n", watchdog_caused_reboot(), watchdog_hw->reason);
}