/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "uart_tx.pio.h"

int main() {
    // We're going to use PIO to print "Hello, world!" on the same GPIO which we
    // normally attach UART0 to.
    const uint PIN_TX = 0;
    // This is the same as the default UART baud rate on Pico
    const uint SERIAL_BAUD = 115200;

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &uart_tx_program);
    uart_tx_program_init(pio, sm, offset, PIN_TX, SERIAL_BAUD);

    while (true) {
        uart_tx_program_puts(pio, sm, "Hello, world! (from PIO!)\n");
        sleep_ms(1000);
    }
}
