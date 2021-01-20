/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pio_spi.h"

// This program instantiates a PIO SPI with each of the four possible
// CPOL/CPHA combinations, with the serial input and output pin mapped to the
// same GPIO. Any data written into the state machine's TX FIFO should then be
// serialised, deserialised, and reappear in the state machine's RX FIFO.

#define PIN_SCK 18
#define PIN_MOSI 16
#define PIN_MISO 16 // same as MOSI, so we get loopback

#define BUF_SIZE 20

void test(const pio_spi_inst_t *spi) {
    static uint8_t txbuf[BUF_SIZE];
    static uint8_t rxbuf[BUF_SIZE];
    printf("TX:");
    for (int i = 0; i < BUF_SIZE; ++i) {
        txbuf[i] = rand() >> 16;
        rxbuf[i] = 0;
        printf(" %02x", (int) txbuf[i]);
    }
    printf("\n");

    pio_spi_write8_read8_blocking(spi, txbuf, rxbuf, BUF_SIZE);

    printf("RX:");
    bool mismatch = false;
    for (int i = 0; i < BUF_SIZE; ++i) {
        printf(" %02x", (int) rxbuf[i]);
        mismatch = mismatch || rxbuf[i] != txbuf[i];
    }
    if (mismatch)
        printf("\nNope\n");
    else
        printf("\nOK\n");
}

int main() {
    stdio_init_all();

    pio_spi_inst_t spi = {
            .pio = pio0,
            .sm = 0
    };
    float clkdiv = 31.25f;  // 1 MHz @ 125 clk_sys
    uint cpha0_prog_offs = pio_add_program(spi.pio, &spi_cpha0_program);
    uint cpha1_prog_offs = pio_add_program(spi.pio, &spi_cpha1_program);

    for (int cpha = 0; cpha <= 1; ++cpha) {
        for (int cpol = 0; cpol <= 1; ++cpol) {
            printf("CPHA = %d, CPOL = %d\n", cpha, cpol);
            pio_spi_init(spi.pio, spi.sm,
                         cpha ? cpha1_prog_offs : cpha0_prog_offs,
                         8,       // 8 bits per SPI frame
                         clkdiv,
                         cpha,
                         cpol,
                         PIN_SCK,
                         PIN_MOSI,
                         PIN_MISO
            );
            test(&spi);
            sleep_ms(10);
        }
    }
}
