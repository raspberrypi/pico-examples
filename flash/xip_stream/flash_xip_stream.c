/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/regs/addressmap.h"
#include "hardware/structs/xip_ctrl.h"

#include "random_test_data.h"

// The XIP has some internal hardware that can stream a linear access sequence
// to a DMAable FIFO, while the system is still doing random accesses on flash
// code + data.

uint32_t buf[count_of(random_test_data)];

int main() {
    stdio_init_all();
    for (int i = 0; i < count_of(random_test_data); ++i)
        buf[i] = 0;

    // This example won't work with PICO_NO_FLASH builds. Note that XIP stream
    // can be made to work in these cases, if you enable some XIP mode first
    // (e.g. via calling flash_enter_cmd_xip() in ROM). However, you will get
    // much better performance by DMAing directly from the SSI's FIFOs, as in
    // this way you can clock data continuously on the QSPI bus, rather than a
    // series of short transfers.
    if ((uint32_t) &random_test_data[0] >= SRAM_BASE) {
        printf("You need to run this example from flash!\n");
        exit(-1);
    }

    // Transfer started by writing nonzero value to stream_ctr. stream_ctr
    // will count down as the transfer progresses. Can terminate early by
    // writing 0 to stream_ctr.
    // It's a good idea to drain the FIFO first!
    printf("Starting stream from %p\n", random_test_data);
    /// \tag::start_stream[]
    while (!(xip_ctrl_hw->stat & XIP_STAT_FIFO_EMPTY))
        (void) xip_ctrl_hw->stream_fifo;
    xip_ctrl_hw->stream_addr = (uint32_t) &random_test_data[0];
    xip_ctrl_hw->stream_ctr = count_of(random_test_data);
    /// \end::start_stream[]

    // Start DMA transfer from XIP stream FIFO to our buffer in memory. Use
    // the auxiliary bus slave for the DMA<-FIFO accesses, to avoid stalling
    // the DMA against general XIP traffic. Doesn't really matter for this
    // example, but it can have a huge effect on DMA throughput.

    printf("Starting DMA\n");
    /// \tag::start_dma[]
    const uint dma_chan = 0;
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_XIP_STREAM);
    dma_channel_configure(
            dma_chan,
            &cfg,
            (void *) buf,                 // Write addr
            (const void *) XIP_AUX_BASE,  // Read addr
            count_of(random_test_data), // Transfer count
            true                        // Start immediately!
    );
    /// \end::start_dma[]

    dma_channel_wait_for_finish_blocking(dma_chan);

    printf("DMA complete\n");

    bool mismatch = false;
    for (int i = 0; i < count_of(random_test_data); ++i) {
        if (random_test_data[i] != buf[i]) {
            printf("Data mismatch: %08x (actual) != %08x (expected)\n", buf[i], random_test_data[i]);
            mismatch = true;
            break;
        }
        printf("%08x%c", buf[i], i % 8 == 7 ? '\n' : ' ');
    }
    if (!mismatch)
        printf("Data check OK\n");
}
