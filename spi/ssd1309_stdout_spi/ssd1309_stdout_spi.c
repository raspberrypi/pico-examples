/**
 * Copyright (c) 2026 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
**/

// Copy stdout and/or simple graphics to an OLED display panel based on the ssd1309 
// (or compatible) controller, using a frame buffer transferred by DMA over an SPI interface.
//
// For details of the display controller and its commands and addressing modes refer to the 
// manufacturer's datasheet at https://www.hpinfotech.ro/SSD1309.pdf

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdio/driver.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "font.h"

// dimensions of the display panel
#define NUM_X_PIXELS        128
#define NUM_Y_PIXELS        64
#define PIXELS_PER_BYTE     8
#define TABSTOPS            4

// how often we want to refresh the display from the frame buffer
#define FRAME_PERIOD_MS     20      // 50 Hz

// clock rate for the SPI interface
// the ssd1309 is specified up to 10 Mbit/sec
#define DISPLAY_SPI_BITRATE 10 * 1000 * 1000

// pins to use for the SPI interface
// we will use the spi0 peripheral and the following GPIO pins (see the
// GPIO function select table in the Pico datasheet).
#define SPI_DEVICE          spi0
#define PIN_CS              17      // chip select (active low)
#define PIN_SCK             18      // SPI clock
#define PIN_MOSI            19      // SPI data transmit (MOSI)
#define PIN_DC              20      // data/command mode (low for command)
#define PIN_R               21      // reset (active low)

// modes for the ssd1309 data/command pin (see the datasheet)
#define DC_COMMAND_MODE     0
#define DC_DATA_MODE        1

// global variables
uint8_t frame_buffer[ NUM_X_PIXELS * NUM_Y_PIXELS / PIXELS_PER_BYTE ];
int dma_ch_transfer_fb;
volatile bool display_needs_refresh = false;
volatile uint fb_cursor_index = 0;


// configure a dma channel to send the frame buffer over SPI
void dma_init() {
    dma_ch_transfer_fb = dma_claim_unused_channel(true);
    dma_channel_config_t c = dma_channel_get_default_config(dma_ch_transfer_fb);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(SPI_DEVICE, true));
    dma_channel_configure(
        dma_ch_transfer_fb,
        &c,                             // the channel_config above
        &spi_get_hw(SPI_DEVICE)->dr,    // write address (doesn't increment)
        frame_buffer,                   // initial read address
        dma_encode_transfer_count(count_of(frame_buffer)),
        false                           // don't trigger yet
    );
}

// initialise the SPI interface
void interface_init() {
    // configure the SPI controller for 8-bit transfers and Motorola SPI mode 0
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

    // wake up the display and set horizontal addressing mode
    gpio_put(PIN_DC, DC_COMMAND_MODE); 
    uint8_t cmd_list[] = { 0xaf, 0x20, 0x00 };
    spi_write_blocking(SPI_DEVICE, cmd_list, sizeof(cmd_list));
    gpio_put(PIN_DC, DC_DATA_MODE);

    // clear the frame buffer and flag it to be transferred
    memset(frame_buffer, 0x00, sizeof(frame_buffer));
    fb_cursor_index = 0;
    display_needs_refresh = true;
}


