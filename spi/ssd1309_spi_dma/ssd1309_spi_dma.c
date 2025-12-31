/**
 * Copyright (c) 2025 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
**/

// An example showing how to drive a ssd1309-based OLED panel over an SPI
// interface from a frame buffer that is transferred in the background under
// DMA control. It should also work on a ssd1306 device (not tested).
//
// To understand the commands and addressing modes for the display refer to
// the manufacturer's datasheet at https://www.hpinfotech.ro/SSD1309.pdf

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/spi.h"
#include "string.h"

// dimensions of our OLED display panel
#define NUM_X_PIXELS        128
#define NUM_Y_PIXELS        64

// how often we want to refresh the display from the frame buffer
#define FRAME_REFRESH_HZ    50

// ssd1309 accepts a maximum SPI clock rate of 10 Mbit/sec and for this
// example we will use it at full speed
#define SPI_BITRATE         10 * 1000 * 1000

// define the pins for the SPI interface
// we will use the spi0 peripheral and the following GPIO pins (see the
// GPIO function select table in the Pico datasheet).
#define SPI_DEVICE          spi0
#define PIN_CS              17      // chip select (active low)
#define PIN_SCK             18      // SPI clock
#define PIN_MOSI            19      // SPI data transmit (MOSI)
#define PIN_DC              20      // data/command mode (low for command)
#define PIN_R               21      // reset (active low)

// modes for the ssd1309 data/command pin (see ssd1309 datasheet)
#define DC_COMMAND_MODE     0
#define DC_DATA_MODE        1


// global variables
int dma_ch_fb_transfer;             // DMA channel for frame buffer transfers
int dma_ch_refresh_delay;           // DMA channel for the frame period delay 
int dma_ch_fb_clear;                // DMA channel for clearing the screen
uint8_t frame_buffer[NUM_X_PIXELS * NUM_Y_PIXELS / 8]__attribute__((aligned(32)));
                                    // the frame buffer (about 1kByte), aligned to a 32-bit address
                                    // boundary so that we can clear the screen with 32-bit transfers


// a function to clear the frame buffer very quickly using a DMA channel
void fb_clear() {
    dma_channel_transfer_to_buffer_now(
        dma_ch_fb_clear, 
        frame_buffer,
        dma_encode_transfer_count(sizeof(frame_buffer) / sizeof(uint32_t))
    );
    dma_channel_wait_for_finish_blocking(dma_ch_fb_clear);
}

// initialise the SPI interface, DMA channels and display
int display_init() {
    // configure the SPI controller for 8-bit transfers using Motorola SPI mode 0
    spi_init(SPI_DEVICE, DISP_SPI_BITRATE);
    spi_set_format(SPI_DEVICE, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // configure our interface pins
    gpio_set_function(PIN_CS,     GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,    GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI,   GPIO_FUNC_SPI);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_R);
    gpio_set_dir(PIN_R, GPIO_OUT);

    // claim free DMA channels (or panic)
    dma_ch_fb_transfer = dma_claim_unused_channel(true);        // used to transfer the frame buffer to SPI
    dma_ch_refresh_delay = dma_claim_unused_channel(true);      // used to delay for the frame refresh period
    dma_ch_fb_clear = dma_claim_unused_channel(true);           // used to clear the frame buffer

    // configure a DMA channel that transfers the frame buffer to the SPI device, then triggers the frame delay channel
    dma_channel_config_t dma_config_fb_transfer = dma_channel_get_default_config(dma_ch_fb_transfer);
    channel_config_set_transfer_data_size(&dma_config_fb_transfer, DMA_SIZE_8);         // use 8-bit transfers
    channel_config_set_dreq(&dma_config_fb_transfer, spi_get_dreq(SPI_DEVICE, true));   // paced by SPI TX DREQ
    channel_config_set_chain_to(&dma_config_fb_transfer, dma_ch_refresh_delay);         // chain to frame_delay
    dma_channel_configure(
        dma_ch_fb_transfer,
        &dma_config_fb_transfer,        // use the configuration that we just created
        &spi_get_hw(SPI_DEVICE)->dr,    // write to the data register of the SPI device (non-incrementing)
        frame_buffer,                   // set the initial read address to the start of the frame buffer
        dma_encode_transfer_count(count_of(frame_buffer)),  // transfer the whole frame buffer
        false                           // don't trigger yet
    );

    // claim a DMA pacing timer for the frame delay channel (or panic)
    int dma_timer_frame_delay = dma_claim_unused_timer(true);
    dma_timer_set_fraction(dma_timer_frame_delay, 1, 0xffff);           // set the maximum clock divider
    uint frame_delay_num_transfers = clock_get_hz(clk_sys) / 0xffff;    // calculate number of transfers for the frame delay

    // configure a DMA channel that takes as long as the frame period, then triggers the frame-transfer channel
    // the transfers aren't wasted: we use them to reset the read address of the other channel 
    dma_channel_config_t dma_config_framerate_delay = dma_channel_get_default_config(dma_ch_refresh_delay);
    channel_config_set_read_increment(&dma_config_framerate_delay, false);          // don't increment the read address
    channel_config_set_dreq(&dma_config_framerate_delay, dma_get_timer_dreq(dma_timer_frame_delay)); // use our pacing timer
    channel_config_set_chain_to(&dma_config_framerate_delay, dma_ch_fb_transfer);   // chain to frame_transfer
    static uint32_t fb_start = (uint32_t)frame_buffer;
    dma_channel_configure(
        dma_ch_refresh_delay,
        &dma_config_framerate_delay,    // the configuration that we just created
        &dma_hw->ch[dma_ch_fb_transfer].read_addr,    // write the frame_transfer channel read address register (32-bits)
        &fb_start,                      // initial read address: start of frame buffer (increments, 32-bit reads)
        dma_encode_transfer_count(frame_delay_num_transfers),  // do the transfer enough times to get the required delay
        false                           // don't trigger yet
    );

    // configure a DMA channel that fills the frame buffer with zeros
    dma_channel_config_t dma_config_fb_clear = dma_channel_get_default_config(dma_ch_fb_clear);
    channel_config_set_write_increment(&dma_config_fb_clear, true);     // increment the write address
    channel_config_set_read_increment(&dma_config_fb_clear, false);     // don't increment the read address
    static uint32_t fb_clear_value = 0x00000000;    // this will be our source data for the transfer
    dma_channel_configure(
        dma_ch_fb_clear,
        &dma_config_fb_clear,           // the configuration that we just created
        frame_buffer,                   // initial write address: start of frame buffer (increments, 32-bit writes)
        &fb_clear_value,                // read address: clear_value (does not increment, 32-bit reads)
        dma_encode_transfer_count(sizeof(frame_buffer) / sizeof(uint32_t)), // number of 32-bit words in the frame buffer
        false                           // don't trigger yet
    );

    // initialise display
    gpio_put(PIN_R, 0);                 // generate a reset pulse (active low)
    sleep_ms(1);
    gpio_put(PIN_R, 1);

    // after a short pause, send the commands to initialise the display
    sleep_ms(1);
    gpio_put(PIN_DC, DC_COMMAND_MODE);  // put the interface into command mode
    uint8_t cmd_list[] = { 0xaf, 0x20, 0x00 };  // commands for 'display on' and 'horizontal addressing mode' (see datasheet)
    spi_write_blocking(SPI_DEVICE, cmd_list, sizeof(cmd_list));
    gpio_put(PIN_DC, DC_DATA_MODE);     // return the interface to data mode
        
    fb_clear();                         // clear the frame buffer
}


