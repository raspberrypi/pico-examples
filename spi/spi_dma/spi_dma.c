/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Example of writing via DMA to the SPI interface and similarly reading it back via a loopback.

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define SPI_INST spi0

#define TEST_SIZE 1024

int main() {
    // Enable UART so we can print status output
    stdio_init_all();

    printf("SPI DMA example\n");

    // Enable SPI at 1 MHz and connect to GPIOs
    spi_init(SPI_INST, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Grab some unused dma channels
    const uint dma_tx = dma_claim_unused_channel(true);
    const uint dma_rx = dma_claim_unused_channel(true);

    // Force loopback for testing (I don't have an SPI device handy)
    hw_set_bits(&spi_get_hw(SPI_INST)->cr1, SPI_SSPCR1_LBM_BITS);

    static uint8_t txbuf[TEST_SIZE];
    static uint8_t rxbuf[TEST_SIZE];
    for (uint i = 0; i < TEST_SIZE; ++i) {
        txbuf[i] = rand();
    }

    // We set the outbound DMA to transfer from a memory buffer to the SPI transmit FIFO paced by the SPI TX FIFO DREQ
    // The default is for the read address to increment every element (in this case 1 byte - DMA_SIZE_8)
    // and for the write address to remain unchanged.

    printf("Configure TX DMA\n");
    dma_channel_config c = dma_channel_get_default_config(dma_tx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_index(SPI_INST) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
    dma_channel_configure(dma_tx, &c,
                          &spi_get_hw(SPI_INST)->dr, // write address
                          txbuf, // read address
                          TEST_SIZE, // element count (each element is of size transfer_data_size)
                          false); // don't start yet

    printf("Configure RX DMA\n");

    // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
    // We coinfigure the read address to remain unchanged for each element, but the write
    // address to increment (so data is written throughout the buffer)
    c = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_index(SPI_INST) ? DREQ_SPI1_RX : DREQ_SPI0_RX);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    dma_channel_configure(dma_rx, &c,
                          rxbuf, // write address
                          &spi_get_hw(SPI_INST)->dr, // read address
                          TEST_SIZE, // element count (each element is of size transfer_data_size)
                          false); // don't start yet


    printf("Starting DMAs...\n");
    // start them exactly simultaneously to avoid races (in extreme cases the FIFO could overflow)
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
    printf("Wait for RX complete...\n");
    dma_channel_wait_for_finish_blocking(dma_rx);
    if (dma_channel_is_busy(dma_tx)) {
        panic("RX completed before TX");
    }

    printf("Done. Checking...");
    for (uint i = 0; i < TEST_SIZE; ++i) {
        if (rxbuf[i] != txbuf[i]) {
            panic("Mismatch at %d/%d: expected %02x, got %02x",
                  i, TEST_SIZE, txbuf[i], rxbuf[i]
            );
        }
    }

    printf("All good\n");
    dma_channel_unclaim(dma_tx);
    dma_channel_unclaim(dma_rx);
    return 0;
}
