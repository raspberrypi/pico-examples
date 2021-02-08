/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "clocked_input.pio.h"

// Set up a PIO state machine to shift in serial data, sampling with an
// external clock, and push the data to the RX FIFO, 8 bits at a time.
//
// Use one of the hard SPI peripherals to drive data into the SM through a
// pair of external jumper wires, then read back and print out the data from
// the SM to confirm everything worked ok.
//
// On your Pico you need to connect jumper wires to these pins:
// - GPIO 2 -> GPIO 5 (clock output to clock input)
// - GPIO 3 -> GPIO 4 (data output to data input)

#define SPI_SCK_PIN 2
#define SPI_TX_PIN 3
// GPIO 4 for PIO data input, GPIO 5 for clock input:
#define PIO_INPUT_PIN_BASE 4

#define BUF_SIZE 8

int main() {
    stdio_init_all();

    // Configure the SPI before PIO to avoid driving any glitches into the
    // state machine.
    spi_init(spi0, 1000 * 1000);
    uint actual_freq_hz = spi_set_baudrate(spi0, clock_get_hz(clk_sys) / 6);
    printf("SPI running at %u Hz\n", actual_freq_hz);
    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);

    // Load the clocked_input program, and configure a free state machine
    // to run the program.
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &clocked_input_program);
    uint sm = pio_claim_unused_sm(pio, true);
    clocked_input_program_init(pio, sm, offset, PIO_INPUT_PIN_BASE);

    // Make up some random data to send.
    static uint8_t txbuf[BUF_SIZE];
    puts("Data to transmit:");
    for (int i = 0; i < BUF_SIZE; ++i) {
        txbuf[i] = rand() >> 16;
        printf("%02x\n", txbuf[i]);
    }

    // The "blocking" write function will send all the data in one go, and not
    // return until the full transmission is finished.
    spi_write_blocking(spi0, (const uint8_t*)txbuf, BUF_SIZE);

    // The data we just sent should now be present in the state machine's
    // FIFO. We only sent 8 bytes, so all the data received by the state
    // machine will fit into the FIFO. Generally you want to be continuously
    // reading data out as it appears in the FIFO -- either with polling, FIFO
    // interrupts, or DMA.
    puts("Reading back from RX FIFO:");
    for (int i = 0; i < BUF_SIZE; ++i) {
        uint8_t rxdata = pio_sm_get_blocking(pio, sm);
        printf("%02x %s\n", rxdata, rxdata == txbuf[i] ? "OK" : "FAIL");
    }
    puts("Done.");
}
