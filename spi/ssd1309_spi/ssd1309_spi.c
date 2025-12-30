/**
 * Copyright (c) 2025 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
**/

// A simple example to display text on an ssd1309-based OLED display with
// an SPI interface. It should also work on a ssd1306 device (not tested).
//
// To understand the commands and addressing modes for the display refer to
// the manufacturer's datasheet at https://www.hpinfotech.ro/SSD1309.pdf


#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ctype.h"
#include "ssd1309_font.h"


// We will use the spi0 peripheral and the following GPIO pins (see the
// GPIO function select table in the RPi Pico datasheet).
#define SPI_DEVICE      spi0
#define PIN_CS          17      // chip select (active low)
#define PIN_SCK         18      // SPI clock
#define PIN_MOSI        19      // SPI data transmit (MOSI)
#define PIN_DC          20      // data/command mode (low for command)
#define PIN_R           21      // reset (active low)

// ssd1309 accepts a maximum SPI clock rate of 10 Mbit/sec, but for
// this simple example we don't need to go that fast.
#define SPI_BITRATE     1 * 1000 * 1000

// dimensions of the display
#define NUM_X_PIXELS    128
#define NUM_Y_PIXELS    64

// modes for the data/command pin
#define DC_COMMAND_MODE 0
#define DC_DATA_MODE    1


// send a command byte to the display
void send_command(uint8_t cmd_byte) {
    gpio_put(PIN_DC, DC_COMMAND_MODE);
    spi_write_blocking(SPI_DEVICE, &cmd_byte, 1);
}

// send a data byte to the display
void send_data(uint8_t data_byte) {
    gpio_put(PIN_DC, DC_DATA_MODE);
    spi_write_blocking(SPI_DEVICE, &data_byte, 1);
}

// set the text cursor position (row, col)
//
// `row` is the text row, from 0 (top) to (NUM_Y_PIXELS / 8) - 1
// `col` is the text column, from 0 (left) to (NUM_X_PIXELS / 8) - 1
void set_cursor_pos(uint text_row, uint text_col) {
    send_command(0xb0 + (text_row & 0x07));     // set the text row ('page') start address
    uint col = text_col * 8;
    send_command(col & 0x0f);                   // set the column start address (low nibble)
    send_command(0x10 + ((col & 0xf0) >> 4));   // set the column start address (high nibble)
}

// display a single text character at the cursor position
void display_char(char c) {
    uint font_index = FONT_INDEX_SPACE;             // default (space)
    c = toupper(c);
    if (isalpha(c)) {
        font_index = c - 'A' + FONT_INDEX_A;        // a-z and A-Z
    } else if (isdigit(c)) {
        font_index = c - '0' + FONT_INDEX_0;        // 0-9
    }
    gpio_put(PIN_DC, DC_DATA_MODE);
    spi_write_blocking(SPI_DEVICE, &(font[font_index * 8]), 8);
}

// display a null-terminated string at the cursor position
void display_string(const char *str) {
    while(*str) {
        display_char(*str);
        str += 1;
    }
}

// clear the display
void display_clear() {
    for (int text_row = 0; text_row < (NUM_Y_PIXELS / 8); text_row += 1) {
        set_cursor_pos(text_row, 0);
        for (int col = 0; col < NUM_X_PIXELS; col += 1) {
            send_data(0x00);
        }
    }
    set_cursor_pos(0, 0);
}

// initialise the SPI interface and reset the display
void display_init() {
    // ssd1309 communicates using Motorola SPI mode 0
    spi_init(SPI_DEVICE, SPI_BITRATE);
    spi_set_format(SPI_DEVICE, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST); // mode 0

    // configure and initialise our interface pins
    gpio_set_function(PIN_CS,   GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_R);
    gpio_set_dir(PIN_R, GPIO_OUT);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);

    // send an active-low reset pulse to the display
    gpio_put(PIN_R, 0);
    sleep_ms(1);
    gpio_put(PIN_R, 1);

    // after a short pause, send the 'display on' command
    sleep_ms(1);
    send_command(0xaf);
    display_clear();
}


int main() {
    stdio_init_all();

    // initialise the interface and reset the display
    display_init();

    // Show some text
    //
    // Note: in the display's default addressing mode, over-length 
    // strings will just wrap on the same row (see the datasheet)

    set_cursor_pos(0, 0);
    display_string("abcdefghijklmnop");

    set_cursor_pos(1, 3);
    display_string("qrstuvwxyz");

    // show a steadily increasing counter
    uint count = 0;
    char count_str[10];
    while(true) {
        snprintf(count_str, sizeof(count_str), "%u", count);
        set_cursor_pos(3, 7);
        display_string(count_str);
        count += 1;
        sleep_ms(250);
    }
}
