/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Show how to reconfigure and restart a channel in a channel completion
// interrupt handler.
//
// Our DMA channel will transfer data to a PIO state machine, which is
// configured to serialise the raw bits that we push, one by one. We're going
// to use this to do some crude LED PWM by repeatedly sending values with the
// right balance of 1s and 0s. (note there are better ways to do PWM with PIO
// -- see the PIO PWM example).
//
// Once the channel has sent a predetermined amount of data, it will halt, and
// raise an interrupt flag. The processor will enter the interrupt handler in
// response to this, where it will reconfigure and restart the channel. This
// repeats.

#include <stdio.h>
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pio_serialiser.pio.h"

// PIO sends one bit per 10 system clock cycles. DMA sends the same 32-bit
// value 10 000 times before halting. This means we cycle through the 32 PWM
// levels roughly once per second.
#define PIO_SERIAL_CLKDIV 10.f
#define PWM_REPEAT_COUNT 10000
#define N_PWM_LEVELS 32

int dma_chan;

void dma_handler() {
    static int pwm_level = 0;
    static uint32_t wavetable[N_PWM_LEVELS];
    static bool first_run = true;
    // Entry number `i` has `i` one bits and `(32 - i)` zero bits.
    if (first_run) {
        first_run = false;
        for (int i = 0; i < N_PWM_LEVELS; ++i)
            wavetable[i] = ~(~0u << i);
    }

    // Clear the interrupt request.
    dma_hw->ints0 = 1u << dma_chan;
    // Give the channel a new wave table entry to read from, and re-trigger it
    dma_channel_set_read_addr(dma_chan, &wavetable[pwm_level], true);

    pwm_level = (pwm_level + 1) % N_PWM_LEVELS;
}

int main() {
#ifndef PICO_DEFAULT_LED_PIN
#warning dma/channel_irq example requires a board with a regular LED
#else
    // Set up a PIO state machine to serialise our bits
    uint offset = pio_add_program(pio0, &pio_serialiser_program);
    pio_serialiser_program_init(pio0, 0, offset, PICO_DEFAULT_LED_PIN, PIO_SERIAL_CLKDIV);

    // Configure a channel to write the same word (32 bits) repeatedly to PIO0
    // SM0's TX FIFO, paced by the data request signal from that peripheral.
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
        dma_chan,
        &c,
        &pio0_hw->txf[0], // Write address (only need to set this once)
        NULL,             // Don't provide a read address yet
        PWM_REPEAT_COUNT, // Write the same value many times, then halt and interrupt
        false             // Don't start yet
    );

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dma_chan, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Manually call the handler once, to trigger the first transfer
    dma_handler();

    // Everything else from this point is interrupt-driven. The processor has
    // time to sit and think about its early retirement -- maybe open a bakery?
    while (true)
        tight_loop_contents();
#endif
}
