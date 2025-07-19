/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "i2c_lcd.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

/*
 * Example code to drive a 16x2 LCD panel via a I2C bridge chip (e.g. PCF8574)
 * NOTE: The panel must be capable of being driven at 3.3v NOT 5v. The RP2040
 * is not 5V tolerant.
 *
 * You will need to use a level shifter on the I2C lines if you want to run the
 * board at 5V. Using it without a level shifter will work but may damage the I/O
 * cells on the RP2040 over time since the I2C pullups on the display will pull
 * the data lines up to 5V.
 *
 * Connections on Raspberry Pi Pico board, other boards may vary.
 * GPIO 0 -> SDA on LCD bridge board
 * GPIO 1 -> SCL on LCD bridge board
 */

// You may need to change this for your LCD module
#define I2_LCD_ADDR 0x27

int main()
{
	// Set up USB virtual COM port
	stdio_init_all();

	// Set up the I2C peripheral at 100k
	i2c_init(i2c0, 100 * 1000);

	// Bind pins to I2C peripheral
	gpio_set_function(0, GPIO_FUNC_I2C);
	gpio_set_function(1, GPIO_FUNC_I2C);

	// Wait long enough for power rails to rise on the LCD module
	sleep_ms(100);

	// Create an instance of the LCD driver (you can create multiple instances
	// either with the same address on different i2c ports or with different
	// addresses on the same i2c port)
	i2c_lcd_handle lcd = i2c_lcd_init(i2c0, I2_LCD_ADDR);

	// Fairly self-explanatory
	i2c_lcd_setBacklightEnabled(lcd, true);

	// Demonstrate writing to the different lines on the display
	int counter = 0;
	while (1)
	{
		i2c_lcd_setCursorLine(lcd, 0);
		i2c_lcd_writeStringf(lcd, "Hello World!");
		i2c_lcd_setCursorLine(lcd, 1);
		i2c_lcd_writeStringf(lcd, "Counter = %d", counter);
		counter++;
	}
}
