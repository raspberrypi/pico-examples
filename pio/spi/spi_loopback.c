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

static int spi_example_passes;
static int spi_example_runs;

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
    if (mismatch) {
        printf("\nNope\n");
    } else {
        printf("\nOK\n");
        spi_example_passes++;
    }
}

int main() {
    stdio_init_all();
    printf("spi_loopback start\n");
    pio_spi_inst_t spi[2];
#if 1
    float clkdiv = 1; // as fast as possible!
#else
    float clkdiv = 31.25f;  // 1 MHz @ 125 clk_sys
#endif

    // pio_claim_free_sm_and_add_program_for_gpio_range finds a free pio and state machine for a program and sets the gpio base correctly
    const uint pin_base = MIN(PIN_SCK, MIN(PIN_MOSI, PIN_MISO));
    const uint pin_count = MAX(PIN_SCK, MAX(PIN_MOSI, PIN_MISO)) - pin_base + 1;
    hard_assert(pio_claim_free_sm_and_add_program_for_gpio_range(&spi_cpha0_program, &spi[0].pio, &spi[0].sm, &spi[0].offset, pin_base, pin_count, true));
    hard_assert(pio_claim_free_sm_and_add_program_for_gpio_range(&spi_cpha1_program, &spi[1].pio, &spi[1].sm, &spi[1].offset, pin_base, pin_count, true));

    for (int cpha = 0; cpha <= 1; ++cpha) {
        for (int cpol = 0; cpol <= 1; ++cpol) {
            spi_example_runs++;
            printf("CPHA = %d, CPOL = %d\n", cpha, cpol);
            pio_spi_init(spi[cpha].pio, spi[cpha].sm,
                         spi[cpha].offset,
                         8,       // 8 bits per SPI frame
                         clkdiv,
                         cpha,
                         cpol,
                         PIN_SCK,
                         PIN_MOSI,
                         PIN_MISO
            );
            test(&spi[cpha]);
            sleep_ms(10);
        }
    }
    printf("Example %s\n", spi_example_passes == spi_example_runs ? "PASSED" : "FAILED");
    assert(spi_example_passes == spi_example_runs);
}
