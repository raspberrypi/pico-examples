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
#include "hardware/clocks.h"
#include "hardware/xosc.h"
#include "hardware/structs/rosc.h"
#include "hardware/pll.h"

#define OTP_KEY_PAGE 29

extern void decrypt(uint8_t* key4way, uint8_t* IV_OTPsalt, uint8_t* IV_public, uint8_t(*buf)[16], int nblk);

// These just have to be higher than the actual frequency, to prevent overclocking unused peripherals
#define ROSC_HZ 300*MHZ
#define OTHER_CLK_DIV 30


void runtime_init_clocks(void) {
    // Disable resus that may be enabled from previous software
    clocks_hw->resus.ctrl = 0;

    uint32_t rosc_div = 2; // default divider 2
    uint32_t rosc_drive = 0x7777; // default drives of 0b111 (0x7)

    // Bump up ROSC speed to ~110MHz
    rosc_hw->freqa = 0; // reset the drive strengths
    rosc_hw->div = rosc_div | ROSC_DIV_VALUE_PASS; // set divider
    // Increment the freqency range one step at a time - this is safe provided the current config is not TOOHIGH
    // because ROSC_CTRL_FREQ_RANGE_VALUE_MEDIUM | ROSC_CTRL_FREQ_RANGE_VALUE_HIGH == ROSC_CTRL_FREQ_RANGE_VALUE_HIGH
    static_assert((ROSC_CTRL_FREQ_RANGE_VALUE_LOW | ROSC_CTRL_FREQ_RANGE_VALUE_MEDIUM) == ROSC_CTRL_FREQ_RANGE_VALUE_MEDIUM);
    static_assert((ROSC_CTRL_FREQ_RANGE_VALUE_MEDIUM | ROSC_CTRL_FREQ_RANGE_VALUE_HIGH) == ROSC_CTRL_FREQ_RANGE_VALUE_HIGH);
    hw_set_bits(&rosc_hw->ctrl, ROSC_CTRL_FREQ_RANGE_VALUE_MEDIUM);
    hw_set_bits(&rosc_hw->ctrl, ROSC_CTRL_FREQ_RANGE_VALUE_HIGH);

    // Enable rosc randomisation
    rosc_hw->freqa = (ROSC_FREQA_PASSWD_VALUE_PASS << ROSC_FREQA_PASSWD_LSB) |
            rosc_drive | ROSC_FREQA_DS1_RANDOM_BITS | ROSC_FREQA_DS0_RANDOM_BITS; // enable randomisation

    // Not used with FREQ_RANGE_VALUE_HIGH, but should still be set to the maximum drive
    rosc_hw->freqb = (ROSC_FREQB_PASSWD_VALUE_PASS << ROSC_FREQB_PASSWD_LSB) |
            ROSC_FREQB_DS7_LSB | ROSC_FREQB_DS6_LSB | ROSC_FREQB_DS5_LSB | ROSC_FREQB_DS4_LSB;

    // CLK SYS = ROSC directly, as it's running slowly enough
    clock_configure_int_divider(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_ROSC_CLKSRC,
                    ROSC_HZ,    // this doesn't have to be accurate
                    1);

    // CLK_REF = ROSC / OTHER_CLK_DIV - this isn't really used, so just needs to be set to a low enough frequency
    clock_configure_int_divider(clk_ref,
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH,
                    0,
                    ROSC_HZ,
                    OTHER_CLK_DIV);


    // Everything else should run from PLL USB, so we can use UART and USB for output
    xosc_init();
    pll_init(pll_usb, PLL_USB_REFDIV, PLL_USB_VCO_FREQ_HZ, PLL_USB_POSTDIV1, PLL_USB_POSTDIV2);

    // CLK USB = PLL USB 48MHz / 1 = 48MHz
    clock_configure_undivided(clk_usb,
                    0, // No GLMUX
                    CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    USB_CLK_HZ);

    // CLK ADC = PLL USB 48MHz / 1 = 48MHz
    clock_configure_undivided(clk_adc,
                    0, // No GLMUX
                    CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    USB_CLK_HZ);

    // CLK PERI = PLL USB 48MHz / 1 = 48MHz. Used as reference clock for UART and SPI serial.
    clock_configure_undivided(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    USB_CLK_HZ);

    // CLK_HSTX = PLL USB 48MHz / 1 = 48MHz. Transmit bit clock for the HSTX peripheral.
    clock_configure_undivided(clk_hstx,
                    0,
                    CLOCKS_CLK_HSTX_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    USB_CLK_HZ);
}

// The function lock_key() is called from decrypt() after key initialisation is complete and before decryption begins.
// That is a suitable point to lock the OTP area where key information is stored.
void lock_key() {
    otp_hw->sw_lock[OTP_KEY_PAGE] = 0xf;
    otp_hw->sw_lock[OTP_KEY_PAGE + 1] = 0xf;
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

    rc = rom_pick_ab_partition_during_update((uint32_t*)workarea, sizeof(workarea), 0);
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
        (uint8_t*)&(otp_data[OTP_KEY_PAGE * 0x40]),
        (uint8_t*)&(otp_data[(OTP_KEY_PAGE + 2) * 0x40]),
        iv, (void*)SRAM_BASE, data_size/16
    );

    // Lock the IV salt
    otp_hw->sw_lock[OTP_KEY_PAGE + 2] = 0xf;

    printf("Post decryption image begins with\n");
    for (int i=0; i < 4; i++)
        printf("%08x\n", *(uint32_t*)(SRAM_BASE + i*4));

    printf("Chaining into %x, size %x\n", SRAM_BASE, data_size);

    stdio_uart_deinit();    // stdio_usb_deinit doesn't work here, so only deinit UART

    rc = rom_chain_image(
        workarea,
        sizeof(workarea),
        SRAM_BASE,
        data_size
    );

    stdio_uart_init();
    printf("Shouldn't return from ROM call %d\n", rc);

    reset_usb_boot(0, 0);
}
