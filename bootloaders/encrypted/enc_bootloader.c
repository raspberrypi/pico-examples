/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "boot/picobin.h"
#include "hardware/uart.h"
#include "pico/bootrom.h"
#include "pico/rand.h"
#include "hardware/structs/otp.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"

#include "config.h"

#define OTP_KEY_PAGE 30

extern void decrypt(uint8_t* key4way, uint8_t* IV_OTPsalt, uint8_t* IV_public, uint8_t(*buf)[16], int nblk);

// The function lock_key() is called from decrypt() after key initialisation is complete and before decryption begins.
// That is a suitable point to lock the OTP area where key information is stored.
void lock_key() {
    otp_hw->sw_lock[OTP_KEY_PAGE] = 0xf;
}


static __attribute__((aligned(4))) uint8_t workarea[4 * 1024];

int main() {
    stdio_init_all();

    printf("Entered bootloader code\n");
    int rc;
    rc = rom_load_partition_table(workarea, sizeof(workarea), false);
    if (rc) {
        printf("Partition Table Load failed %d - resetting\n", rc);
        reset_usb_boot(0, 0);
    }

    boot_info_t info;
    printf("Getting boot info\n");
    rc = rom_get_boot_info(&info);
    printf("Boot Type %x\n", info.boot_type);

    if (info.boot_type == BOOT_TYPE_FLASH_UPDATE) {
        printf("Flash Update Base %x\n", info.reboot_params[0]);
    }

    rc = rom_pick_ab_update_partition((uint32_t*)workarea, sizeof(workarea), 0);
    if (rc < 0) {
        printf("Partition Table A/B choice failed %d - resetting\n", rc);
        reset_usb_boot(0, 0);
    }
    uint8_t boot_partition = rc;
    printf("Picked A/B Boot partition %x\n", boot_partition);

    rc = rom_get_partition_table_info((uint32_t*)workarea, 0x8, PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION | (boot_partition << 24));

    uint32_t data_start_addr = 0;
    uint32_t data_end_addr = 0;
    uint32_t data_max_size = 0;
    if (rc != 3) {
        printf("No boot partition - assuming bin at start of flash\n");
        data_start_addr = 0;
        data_end_addr = 0x70000; // must fit into 0x20000000 -> 0x20070000
        data_max_size = data_end_addr - data_start_addr;
    } else {
        uint16_t first_sector_number = (((uint32_t*)workarea)[1] & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB;
        uint16_t last_sector_number = (((uint32_t*)workarea)[1] & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB;
        data_start_addr = first_sector_number * 0x1000;
        data_end_addr = (last_sector_number + 1) * 0x1000;
        data_max_size = data_end_addr - data_start_addr;

        printf("Partition Start %x, End %x, Max Size %x\n", data_start_addr, data_end_addr, data_max_size);
    }

    printf("Decrypting the chosen image\n");
    uint32_t first_mb_start = 0;
    bool first_mb_start_found = false;
    uint32_t first_mb_end = 0;
    uint32_t last_mb_start = 0;
    for (uint16_t i=0; i < 0x1000; i += 4) {
        if (*(uint32_t*)(XIP_BASE + data_start_addr + i) == 0xffffded3) {
            printf("Found first block start\n");
            first_mb_start = i;
            first_mb_start_found = true;
        } else if (first_mb_start_found && (*(uint32_t*)(XIP_BASE + data_start_addr + i) == 0xab123579)) {
            printf("Found first block end\n");
            first_mb_end = i + 4;
            last_mb_start = *(uint32_t*)(XIP_BASE + data_start_addr + i-4) + first_mb_start;
            break;
        }
    }

    if (last_mb_start > data_max_size) {
        // todo - harden this check
        printf("ERROR: Encrypted binary is too big for it's partition - resetting\n");
        reset_usb_boot(0, 0);
    }

    if (*(uint32_t*)(XIP_BASE + data_start_addr + last_mb_start) == 0xffffded3) {
        printf("Found last block start where expected\n");
    } else {
        printf("Did not find last block where expected\n");
        last_mb_start = 0;
    }

    if (first_mb_end == 0 || last_mb_start == 0) {
        printf("Couldn't find encrypted image %x %x - resetting\n", first_mb_end, last_mb_start);
        reset_usb_boot(0, 0);
    }

    printf("Encrypted from %x to %x\n", first_mb_end, last_mb_start);

    if (data_start_addr + last_mb_start > data_end_addr) {
        // todo - harden this check
        printf("ERROR: Encrypted binary is too big for it's partition - resetting\n");
        reset_usb_boot(0, 0);
    }

    printf("OTP Valid Keys %x\n", otp_hw->key_valid);

    printf("Unlocking\n");
    for (int i=0; i<4; i++) {
        uint32_t key_i = ((i*2+1) << 24) | ((i*2+1) << 16) |
                         (i*2 << 8) | i*2;
        otp_hw->crt_key_w[i] = key_i;
    }

    uint8_t iv[16];
    data_start_addr += first_mb_end;
    memcpy(iv, (void*)(XIP_BASE + data_start_addr), sizeof(iv));
    data_start_addr += 16;

    uint32_t data_size = last_mb_start - (first_mb_end + 16);
    printf("Data start %x, size %x\n", data_start_addr, data_size);
    if (SRAM_BASE + data_size > 0x20070000) {
        // todo - harden this check
        printf("ERROR: Encrypted binary is too big, and will overwrite this bootloader - resetting\n");
        reset_usb_boot(0, 0);
    }
    memcpy((void*)SRAM_BASE, (void*)(XIP_BASE + data_start_addr), data_size);

    printf("Pre decryption image begins with\n");
    for (int i=0; i < 4; i++)
        printf("%08x\n", *(uint32_t*)(SRAM_BASE + i*4));

    // Read key directly from OTP - guarded reads will throw a bus fault if there are any errors
    uint16_t* otp_data = (uint16_t*)OTP_DATA_GUARDED_BASE;

    decrypt(
        (uint8_t*)&(otp_data[(OTP_CMD_ROW_BITS &  (OTP_KEY_PAGE * 0x40))]),
        (uint8_t*)&(otp_data[(OTP_CMD_ROW_BITS &  ((OTP_KEY_PAGE + 1) * 0x40))]),
        iv, (void*)SRAM_BASE, data_size/16
    );

    // Lock the IV salt
    otp_hw->sw_lock[OTP_KEY_PAGE + 1] = 0xf;

    printf("Post decryption image begins with\n");
    for (int i=0; i < 4; i++)
        printf("%08x\n", *(uint32_t*)(SRAM_BASE + i*4));

    printf("Chaining into %x, size %x\n", SRAM_BASE, data_size);

    stdio_deinit_all();

    rc = rom_chain_image(
        workarea,
        sizeof(workarea),
        SRAM_BASE,
        data_size
    );

    stdio_init_all();
    printf("Shouldn't return from ROM call %d\n", rc);

    reset_usb_boot(0, 0);
}
