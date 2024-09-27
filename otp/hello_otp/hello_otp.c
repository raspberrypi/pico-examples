/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/structs/otp.h"

int main() {
    stdio_init_all();

    otp_cmd_t cmd;
    int8_t ret;

    // Row to write ECC data
    uint16_t ecc_row = 0x400;
    // Row to write raw data
    uint16_t raw_row = 0x410;

    // Check rows are empty - else the rest of the tests won't behave as expected
    unsigned char initial_data[32] = {0};
    cmd.flags = ecc_row;
    ret = rom_func_otp_access(initial_data, sizeof(initial_data)/2, cmd);
    if (ret) {
        printf("ERROR: Initial ECC Row Read failed with error %d\n", ret);
    }
    cmd.flags = raw_row;
    ret = rom_func_otp_access(initial_data+(sizeof(initial_data)/2), sizeof(initial_data)/2, cmd);
    if (ret) {
        printf("ERROR: Initial Raw Row Read failed with error %d\n", ret);
    }
    for (int i=0; i < sizeof(initial_data); i++) {
        if (initial_data[i] != 0) {
            printf("ERROR: This example requires empty OTP rows to run - change the ecc_row and raw_row variables to an empty row and recompile\n");
            return 0;
        }
    }

    if (ecc_row) {
        // Write an ECC value to OTP - the buffer must have a multiple of 2 length for ECC data
        unsigned char ecc_write_data[16] = "Hello from OTP";
        cmd.flags = ecc_row | OTP_CMD_ECC_BITS | OTP_CMD_WRITE_BITS;
        ret = rom_func_otp_access(ecc_write_data, sizeof(ecc_write_data), cmd);
        if (ret) {
            printf("ERROR: ECC Write failed with error %d\n", ret);
        } else {
            printf("ECC Write succeeded\n");
        }

        // Read it back
        unsigned char ecc_read_data[sizeof(ecc_write_data)] = {0};
        cmd.flags = ecc_row | OTP_CMD_ECC_BITS;
        ret = rom_func_otp_access(ecc_read_data, sizeof(ecc_read_data), cmd);
        if (ret) {
            printf("ERROR: ECC Read failed with error %d\n", ret);
        } else {
            printf("ECC Data read is \"%s\"\n", ecc_read_data);
        }

        // Set some bits, to demonstrate ECC error correction
        unsigned char ecc_toggle_buffer[sizeof(ecc_write_data)*2] = {0};
        cmd.flags = ecc_row;
        ret = rom_func_otp_access(ecc_toggle_buffer, sizeof(ecc_toggle_buffer), cmd);
        if (ret) {
            printf("ERROR: Raw read of ECC data failed with error %d\n", ret);
        } else {
            ecc_toggle_buffer[0] = 'x'; // will fail to recover, as flips 2 bits from 'H' (100_1000 -> 111_1000)
            ecc_toggle_buffer[24] = 't'; // will recover, as only flips 1 bit from 'T' (101_0100 -> 111_0100)
            cmd.flags = ecc_row | OTP_CMD_WRITE_BITS;
            ret = rom_func_otp_access(ecc_toggle_buffer, sizeof(ecc_toggle_buffer), cmd);
            if (ret) {
                printf("ERROR: Raw overwrite of ECC data failed with error %d\n", ret);
            } else {
                printf("Raw overwrite of ECC data succeeded\n");
            }
        }

        // Read it back
        unsigned char ecc_toggled_read_data[sizeof(ecc_write_data)] = {0};
        cmd.flags = ecc_row | OTP_CMD_ECC_BITS;
        ret = rom_func_otp_access(ecc_toggled_read_data, sizeof(ecc_toggled_read_data), cmd);
        if (ret) {
            printf("ERROR: ECC Read failed with error %d\n", ret);
        } else {
            printf("ECC Data read is now \"%s\"\n", ecc_toggled_read_data);
        }

        // Attempt to write a different ECC value to OTP - should fail
        unsigned char ecc_overwrite_data[sizeof(ecc_write_data)] = "hello from otp";
        cmd.flags = ecc_row | OTP_CMD_ECC_BITS | OTP_CMD_WRITE_BITS;
        ret = rom_func_otp_access(ecc_overwrite_data, sizeof(ecc_overwrite_data), cmd);
        if (ret == BOOTROM_ERROR_UNSUPPORTED_MODIFICATION) {
            printf("Overwrite of ECC data failed as expected\n");
        } else {
            printf("ERROR: ");
            if (ret) {
                printf("Overwrite failed with error %d\n", ret);
            } else {
                printf("Overwrite succeeded\n");
            }
        }
    }

    if (raw_row) {
        // Write a raw value to OTP - the buffer must have a multiple of 4 length for raw data
        // Each row only holds 24 bits, so every 4th byte isn't written to OTP
        unsigned char raw_write_data[20] = "Hel\0lo \0fro\0m O\0TP";
        cmd.flags = raw_row | OTP_CMD_WRITE_BITS;
        ret = rom_func_otp_access(raw_write_data, sizeof(raw_write_data), cmd);
        if (ret) {
            printf("ERROR: Raw Write failed with error %d\n", ret);
        } else {
            printf("Raw Write succeeded\n");
        }

        // Read it back
        unsigned char raw_read_data[sizeof(raw_write_data)] = {0};
        cmd.flags = raw_row;
        ret = rom_func_otp_access(raw_read_data, sizeof(raw_read_data), cmd);
        if (ret) {
            printf("ERROR: Raw Read failed with error %d\n", ret);
        } else {
            // Remove the null bytes
            for (int i=0; i < sizeof(raw_read_data)/4; i++) {
                memcpy(raw_read_data + i*3, raw_read_data + i*4, 3);
            }
            printf("Raw Data read is \"%s\"\n", raw_read_data);
        }

        // Attempt to write a different raw value to OTP - should succeed, provided no bits are cleared
        // This can be done by using '~' for even characters, and 'o' for odd ones
        unsigned char raw_overwrite_data[sizeof(raw_write_data)] = {0};
        for (int i=0; i < sizeof(raw_write_data); i++) {
            if (raw_write_data[i]) {
                raw_overwrite_data[i] = (raw_write_data[i] % 2) ? 'o' : '~';
            } else {
                raw_overwrite_data[i] = 0;
            }
        }
        cmd.flags = raw_row | OTP_CMD_WRITE_BITS;
        ret = rom_func_otp_access(raw_overwrite_data, sizeof(raw_overwrite_data), cmd);
        if (ret) {
            printf("ERROR: Raw Overwrite failed with error %d\n", ret);
        } else {
            printf("Raw Overwrite succeeded\n");
        }

        // Read it back
        unsigned char raw_read_data_again[sizeof(raw_write_data)] = {0};
        cmd.flags = raw_row;
        ret = rom_func_otp_access(raw_read_data_again, sizeof(raw_read_data_again), cmd);
        // Remove the null bytes
        for (int i=0; i < sizeof(raw_read_data_again)/4; i++) {
            memcpy(raw_read_data_again + i*3, raw_read_data_again + i*4, 3);
        }
        if (ret) {
            printf("ERROR: Raw Read failed with error %d\n", ret);
        } else {
            printf("Raw Data read is now \"%s\"\n", raw_read_data_again);
        }
    }

    int16_t lock_row = ecc_row ? ecc_row : raw_row;
    // Lock the OTP page, to prevent any more reads or writes until the next reset
    int page = lock_row / 0x40;
    otp_hw->sw_lock[page] = 0xf;
    printf("OTP Software Lock Done\n");

    // Attempt to read it back again - should fail
    unsigned char read_data_locked[8] = {0};
    cmd.flags = lock_row | (lock_row == ecc_row ? OTP_CMD_ECC_BITS : 0);
    ret = rom_func_otp_access(read_data_locked, sizeof(read_data_locked), cmd);
    if (ret == BOOTROM_ERROR_NOT_PERMITTED) {
        printf("Locked read failed as expected\n");
    } else {
        printf("ERROR: ");
        if (ret) {
            printf("Locked read failed with error %d\n", ret);
        } else {
            printf("Locked read succeeded. Data read is \"%s\"\n", read_data_locked);
        }
    }

    return 0;
}
