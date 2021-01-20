/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/dma.h"
#include "hardware/structs/ssi.h"

// This example DMAs 16kB of data from the start of flash to SRAM, and
// measures the transfer speed.
//
// The SSI (flash interface) inside the XIP block has DREQ logic, so we can
// DMA directly from its FIFOs. Unlike the XIP stream hardware (see
// flash_xip_stream.c) this can *not* be done whilst code is running from
// flash, without careful footwork like we do here. The tradeoff is that it's
// ~2.5x as fast in QSPI mode, ~2x as fast in SPI mode.

void __no_inline_not_in_flash_func(flash_bulk_read)(uint32_t *rxbuf, uint32_t flash_offs, size_t len,
                                                 uint dma_chan) {
    // SSI must be disabled to set transfer size. If software is executing
    // from flash right now then it's about to have a bad time
    ssi_hw->ssienr = 0;
    ssi_hw->ctrlr1 = len - 1; // NDF, number of data frames
    ssi_hw->dmacr = SSI_DMACR_TDMAE_BITS | SSI_DMACR_RDMAE_BITS;
    ssi_hw->ssienr = 1;
    // Other than NDF, the SSI configuration used for XIP is suitable for a bulk read too.

    // Configure and start the DMA. Note we are avoiding the dma_*() functions
    // as we can't guarantee they'll be inlined
    dma_hw->ch[dma_chan].read_addr = (uint32_t) &ssi_hw->dr0;
    dma_hw->ch[dma_chan].write_addr = (uint32_t) rxbuf;
    dma_hw->ch[dma_chan].transfer_count = len;
    // Must enable DMA byteswap because non-XIP 32-bit flash transfers are
    // big-endian on SSI (we added a hardware tweak to make XIP sensible)
    dma_hw->ch[dma_chan].ctrl_trig =
            DMA_CH0_CTRL_TRIG_BSWAP_BITS |
            DREQ_XIP_SSIRX << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB |
            dma_chan << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB |
            DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS |
            DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_WORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB |
            DMA_CH0_CTRL_TRIG_EN_BITS;

    // Now DMA is waiting, kick off the SSI transfer (mode continuation bits in LSBs)
    ssi_hw->dr0 = (flash_offs << 8u) | 0xa0u;

    // Wait for DMA finish
    while (dma_hw->ch[dma_chan].ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_BITS);

    // Reconfigure SSI before we jump back into flash!
    ssi_hw->ssienr = 0;
    ssi_hw->ctrlr1 = 0; // Single 32-bit data frame per transfer
    ssi_hw->dmacr = 0;
    ssi_hw->ssienr = 1;
}

#define DATA_SIZE_WORDS 4096

uint32_t rxdata[DATA_SIZE_WORDS];
uint32_t *expect = (uint32_t *) XIP_NOCACHE_NOALLOC_BASE;

int main() {
    stdio_init_all();
    memset(rxdata, 0, DATA_SIZE_WORDS * sizeof(uint32_t));

    printf("Starting DMA\n");
    uint32_t start_time = time_us_32();
    flash_bulk_read(rxdata, 0, DATA_SIZE_WORDS, 0);
    uint32_t finish_time = time_us_32();
    printf("DMA finished\n");

    float elapsed_time_s = 1e-6f * (finish_time - start_time);
    printf("Transfer speed: %.3f MB/s\n", (sizeof(uint32_t) * DATA_SIZE_WORDS / 1e6f) / elapsed_time_s);

    bool mismatch = false;
    for (int i = 0; i < DATA_SIZE_WORDS; ++i) {
        if (rxdata[i] != expect[i]) {
            printf("Mismatch at %d: expected %08x, got %08x\n", i, expect[i], rxdata[i]);
            mismatch = true;
            break;
        }
    }
    if (!mismatch)
        printf("Data check ok\n");
}