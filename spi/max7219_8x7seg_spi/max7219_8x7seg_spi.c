/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

/* Example code to talk to a Max7219 driving an 8 digit 7 segment display via SPI

   NOTE: The device is driven at 5v, but SPI communications are at 3v3

   Connections on Raspberry Pi Pico board and a generic Max7219 board, other
   boards may vary.

   * GPIO 17 (pin 22) Chip select -> CS on Max7219 board
   * GPIO 18 (pin 24) SCK/spi0_sclk -> CLK on Max7219 board
   * GPIO 19 (pin 25) MOSI/spi0_tx -> DIN on Max7219 board
   * 5v (pin 40) -> VCC on Max7219 board
   * GND (pin 38)  -> GND on Max7219 board

   Note: SPI devices can have a number of different naming schemes for pins. See
   the Wikipedia page at https://en.wikipedia.org/wiki/Serial_Peripheral_Interface
   for variations.

*/

// This defines how many Max7219 modules we have cascaded together, in this case, just the one.
#define NUM_MODULES 1

const uint8_t CMD_NOOP = 0;
const uint8_t CMD_DIGIT0 = 1; // Goes up to 8, for each line
const uint8_t CMD_DECODEMODE = 9;
const uint8_t CMD_BRIGHTNESS = 10;
const uint8_t CMD_SCANLIMIT = 11;
const uint8_t CMD_SHUTDOWN = 12;
const uint8_t CMD_DISPLAYTEST = 15;

#ifdef PICO_DEFAULT_SPI_CSN_PIN
static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}
#endif

#if defined(spi_default) && defined(PICO_DEFAULT_SPI_CSN_PIN)
static void write_register(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;
    cs_select();
    spi_write_blocking(spi_default, buf, 2);
    cs_deselect();
    sleep_ms(1);
}

static void write_register_all(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;
    cs_select();
    for (int i = 0; i< NUM_MODULES;i++) {
        spi_write_blocking(spi_default, buf, 2);
    }
    cs_deselect();
}
#endif


void display_num(int32_t num)
{
    int digit = 0;
    while (num && digit < 8) {
        write_register_all(CMD_DIGIT0 + digit, num % 10);
        num /= 10;
        digit++;
    }        
}

void clear()
{
    for (int i=0;i<8;i++) {
        write_register_all(CMD_DIGIT0 + i, 0);
    }
}


int main() {
    stdio_init_all();

#if !defined(spi_default) || !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
#warning spi/max7219_8x7seg_spi example requires a board with SPI pins
    puts("Default SPI pins were not defined");
#else

    printf("Hello, max7219! Drawing things on a 8 x 7 segment display since 2022...\n");

    // This example will use SPI0 at 10MHz.
    spi_init(spi_default, 10 * 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    // Make the SPI pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);

    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));

    // Send init sequence to device

    write_register_all(CMD_SHUTDOWN, 0);
    write_register_all(CMD_DISPLAYTEST, 0);
    write_register_all(CMD_SCANLIMIT, 7);
    write_register_all(CMD_DECODEMODE, 255);
    write_register_all(CMD_SHUTDOWN, 1);
    write_register_all(CMD_BRIGHTNESS, 8);

    clear();

    int j = 0;

    while (true) {
        display_num(j++);
        sleep_ms(1);
        if (j > 99999999) {
            j = 0;
        }
    }
#endif
}