// a simple stdout driver for the display
void fb_out_chars(const char *buf, int len) {
    uint font_index;
    while (len) {
        char code = *buf;
        buf += 1;
        len -= 1;
        while (fb_cursor_index >= sizeof(frame_buffer)) {
            // scroll frame buffer (on RP2350 you could use a decrementing dma transfer but it's probably overkill)
            memmove(frame_buffer, frame_buffer + NUM_X_PIXELS, sizeof(frame_buffer) - NUM_X_PIXELS);
            fb_cursor_index -= NUM_X_PIXELS;
            memset(&frame_buffer[sizeof(frame_buffer) - NUM_X_PIXELS - 1], 0x00, NUM_X_PIXELS);
        }
        // handle control codes
        if (code == '\n') {
            fb_cursor_index = (fb_cursor_index / NUM_X_PIXELS + 1) * NUM_X_PIXELS;
        } else if (code == '\t') {
            fb_cursor_index = (fb_cursor_index / (TABSTOPS * FONT_BYTES_PER_CODE) + 1) * TABSTOPS * FONT_BYTES_PER_CODE;
        } else {
            // handle alphanumeric codes
            if (code < FONT_CODE_FIRST || code > FONT_CODE_LAST ) {
                font_index = FONT_INDEX_UNDEF;
            } else {
                font_index = FONT_BYTES_PER_CODE * (FONT_INDEX_START + code - FONT_CODE_FIRST);
            }
            // copy bitmap to frame buffer and advance cursor
            memcpy(&frame_buffer[fb_cursor_index], &font[font_index], FONT_BYTES_PER_CODE);
            fb_cursor_index += FONT_BYTES_PER_CODE;
        }
    }
    display_needs_refresh = true;
}


// some simple graphics funtions (for the display memory layout see the datasheet)
void set_pixel(uint x, uint y) {
    if (x < NUM_X_PIXELS && y < NUM_Y_PIXELS) {
        frame_buffer[x + (y / 8) * NUM_X_PIXELS] |= (1 << (y % 8));
        display_needs_refresh = true;
    }
}

void clear_pixel(uint x, uint y) {
    if (x < NUM_X_PIXELS && y < NUM_Y_PIXELS) {
        frame_buffer[x + (y / 8) * NUM_X_PIXELS] &= ~(1 << (y % 8));
        display_needs_refresh = true;
    }
}

void draw_line(int x0, int y0, int x1, int y1) {
  int dx =  abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1; 
  int err = dx + dy, e2;
  while(true) {
    set_pixel (x0, y0);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
  display_needs_refresh = true;
}


// set the text output position
// rows go from 0 at the top to NUM_Y_PIXELS/8 - 1 and columns go from 0 on the left to NUM_X_PIXELS/8 - 1
void set_cursor_pos(uint text_row, uint text_col) {
    if (text_row < NUM_Y_PIXELS / PIXELS_PER_BYTE && text_col < NUM_X_PIXELS / FONT_BYTES_PER_CODE) {
        fb_cursor_index = text_row * NUM_X_PIXELS + text_col * FONT_BYTES_PER_CODE;
    }
}

// the callback function for our frame-rate timer
bool frame_refresh_callback(__unused struct repeating_timer *t) {
    if (display_needs_refresh) {
        dma_channel_set_read_addr(dma_ch_transfer_fb, frame_buffer, true);  // reset and trigger the dma channel
        display_needs_refresh = false;
    }
    return true;    // reschedule the timer
}


int main(){
    stdio_init_all();

    // initialise everything
    dma_init();
    interface_init();
    display_reset();

    // start refreshing the display
    struct repeating_timer timer;
    add_repeating_timer_ms(FRAME_PERIOD_MS, frame_refresh_callback, NULL, &timer);

    // install our simple driver to copy stdout to the frame buffer
    stdio_driver_t fb_stdio_driver = { fb_out_chars };
    stdio_set_driver_enabled(&fb_stdio_driver, true);

    // display some text
    set_cursor_pos(0, 2);
    printf("Hello, World\n");

    // show a moving 'snake'
    int head_x = NUM_Y_PIXELS - 1, head_y = NUM_Y_PIXELS - 1, head_dx = 1, head_dy = 1;
    int tail_x = FONT_BYTES_PER_CODE + 1, tail_y = FONT_BYTES_PER_CODE + 1, tail_dx = 1, tail_dy = 1;
    while(true) {
        set_pixel(head_x, head_y);
        clear_pixel(tail_x, tail_y);
        if (head_x + head_dx < 0 || head_x + head_dx >= NUM_X_PIXELS) {
            head_dx = -head_dx;
        }
        head_x += head_dx;
        if (head_y + head_dy <= FONT_BYTES_PER_CODE || head_y + head_dy >= NUM_Y_PIXELS) {
            head_dy = -head_dy;
        }
        head_y += head_dy;
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
