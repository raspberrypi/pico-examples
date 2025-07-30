/**
* Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef I2C_LCD_H
#define I2C_LCD_H
#include <stdbool.h>
#include <hardware/i2c.h>

/*
 * This is a library-like example - it demonstrates creating a driver that can be
 * instantiated multiple times to control more than one device, as well as hiding
 * internal driver state from user code, and showing how to document function
 * arguments and return values.
 */

// "Opaque" type; implementation defined "privately" inside the .c file
// so that user code cannot mess with the internal state of the driver
typedef struct i2c_lcd* i2c_lcd_handle;

/*! \brief Create an instance of the LCD driver
 *
 * Initializes the LCD driver and performs initial configuration of the display
 *
 * \param i2c_peripheral a reference to the I2C hardware bus that the LCD is connected to
 * \param address the I2C address of the LCD module
 * \return Pointer to newly created driver instance
 */
i2c_lcd_handle i2c_lcd_init(i2c_inst_t* i2c_peripheral, uint8_t address);

/*! \brief Move the cursor to a certain location on the LCD
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param x the column (0-indexed) to move the cursor to
 * \param y the row (0-indexed) to move the cursor to
 */
void i2c_lcd_set_cursor_location(i2c_lcd_handle inst, uint8_t x, uint8_t y);

/*! \brief Move the cursor to the beginning of a certain line on the LCD
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param line the line number (0-indexed) to the move the cursor to the beginning of
 */
void i2c_lcd_set_cursor_line(i2c_lcd_handle inst, uint8_t line);

/*! \brief Write a string into the display memory starting from the current cursor location
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param string the string to send to the display
 */
void i2c_lcd_write_string(i2c_lcd_handle inst, char* string);

/*! \brief Write a formatted string into the display memory starting from the current cursor location
 *
 * This is a convenience function to allow "printf" functionality to the display without
 * having to do snprintf on a buffer yourself
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param format the format string used to generate the final string send to the display
 * \param ... the arguments for string formatting
 */
void i2c_lcd_write_stringf(i2c_lcd_handle inst, const char* __restrict format, ...) _ATTRIBUTE ((__format__ (__printf__, 2, 3)));

/*! \brief Write a single character into the display memory starting from the current cursor location
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param c the character to write into display memory
 */
void i2c_lcd_write_char(i2c_lcd_handle inst, char c);

/*! \brief Write a string to each line of the display
 *
 * Convenience function to avoid having to move the cursor around when writing to
 * both lines of the display.
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param line1 the string to be shown on the first line of the display
 * \param line2 the string to be shown on the second line of the display
 */
void i2c_lcd_write_lines(i2c_lcd_handle inst, char* line1, char* line2);

/*! \brief Clear the contents of the display memory
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 */
void i2c_lcd_clear(i2c_lcd_handle inst);

/*! \brief Enable or disable the LCD backlight
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param enabled whether to enable the backlight
 */
void i2c_lcd_set_backlight_enabled(i2c_lcd_handle inst, bool enabled);

/*! \brief Control display visibility
 *
 * This allows "blanking" the display without actually clearing the memory contents
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param enabled whether the display should be visible
 */
void i2c_lcd_set_display_visible(i2c_lcd_handle inst, bool enabled);

/*! \brief Control display cursor visibility
 *
 * \param inst pointer to an instance of this driver, obtained from \link i2c_lcd_init
 * \param cursorVisible whether the cursor should be visible
 * \param blink whether the cursor should also blink
 */
void i2c_lcd_set_cursor_enabled(i2c_lcd_handle inst, bool cursorVisible, bool blink);

#endif //I2C_LCD_H