// convenience functions to set and clear pixel positions in the frame buffer
void set_pixel_xy(uint x, uint y) {
    if (x < NUM_X_PIXELS && y < NUM_Y_PIXELS) {
        frame_buffer[x + (y / 8) * NUM_X_PIXELS] |= (1 << (y % 8));
    }
}

void clear_pixel_xy(uint x, uint y) {
    if (x < NUM_X_PIXELS && y < NUM_Y_PIXELS) {
        frame_buffer[x + (y / 8) * NUM_X_PIXELS] &= ~(1 << (y % 8));
    }
}


int main(){
    stdio_init_all();
    display_init();
    fb_clear();

    // start the DMA transfer cycle
    dma_channel_start(dma_ch_fb_transfer);

    // You can now read and write the frame buffer at will, and the results will transparently
    // appear on the display without any CPU intervention! For details of the address layout
    // consult the manufacturer's datasheet (reference above).

    // NOTE: you can read and write the frame buffer at will, but once you have started the
    // DMA transfers running don't send any commands or data direct to the display otherwise
    // it will get very confused.
    
    
    once you have started the DMA transfer cycle, or it
    // will almost certainly lose sync with
    // the frame buffer. You could in theory send a command while the frame_delay transfer
    // is in progress (synchronised by the DMA interupt) but that is out of scope for this
    // example.

    // simple example: draw a moving 'snake'
    int head_x = NUM_Y_PIXELS - 1, head_y = NUM_Y_PIXELS - 1, head_dx = 1, head_dy = 1;
    int tail_x = 0, tail_y = 0, tail_dx = 1, tail_dy = 1;
    while(true) {
        set_pixel_xy(head_x, head_y);
        clear_pixel_xy(tail_x, tail_y);   

        // update head position
        if (head_x + head_dx < 0 || head_x + head_dx >= NUM_X_PIXELS) {
            head_dx = -head_dx;
        }
        head_x += head_dx;
        if (head_y + head_dy < 0 || head_y + head_dy >= NUM_Y_PIXELS) {
            head_dy = -head_dy;
        }
        head_y += head_dy;

        // update tail position
        if (tail_x + tail_dx < 0 || tail_x + tail_dx >= NUM_X_PIXELS) {
            tail_dx = -tail_dx;
        }
        tail_x += tail_dx;
        if (tail_y + tail_dy < 0 || tail_y + tail_dy >= NUM_Y_PIXELS) {
            tail_dy = -tail_dy;
        }
        tail_y += tail_dy;

        sleep_ms(5);
    }
}
