/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"

int main() {
    // create feature groups to group configuration settings
    // these will also show up in picotool info, not just picotool config
    bi_decl(bi_program_feature_group(0x1111, 0, "UART Configuration"));
    bi_decl(bi_program_feature_group(0x1111, 1, "Enabled Interfaces"));
    // stdio_uart configuration and initialisation
    bi_decl(bi_ptr_int32(0x1111, 1, use_uart, 1));
    bi_decl(bi_ptr_int32(0x1111, 0, uart_num, 0));
    bi_decl(bi_ptr_int32(0x1111, 0, uart_tx, 0));
    bi_decl(bi_ptr_int32(0x1111, 0, uart_rx, 1));
    bi_decl(bi_ptr_int32(0x1111, 0, uart_baud, 115200));
    if (use_uart) {
        stdio_uart_init_full(UART_INSTANCE(uart_num), uart_baud, uart_tx, uart_rx);
    }

    // stdio_usb initialisation
    bi_decl(bi_ptr_int32(0x1111, 1, use_usb, 1));
    if (use_usb) {
        stdio_usb_init();
    }

    // default printed string
    bi_decl(bi_ptr_string(0, 0, text, "Hello, world!", 256));

    while (true) {
        printf("%s\n", text);
        sleep_ms(1000);
    }
}
