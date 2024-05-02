/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pio_tdm.h"

// Just 8 bit functions provided here. The PIO program supports any frame size
// 1...32, but the software to do the necessary FIFO shuffling is left as an
// exercise for the reader :)
//
// Likewise we only provide MSB-first here. To do LSB-first, you need to
// - Do shifts when reading from the FIFO, for general case n != 8, 16, 32
// - Do a narrow read at a one halfword or 3 byte offset for n == 16, 8
// in order to get the read data correctly justified. 

void __time_critical_func(pio_tdm_write16_blocking)(const pio_tdm_inst_t *tdm, const uint16_t *src, size_t len) {
    size_t tx_remain = len, rx_remain = len;
    // Do 8 bit accesses on FIFO, so that write data is byte-replicated. This
    // gets us the left-justification for free (for MSB-first shift-out)
    io_rw_16 *txfifo = (io_rw_16 *) &tdm->pio->txf[tdm->sm + 1];
    io_rw_16 *rxfifo = (io_rw_16 *) &tdm->pio->rxf[tdm->sm];
//    while (tx_remain || rx_remain) {
    while (tx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(tdm->pio, tdm->sm + 1)) {
            *txfifo = *src++;
            --tx_remain;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(tdm->pio, tdm->sm)) {
            (void) *rxfifo;
            --rx_remain;
        }
    }
}

void __time_critical_func(pio_tdm_read16_blocking)(const pio_tdm_inst_t *tdm, uint16_t *dst, size_t len) {
    size_t tx_remain = len, rx_remain = len;
    io_rw_16 *txfifo = (io_rw_16 *) &tdm->pio->txf[tdm->sm + 1];
    io_rw_16 *rxfifo = (io_rw_16 *) &tdm->pio->rxf[tdm->sm];
    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(tdm->pio, tdm->sm + 1)) {
            *txfifo = 0;
            --tx_remain;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(tdm->pio, tdm->sm)) {
            *dst++ = *rxfifo;
            --rx_remain;
        }
    }
}

void __time_critical_func(pio_tdm_write16_read16_blocking)(const pio_tdm_inst_t *tdm, uint16_t *src, uint16_t *dst,
                                                         size_t len) {
    size_t tx_remain = len, rx_remain = len;
    io_rw_16 *txfifo = (io_rw_16 *) &tdm->pio->txf[tdm->sm + 1];
    io_rw_16 *rxfifo = (io_rw_16 *) &tdm->pio->rxf[(tdm->sm)];
    while (tx_remain || rx_remain) {
        if (tx_remain && !pio_sm_is_tx_fifo_full(tdm->pio, tdm->sm + 1)) {
            *txfifo = *src++;
            --tx_remain;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(tdm->pio, (tdm->sm))) {
            *dst = *rxfifo;
            if (*dst != 0xffff)
                {
                dst++;
                --rx_remain;
            }
        }
    }
}

