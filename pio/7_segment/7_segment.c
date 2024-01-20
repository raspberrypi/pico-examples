/**
 * Copyright (c) 2023 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
**/

#include <stdio.h>
#include "pico/stdlib.h"
#include "7_segment_lib.h"


const PIO pio = pio0;
const uint first_segment_pin = 8;   // gpio 15-8 = segments E,D,B,G,A,C,F,dp
const uint first_digit_pin = 16;    // gpio 19-16 = common anodes 4,3,2,1

//                      EDBGACF.       bit ordering depends on your display and wiring
const uint32_t Pico = 0b10111010 << 24 |    // 'P'
                      0b10000000 << 16 |    // 'i'
                      0b11010000 <<  8 |    // 'c'
                      0b11010100;           // 'o'

int main() {
    uint sm;
    stdio_init_all();
    
    if (seven_segment_init (pio, &sm, first_segment_pin, first_digit_pin)) {
        puts ("running");

        // display scrolling 'Pico'
        for (int shift = 24; shift >= 0; shift -= 8) {
            pio_sm_put (pio, sm, Pico >> shift);
            sleep_ms (1000);
        }

        // count to 9999
        for (int i = 0; i < 10000; i += 1) {
            sleep_ms (100);
            pio_sm_put (pio, sm, int_to_seven_segment (i));
        }
    }

    puts ("done");
    while (true);
}