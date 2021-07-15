/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"

 // leave uart0 free for stdio
#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 8

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

void lcd_write(uint8_t cmd, uint8_t* buf, uint8_t buflen) {
    // all commands are prefixed with 0xFE
    const uint8_t pre = 0xFE;
    uart_write_blocking(UART_ID, &pre, 1);
    uart_write_blocking(UART_ID, &cmd, 1);
    uart_write_blocking(UART_ID, buf, buflen);
    sleep_ms(10);
}

void lcd_set_size(uint8_t w, uint8_t h) {
    uint8_t buf[] = { w, h };
    lcd_write(LCD_SET_DISPLAY_SIZE, buf, 2);
}

void lcd_set_contrast(uint8_t contrast) {
    lcd_write(LCD_SET_CONTRAST, &contrast, 1);
}

void lcd_set_brightness(uint8_t brightness) {
    lcd_write(LCD_SET_BRIGHTNESS, &brightness, 1);
}

void lcd_set_cursor(bool is_on) {
    if (is_on) {
        lcd_write(LCD_BLOCK_CURSOR_ON, NULL, 0);
        lcd_write(LCD_UNDERLINE_CURSOR_ON, NULL, 0);
    } else {
        lcd_write(LCD_BLOCK_CURSOR_OFF, NULL, 0);
        lcd_write(LCD_UNDERLINE_CURSOR_OFF, NULL, 0);
    }

}

void lcd_set_backlight(bool is_on) {
    uint8_t pre = 0;
    if (is_on) {
        lcd_write(LCD_DISPLAY_ON, &pre, 1);
    } else {
        lcd_write(LCD_DISPLAY_OFF, NULL, 0);
    }
}

void lcd_clear() {
    lcd_write(LCD_CLEAR_SCREEN, NULL, 0);
}

void lcd_cursor_reset() {
    lcd_write(LCD_CURSOR_HOME, NULL, 0);
}

void lcd_init() {
    lcd_set_backlight(true);
    lcd_set_size(16, 2);
    lcd_set_contrast(200);
    lcd_set_brightness(255);
    lcd_set_cursor(true);
    lcd_clear();
    lcd_cursor_reset();
}

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);

    bi_decl(bi_1pin_with_func(UART_TX_PIN, GPIO_FUNC_UART));

    //lcd_init();
    uart_puts(uart1, "HLLOE\n");
    while (1);
}
