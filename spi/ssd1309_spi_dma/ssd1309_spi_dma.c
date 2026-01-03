/**
 * Copyright (c) 2025 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
**/

// An example showing how to drive a ssd1309-based OLED panel from a frame buffer using
// DMA transfers over SPI. It should also work on a ssd1306 device (not tested).
//
// To understand the commands and addressing modes for the display refer to
// the manufacturer's datasheet at https://www.hpinfotech.ro/SSD1309.pdf

#include <stdio.h>
#include "pico/stdio/driver.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "string.h"

#include "font.h"

// dimensions of our OLED display panel
#define NUM_X_PIXELS        128
#define NUM_Y_PIXELS        64
#define PIXELS_PER_BYTE     8

// how often we want to refresh the display from the frame buffer
#define FRAME_PERIOD_MS     20

// ssd1309 accepts a maximum SPI clock rate of 10 Mbit/sec
#define DISPLAY_SPI_BITRATE 10 * 1000 * 1000

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
uint8_t frame_buffer[NUM_X_PIXELS * NUM_Y_PIXELS / PIXELS_PER_BYTE];
int dma_ch_transfer_fb;
volatile bool display_needs_refresh;
volatile uint8_t *printf_write_ptr;

// set up the DMA channels
void dma_init() {
    // transfer the frame buffer to the SPI
    // remember to reset the read address after each transfer
    dma_ch_transfer_fb = dma_claim_unused_channel(true);
    dma_channel_config_t c = dma_channel_get_default_config(dma_ch_transfer_fb);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(SPI_DEVICE, true));
    dma_channel_configure(
        dma_ch_transfer_fb,
        &c,
        &spi_get_hw(SPI_DEVICE)->dr,    // write address (doesn't increment)
        frame_buffer,                   // initial read address
        dma_encode_transfer_count(count_of(frame_buffer)),
        false                           // don't trigger yet
    );
}

// initialise the SPI interface
void interface_init() {
    // configure the SPI controller for 8-bit transfers using Motorola SPI mode 0
    spi_init(SPI_DEVICE, DISPLAY_SPI_BITRATE);
    spi_set_format(SPI_DEVICE, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // configure our interface pins
    gpio_set_function(PIN_CS,     GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,    GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI,   GPIO_FUNC_SPI);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_R);
    gpio_set_dir(PIN_R, GPIO_OUT);
}

// reset and initialise the display
void display_reset() {
    // send active-low reset pulse
    gpio_put(PIN_R, 0);
    sleep_ms(1);
    gpio_put(PIN_R, 1);
    sleep_ms(1);

    // wake the display and set horizontal addressing mode
    gpio_put(PIN_DC, DC_COMMAND_MODE); 
    uint8_t cmd_list[] = { 0xaf, 0x20, 0x00 };
    spi_write_blocking(SPI_DEVICE, cmd_list, sizeof(cmd_list));
    gpio_put(PIN_DC, DC_DATA_MODE);

    // clear the frame buffer and set it to refresh
    memset(frame_buffer, 0x00, sizeof(frame_buffer));
    printf_write_ptr = frame_buffer;
    display_needs_refresh = true;
}

bool frame_refresh_callback(__unused struct repeating_timer *t) {
    if (display_needs_refresh) {
        // reset read address and start transfer
        dma_channel_set_read_addr(dma_ch_transfer_fb, frame_buffer, true);
        display_needs_refresh = false;
    }
    return true;    // repeat timer
}


// convenience functions to set/clear pixels in the frame buffer
void set_pixel_xy(uint x, uint y) {
    if (x < NUM_X_PIXELS && y < NUM_Y_PIXELS) {
        frame_buffer[x + (y / 8) * NUM_X_PIXELS] |= (1 << (y % 8));
        display_needs_refresh = true;
    }
}

void clear_pixel_xy(uint x, uint y) {
    if (x < NUM_X_PIXELS && y < NUM_Y_PIXELS) {
        frame_buffer[x + (y / 8) * NUM_X_PIXELS] &= ~(1 << (y % 8));
        display_needs_refresh = true;
    }
}

// simple stdout callback for the display
void fb_out_chars(const char *buf, int len) {
    while (len) {
        memcpy(
            (void *)printf_write_ptr, 
            &font[FONT_BYTES_PER_CODE * (FONT_INDEX_START + (*buf) - FONT_CODE_START)],
            FONT_BYTES_PER_CODE
        );
        printf_write_ptr += FONT_BYTES_PER_CODE;
        buf += 1;
        len -= 1;
        // TODO: manage scrolling at end of display
    }
    display_needs_refresh = true;
}


int main(){
    stdio_init_all();

    // initialise the interface and display
    dma_init();
    interface_init();
    display_reset();

    // start the display referesh timer
    struct repeating_timer timer;
    add_repeating_timer_ms(FRAME_PERIOD_MS, frame_refresh_callback, NULL, &timer);

    // add a simple stdio driver for the display
    stdio_driver_t fb_stdio_driver = { fb_out_chars };
    stdio_set_driver_enabled(&fb_stdio_driver, true);

    printf("Hello, World!\n");

    // anything you write to the frame buffer will now be transferred transparently to the
    // display - for the memory layout consult the manufacturer's datasheet (reference above).

    // simple example: a moving 'snake'
    int head_x = NUM_Y_PIXELS - 1, head_y = NUM_Y_PIXELS - 1, head_dx = 1, head_dy = 1;
    int tail_x = FONT_BYTES_PER_CODE + 1, tail_y = FONT_BYTES_PER_CODE + 1, tail_dx = 1, tail_dy = 1;
    while(true) {
        set_pixel_xy(head_x, head_y);
        clear_pixel_xy(tail_x, tail_y);

        // update head position
        if (head_x + head_dx < 0 || head_x + head_dx >= NUM_X_PIXELS) {
            head_dx = -head_dx;
        }
        head_x += head_dx;
        if (head_y + head_dy <= FONT_BYTES_PER_CODE || head_y + head_dy >= NUM_Y_PIXELS) {
            head_dy = -head_dy;
        }
        head_y += head_dy;

        // update tail position
        if (tail_x + tail_dx < 0 || tail_x + tail_dx >= NUM_X_PIXELS) {
            tail_dx = -tail_dx;
        }
        tail_x += tail_dx;
        if (tail_y + tail_dy <= FONT_BYTES_PER_CODE || tail_y + tail_dy >= NUM_Y_PIXELS) {
            tail_dy = -tail_dy;
        }
        tail_y += tail_dy;

        sleep_ms(5);
    }
}
