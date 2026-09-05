// Adapted from https://github.com/raspberrypi/pico-examples/blob/master/pio/ws2812/ws2812_parallel.c
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PLED_H
#define PLED_H

#include <stdlib.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <cmath>
#include <cstring>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/sem.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"

#include "CRGB.hpp"

#define LEDBITS 24
// bit plane content dma channel
#define DMA_CHANNEL 0
// chain channel for configuring main dma channel to output from disjoint 8 word fragments of memory
#define DMA_CB_CHANNEL 1

#define DMA_CHANNEL_MASK (1u << DMA_CHANNEL)
#define DMA_CB_CHANNEL_MASK (1u << DMA_CB_CHANNEL)
#define DMA_CHANNELS_MASK (DMA_CHANNEL_MASK | DMA_CB_CHANNEL_MASK)
#define PIO_FREQ 800000

#ifndef NUM_STRIPS
#define NUM_STRIPS 1
#endif

#ifndef LEDS_PER_STRIP
#define LEDS_PER_STRIP 1
#endif

#ifndef PIN_BASE
#define PIN_BASE 2
#endif

#ifndef INITIAL_BRIGHT
#define INITIAL_BRIGHT 64
#endif

namespace PLED {
uint8_t brightness = INITIAL_BRIGHT;
uint32_t planes[LEDS_PER_STRIP][LEDBITS];
CRGB strip[NUM_STRIPS][LEDS_PER_STRIP];
CRGB *led = &strip[0][0];
// start of each value fragment (+1 for NULL terminator)
uintptr_t fragment_start[LEDS_PER_STRIP + 1];
// posted when it is safe to output a new set of values
struct semaphore reset_delay_complete_sem;
// alarm handle for handling delay
alarm_id_t reset_delay_alarm_id;
PIO pio = pio0;
int sm = 0;
uint offset;

void transpose_bits() {
    memset(&planes, 0, sizeof(planes));
    for (uint l = 0; l < LEDS_PER_STRIP; l++) {
        uint32_t s;
        uint32_t sbit;
        for (s = 0, sbit = 1; s < NUM_STRIPS; s++, sbit <<= 1) {
            for (uint32_t b = 0, grb = (strip[s][l] * brightness).toGRB(); b < LEDBITS && grb; b++, grb >>= 1) {
                if (grb & 1) {
                    planes[l][LEDBITS - 1 - b] |= sbit;
                }
            }
        }
    }
}

int64_t reset_delay_complete(__unused alarm_id_t id, __unused void *user_data) {
    reset_delay_alarm_id = 0;
    sem_release(&reset_delay_complete_sem);
    // no repeat
    return 0;
}

void __isr dma_complete_handler() {
    if (dma_hw->ints0 & DMA_CHANNEL_MASK) {
        // clear IRQ
        dma_hw->ints0 = DMA_CHANNEL_MASK;
        // when the dma is complete we start the reset delay timer
        if (reset_delay_alarm_id)
            cancel_alarm(reset_delay_alarm_id);
        reset_delay_alarm_id = add_alarm_in_us(400, reset_delay_complete, NULL, true);
    }
}

void dma_init(PIO pio, uint sm) {
    dma_claim_mask(DMA_CHANNELS_MASK);

    // main DMA channel outputs 24 word fragments, and then chains back to the chain channel
    dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_dreq(&channel_config, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&channel_config, DMA_CB_CHANNEL);
    channel_config_set_irq_quiet(&channel_config, true);
    dma_channel_configure(DMA_CHANNEL,
                          &channel_config,
                          &pio->txf[sm],
                          NULL,  // set by chain
                          24,    // 24 words for 24 bit planes
                          false);

    // chain channel sends single word pointer to start of fragment each time
    dma_channel_config chain_config = dma_channel_get_default_config(DMA_CB_CHANNEL);
    dma_channel_configure(DMA_CB_CHANNEL,
                          &chain_config,
                          &dma_channel_hw_addr(
                               DMA_CHANNEL)
                               ->al3_read_addr_trig,  // ch DMA config (target "ring" buffer size 4) - this is (read_addr trigger)
                          NULL,                       // set later
                          1,
                          false);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    dma_channel_set_irq0_enabled(DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);
}

void output_strips_dma() {
    for (uint l = 0; l < LEDS_PER_STRIP; l++) {
        fragment_start[l] = (uintptr_t)planes[l];  // MSB first
    }
    fragment_start[LEDS_PER_STRIP] = 0;
    dma_channel_hw_addr(DMA_CB_CHANNEL)->al3_read_addr_trig = (uintptr_t)fragment_start;
}

void init() {
    offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, PIN_BASE, NUM_STRIPS, PIO_FREQ);
    sem_init(&reset_delay_complete_sem, 1, 1);  // initially posted so we don't block first time
    dma_init(pio, sm);
}

void show() {
    sem_acquire_blocking(&reset_delay_complete_sem);
    transpose_bits();
    output_strips_dma();
}
}  // namespace PLED
#endif
