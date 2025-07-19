/**
* Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef I2C_LCD_H
#define I2C_LCD_H
#include <stdbool.h>
#include <hardware/i2c.h>

typedef struct i2c_lcd* i2c_lcd_handle;

i2c_lcd_handle i2c_lcd_init(i2c_inst_t* handle, uint8_t address);
void i2c_lcd_setCursorLocation(i2c_lcd_handle inst, uint8_t x, uint8_t y);
void i2c_lcd_setCursorLine(i2c_lcd_handle inst, uint8_t line);
void i2c_lcd_writeString(i2c_lcd_handle inst, char* string);
void i2c_lcd_writeStringf(i2c_lcd_handle inst, const char* __restrict format, ...) _ATTRIBUTE ((__format__ (__printf__, 2, 3)));
void i2c_lcd_writeChar(i2c_lcd_handle inst, char c);
void i2c_lcd_writeLines(i2c_lcd_handle inst, char* line1, char* line2);
void i2c_lcd_clear(i2c_lcd_handle inst);
void i2c_lcd_setBacklightEnabled(i2c_lcd_handle inst, bool enabled);
void i2c_lcd_setDisplayVisible(i2c_lcd_handle inst, bool en);
void i2c_lcd_setCursorEnabled(i2c_lcd_handle inst, bool cusror, bool blink);

#endif //I2C_LCD_H
