/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Use the DMA engine's 'sniff' capability to calculate a CRC32 on data in a buffer.
// Note:  This does NOT do an actual data copy, it 'transfers' all the data to a single
// dummy destination byte so as to be able to crawl over the input data using a 'DMA'.
// If a data copy *with* a CRC32 sniff is required, the start address of the suitably sized
// destination buffer must be supplied and the 'write_increment' set to true (see below).

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"

#define CRC32_INIT                  ((uint32_t)-1l)

#define DATA_TO_CHECK_LEN           9
#define CRC32_LEN                   4
#define TOTAL_LEN                   (DATA_TO_CHECK_LEN + CRC32_LEN)

// commonly used crc test data and also space for the crc value
static uint8_t src[TOTAL_LEN] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x00, 0x00, 0x00, 0x00 };
static uint8_t dummy_dst[1];

// This uses a standard polynomial with the alternate 'reversed' shift direction.
// It is possible to use a non-reversed algorithm here but the DMA sniff set-up
// below would need to be modified to remain consistent and allow the check to pass.
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

    // calculate and append the crc
    crc_res = soft_crc32_block(CRC32_INIT, src, DATA_TO_CHECK_LEN);
    *((uint32_t *)&src[DATA_TO_CHECK_LEN]) = crc_res;

    printf("Buffer to DMA: ");
    for (int i = 0; i < TOTAL_LEN; i++) {
        printf("0x%02x ", src[i]);
    }
    printf("\n");

    // UNcomment the next line to deliberately corrupt the buffer
    //src[0]++;  // modify any byte, in any way, to break the CRC32 check

    // Get a free channel, panic() if there are none
    int chan = dma_claim_unused_channel(true);

    // 8 bit transfers. The read address increments after each transfer but
    // the write address remains unchanged pointing to the dummy destination.
    // No DREQ is selected, so the DMA transfers as fast as it can.
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);

    // (bit-reverse) CRC32 specific sniff set-up
    channel_config_set_sniff_enable(&c, true);
    dma_sniffer_set_data_accumulator(CRC32_INIT);
    dma_sniffer_set_output_reverse_enabled(true);
    dma_sniffer_enable(chan, DMA_SNIFF_CTRL_CALC_VALUE_CRC32R, true);

    dma_channel_configure(
        chan,          // Channel to be configured
        &c,            // The configuration we just created
        dummy_dst,     // The (unchanging) write address
        src,           // The initial read address
        TOTAL_LEN,     // Total number of transfers inc. appended crc; each is 1 byte
        true           // Start immediately.
    );

    // We could choose to go and do something else whilst the DMA is doing its
    // thing. In this case the processor has nothing else to do, so we just
    // wait for the DMA to finish.
    dma_channel_wait_for_finish_blocking(chan);

    uint32_t sniffed_crc = dma_sniffer_get_data_accumulator();
    printf("Completed DMA sniff of %d byte buffer, DMA sniff accumulator value: 0x%x\n", TOTAL_LEN, sniffed_crc);

    if (0ul == sniffed_crc) {
        printf("CRC32 check is good\n");
    }
    else {
        printf("ERROR - CRC32 check FAILED!\n");
    }
}
