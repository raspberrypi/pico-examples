/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Example code to drive a 16x2 LCD panel via an Adafruit TTL LCD "backpack"

   Optionally, the backpack can be connected the VBUS (pin 40) at 5V if
   the Pico in question is powered by USB for greater brightness.
   
   If this is done, then no other connections should be made to the backpack apart
   from those listed below as the backpack's logic levels will change.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO 8 (pin 11)-> RX on backpack
   3.3v (pin 36) -> 3.3v on backpack
   GND (pin 38)  -> GND on backpack
*/

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"

 // leave uart0 free for stdio
#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 8
#define LCD_WIDTH 16
#define LCD_HEIGHT 2

// basic commands
#define LCD_DISPLAY_ON 0x42
#define LCD_DISPLAY_OFF 0x46
#define LCD_SET_BRIGHTNESS 0x99
#define LCD_SET_CONTRAST 0x50
#define LCD_AUTOSCROLL_ON 0x51
#define LCD_AUTOSCROLL_OFF 0x52
#define LCD_CLEAR_SCREEN 0x58
#define LCD_SET_SPLASH 0x40

// cursor commands
#define LCD_SET_CURSOR_POS 0x47
#define LCD_CURSOR_HOME 0x48
#define LCD_CURSOR_BACK 0x4C
#define LCD_CURSOR_FORWARD 0x4D
#define LCD_UNDERLINE_CURSOR_ON 0x4A
#define LCD_UNDERLINE_CURSOR_OFF 0x4B
#define LCD_BLOCK_CURSOR_ON 0x53
#define LCD_BLOCK_CURSOR_OFF 0x54

// rgb commands
#define LCD_SET_BACKLIGHT_COLOR 0xD0
#define LCD_SET_DISPLAY_SIZE 0xD1

// change to 0 if display is not RGB capable
#define LCD_IS_RGB 1

void lcd_write(uint8_t cmd, uint8_t* buf, uint8_t buflen) {
    // all commands are prefixed with 0xFE
    const uint8_t pre = 0xFE;
    uart_write_blocking(UART_ID, &pre, 1);
    uart_write_blocking(UART_ID, &cmd, 1);
    uart_write_blocking(UART_ID, buf, buflen);
    sleep_ms(10); // give the display some time
}

void lcd_set_size(uint8_t w, uint8_t h) {
    // sets the dimensions of the display
    uint8_t buf[] = { w, h };
    lcd_write(LCD_SET_DISPLAY_SIZE, buf, 2);
}

void lcd_set_contrast(uint8_t contrast) {
    // sets the display contrast
    lcd_write(LCD_SET_CONTRAST, &contrast, 1);
}

void lcd_set_brightness(uint8_t brightness) {
    // sets the backlight brightness
    lcd_write(LCD_SET_BRIGHTNESS, &brightness, 1);
}

void lcd_set_cursor(bool is_on) {
    // set is_on to true if we want the blinking block and underline cursor to show
    if (is_on) {
        lcd_write(LCD_BLOCK_CURSOR_ON, NULL, 0);
        lcd_write(LCD_UNDERLINE_CURSOR_ON, NULL, 0);
    } else {
        lcd_write(LCD_BLOCK_CURSOR_OFF, NULL, 0);
        lcd_write(LCD_UNDERLINE_CURSOR_OFF, NULL, 0);
    }
}

void lcd_set_backlight(bool is_on) {
    // turn the backlight on (true) or off (false)
    if (is_on) {
        lcd_write(LCD_DISPLAY_ON, (uint8_t *) 0, 1);
    } else {
        lcd_write(LCD_DISPLAY_OFF, NULL, 0);
    }
}

void lcd_clear() {
    // clear the contents of the display
    lcd_write(LCD_CLEAR_SCREEN, NULL, 0);
}

void lcd_cursor_reset() {
    // reset the cursor to (1, 1)
    lcd_write(LCD_CURSOR_HOME, NULL, 0);
}

#if LCD_IS_RGB
void lcd_set_backlight_color(uint8_t r, uint8_t g, uint8_t b) {
    // only supported on RGB displays!
    uint8_t buf[] = { r, g, b };
    lcd_write(LCD_SET_BACKLIGHT_COLOR, buf, 3);
}
#endif

void lcd_init() {
    lcd_set_backlight(true);
    lcd_set_size(LCD_WIDTH, LCD_HEIGHT);
    lcd_set_contrast(155);
    lcd_set_brightness(255);
    lcd_set_cursor(false);
}

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    uart_set_translate_crlf(UART_ID, false);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);

    bi_decl(bi_1pin_with_func(UART_TX_PIN, GPIO_FUNC_UART));

    lcd_init();

    // define startup sequence and save to EEPROM
    // no more or less than 32 chars, if not enough, fill remaining ones with spaces
    uint8_t splash_buf[] = "Hello LCD, from Pi Towers!      ";
    lcd_write(LCD_SET_SPLASH, splash_buf, LCD_WIDTH * LCD_HEIGHT);

    lcd_cursor_reset();
    lcd_clear();

#if LCD_IS_RGB
    uint8_t i = 0; // it's ok if this overflows and wraps, we're using sin
    const float frequency = 0.1f;
    uint8_t red, green, blue;
#endif

    while (1) {
        // send any chars from stdio straight to the backpack
        char c = getchar();
        // any bytes not followed by 0xFE (the special command) are interpreted
        // as text to be displayed on the backpack, so we just send the char
        // down the UART byte pipe!
        if (c < 128) uart_putc_raw(UART_ID, c); // skip extra non-ASCII chars
#if LCD_IS_RGB
        // change the display color on keypress, rainbow style!
        red = (uint8_t)(sin(frequency * i + 0) * 127 + 128);
        green = (uint8_t)(sin(frequency * i + 2) * 127 + 128);
        blue = (uint8_t)(sin(frequency * i + 4) * 127 + 128);
        lcd_set_backlight_color(red, green, blue);
        i++;
#endif
    }
}
