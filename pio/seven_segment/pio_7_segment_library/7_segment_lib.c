#include <stdio.h>
#include "hardware/gpio.h"
#include "7_segment_lib.h"

// segment patterns for digits 0-9
//
// By convention the segments are labelled as follows:
//
//  AAAA
// F    B
// F    B
//  GGGG
// E    C
// E    C
//  DDDD  .
//
// the bit ordering should match the way you connect up the GPIOs. Here it is
//    EDBGACF.
static const uint8_t segments[] = {
    0b11101110,     // 0
    0b00100100,     // 1
    0b11111000,     // 2
    0b01111100,     // 3
    0b00110110,     // 4
    0b01011110,     // 5
    0b11011110,     // 6
    0b00101100,     // 7
    0b11111110,     // 8
    0b01111110      // 9
};


// Simple example of how to convert an integer between -999 and 9999
// into a 32-bit word representing up to four 7-segment digits.
// Credit to @lurch for the code to handle negative numbers.
//
uint32_t int_to_seven_segment (int num) {
    uint32_t word = 0;
    if (num < -999 || num > 9999) {
        // number out of range, display 'E' symbol
        //       EDBGACF.
        word = 0b11011010;
    } else { 
        if (num == 0) {
            word = segments[0];
        } else {
            bool negative = num < 0;
            if (negative) {
                num *= -1;
            }
            int bitshift;
            for (bitshift = 0; bitshift < 32 && num > 0; bitshift += 8) {
                word |= segments[num % 10] << bitshift;
                num /= 10;
            }
            if (negative) {
                // display '-' symbol
                //        EDBGACF.
                word |= 0b00010000 << bitshift;
            }
        }
    }
    return word;
}


// configures and initialises a PIO state machine
//
bool seven_segment_init (PIO pio, uint *p_sm, uint segment_pinbase, uint digit_pinbase) {
    // add the program to the PIO shared instruction memory
    if (pio_can_add_program (pio, &seven_segment_program) == false) {
        puts ("could not add the pio program");
        return false;
    }
    uint offset = pio_add_program (pio, &seven_segment_program);

    // claim a free state machine
    int sm = pio_claim_unused_sm (pio, false);
    if (sm == -1) {
        puts ("could not claim a state machine");
        return false;
    } else {
        *p_sm = (uint)sm;
    }

    // set segment pins to PIO output
    for (int pin = 0; pin < 8; pin += 1) {
        pio_gpio_init (pio, segment_pinbase + pin);
    }
    pio_sm_set_consecutive_pindirs (pio, *p_sm, segment_pinbase, 8, true);

    // set digit mux pins to PIO output
    for (int pin = 0; pin < 4; pin += 1) {
        pio_gpio_init (pio, digit_pinbase + pin);
    }
    pio_sm_set_consecutive_pindirs (pio, *p_sm, digit_pinbase, 4, true);

    // initialise X register to zero
    pio_sm_exec_wait_blocking (pio, *p_sm, pio_encode_mov (pio_x, pio_null));

    // configure and enable the state machine
    seven_segment_sm_init (pio, *p_sm, offset, segment_pinbase, digit_pinbase);

    return true;
}