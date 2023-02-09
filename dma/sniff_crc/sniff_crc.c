/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Use the DMA engine's 'sniff' capability to calculate a CRC32 on data copied between two buffers in memory.

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"

#define CRC32_INIT                  ((uint32_t)-1l)

#define DATA_TO_CHECK_LEN           9
#define CRC32_LEN                   4
#define TOTAL_LEN                   (DATA_TO_CHECK_LEN + CRC32_LEN)

// commonly used crc test data and also space for the crc
static uint8_t src[TOTAL_LEN] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x00, 0x00, 0x00, 0x00 };
static uint8_t dst[TOTAL_LEN];

// This uses the "reversed" polynomial and shift direction
static uint32_t soft_crc32_block(uint32_t crc, uint8_t *bytp, uint32_t length) {
    while(length--) {
        uint32_t byte32 = (uint32_t)*bytp++;

        for (uint8_t bit = 8; bit; bit--, byte32 >>= 1) {
            crc = (crc >> 1) ^ (((crc ^ byte32) & 1ul) ? 0xEDB88320ul : 0ul);
        }
    }
    return crc;
}

int main() {
    uint32_t crc_res;

    stdio_init_all();

    printf("\n");
    printf("\n");

    // cacluate and append the crc
    crc_res = soft_crc32_block(CRC32_INIT, src, DATA_TO_CHECK_LEN);
    *((uint32_t *)&src[DATA_TO_CHECK_LEN]) = crc_res;

    printf("Buffer to DMA: ");
    for (int i = 0; i < 12; i++) {
        printf("0x%02x ", src[i]);
    }
    printf("\n");

    // Get a free channel, panic() if there are none
    int chan = dma_claim_unused_channel(true);

    // 8 bit transfers. Both read and write address increment after each
    // transfer (each pointing to a location in src or dst respectively).
    // No DREQ is selected, so the DMA transfers as fast as it can.

    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);

    // Sniff specific set-up
    channel_config_set_sniff_enable(&c, true);
    dma_sniffer_set_data_accumulator(CRC32_INIT);
    dma_sniffer_set_output_reverse_enabled(true);
    dma_sniffer_enable(chan, DMA_SNIFF_CTRL_CALC_VALUE_CRC32R, true);

    dma_channel_configure(
        chan,          // Channel to be configured
        &c,            // The configuration we just created
        dst,           // The initial write address
        src,           // The initial read address
        TOTAL_LEN,     // Total number of transfers inc. appended CRC; each is 1 byte
        true           // Start immediately.
    );

    // We could choose to go and do something else whilst the DMA is doing its
    // thing. In this case the processor has nothing else to do, so we just
    // wait for the DMA to finish.
    dma_channel_wait_for_finish_blocking(chan);

    uint32_t sniffed_crc = dma_sniffer_get_data_accumulator();
    printf("Completed DMA copy of %d byte buffer, sniffed crc: 0x%08lx\n", TOTAL_LEN, sniffed_crc);
}
