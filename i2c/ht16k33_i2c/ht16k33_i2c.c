/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include <ctype.h>

/* Example code to drive a 4 digit 14 segment LED backpack using a HT16K33 I2C
   driver chip

   NOTE: The panel must be capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore I2C) cannot be used at 5v. In development the particular 
   device used allowed the PCB VCC to be 5v, but you can set the I2C voltage 
   to 3.3v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO 4 (pin 6)-> SDA on LED board
   GPIO 5 (pin 7)-> SCL on LED board
   GND (pin 38)  -> GND on LED board
   5v (pin 40)   -> VCC on LED board
   3.3v (pin 36) -> vi2c on LED board
*/

// How many digits are on our display.
#define NUM_DIGITS 4

// By default these display drivers are on bus address 0x70. Often there are
// solder on options on the PCB of the backpack to set an address between
// 0x70 and 0x77 to allow multiple devices to be used.
const int I2C_addr = 0x70;


// commands


#define HT16K33_SYSTEM_STANDBY  0x20
#define HT16K33_SYSTEM_RUN      0x21

#define HT16K33_SET_ROW_INT     0xA0

#define HT16K33_BRIGHTNESS      0xE0

// Display on/off/blink
#define HT16K33_DISPLAY_SETUP   0x80
// OR/clear these to display setup register
#define HT16K33_DISPLAY_OFF     0x0
#define HT16K33_DISPLAY_ON      0x1
#define HT16K33_BLINK_2HZ       0x2
#define HT16K33_BLINK_1HZ       0x4
#define HT16K33_BLINK_0p5HZ     0x6

// Converts a character to the bit pattern needed to display the right segments.
// These are pretty standard for 14segment LED's
uint16_t char_to_pattern(char ch) {
// Map, "A" to "Z"
int16_t alpha[] = {
    0xF7,0x128F,0x39,0x120F,0xF9,0xF1,0xBD,0xF6,0x1209,0x1E,0x2470,0x38,0x536,0x2136,
    0x3F,0xF3,0x203F,0x20F3,0x18D,0x1201,0x3E,0xC30,0x2836,0x2D00,0x1500,0xC09
    };

// Map, "0" to "9"
int16_t num[] = {
    0xC3F,0x406,0xDB,0x8F,0xE6,0xED,0xFD,0x1401,0xFF,0xE7
    };

    if (isalpha(ch))
        return alpha[toupper(ch) - 'A'];
    
    if (isdigit(ch))
        return num[ch - '0'];
    
    return 0;
}

/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val) {
#ifdef i2c_default
    i2c_write_blocking(i2c_default, I2C_addr, &val, 1, false);
#endif
}


void ht16k33_init() {
    i2c_write_byte(HT16K33_SYSTEM_RUN);
    i2c_write_byte(HT16K33_SET_ROW_INT);
    i2c_write_byte(HT16K33_DISPLAY_SETUP | HT16K33_DISPLAY_ON);
}

// Send a specific binary value to the specified digit
static inline void ht16k33_display_set(int position, uint16_t bin) {
    uint8_t buf[3];
    buf[0] = position * 2;
    buf[1] = bin & 0xff;
    buf[2] = bin >> 8;
#ifdef i2c_default
    i2c_write_blocking(i2c_default, I2C_addr, buf, count_of(buf), false);
#endif
}

static inline void ht16k33_display_char(int position, char ch) {
    ht16k33_display_set(position, char_to_pattern(ch));
}
    
void ht16k33_display_string(char *str) {
    int digit = 0;
    while (*str && digit <= NUM_DIGITS) {
        ht16k33_display_char(digit++, *str++);
    }
}

void ht16k33_scroll_string(char *str, int interval_ms) {
    int l = strlen(str);

    if (l <= NUM_DIGITS) {
        ht16k33_display_string(str);
    }
    else {
        for (int i = 0; i < l - NUM_DIGITS + 1; i++) {
            ht16k33_display_string(&str[i]);
            sleep_ms(interval_ms);
        }
    }
}

void ht16k33_set_brightness(int bright) {
    i2c_write_byte(HT16K33_BRIGHTNESS | (bright <= 15 ? bright : 15));
}

void ht16k33_set_blink(int blink) {
    int s = 0;
    switch (blink) {
        default: break;
        case 1: s = HT16K33_BLINK_2HZ; break;
        case 2: s = HT16K33_BLINK_1HZ; break;
        case 3: s = HT16K33_BLINK_0p5HZ; break;
    }

    i2c_write_byte(HT16K33_DISPLAY_SETUP | HT16K33_DISPLAY_ON | s);
}

int main() {

    stdio_init_all();

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/ht16k33_i2c example requires a board with I2C pins
#else
    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    printf("Welcome to HT33k16!\n");

    ht16k33_init();

    ht16k33_display_set(0, 0);
    ht16k33_display_set(1, 0);
    ht16k33_display_set(2, 0);
    ht16k33_display_set(3, 0);

again:

    ht16k33_scroll_string("Welcome to the Raspberry Pi Pico", 150);

    // Do a speeding up propeller effort using the inner segments
    int bits[] = {0x40, 0x0100, 0x0200, 0x0400, 0x80, 0x2000, 0x1000, 0x0800};
    for (int j = 0;j < 10;j++) {
        for (int i = 0;i< count_of(bits); i++) {
            for (int digit = 0;digit <= NUM_DIGITS; digit++) {
                ht16k33_display_set(digit, bits[i]);
            }
            sleep_ms(155 - j*15);
        }
    }

    char *strs[] = {
        "Help", "I am", "in a", "Pico", "and ", "Cant", "get ", "out "
    };

    for (int i = 0; i < count_of(strs); i++) {
        ht16k33_display_string(strs[i]);
        sleep_ms(500);
    }

    sleep_ms(1000);

    // Test brightness and blinking

    // Set all segments on all digits on
    ht16k33_display_set(0, 0xffff);
    ht16k33_display_set(1, 0xffff);
    ht16k33_display_set(2, 0xffff);
    ht16k33_display_set(3, 0xffff);

    // Fade up and down
    for (int j=0;j<5;j++) {
        for (int i = 0; i < 15; i++) {
            ht16k33_set_brightness(i);
            sleep_ms(30);
        }

        for (int i = 14; i >=0; i--) {
            ht16k33_set_brightness(i);
            sleep_ms(30);
        }
    }

    ht16k33_set_brightness(15);

    ht16k33_set_blink(1); // 0 for no blink, 1 for 2Hz, 2 for 1Hz, 3 for 0.5Hz
    sleep_ms(5000);
    ht16k33_set_blink(0);

    goto again;
#endif
}
