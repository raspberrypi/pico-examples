/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq);

// by default flash leds on gpios 3-4
#ifndef PIO_BLINK_LED1_GPIO
#define PIO_BLINK_LED1_GPIO 3
#endif

// and flash leds on gpios 5-6
// or if the device supports more than 32 gpios, flash leds on 32-33
#ifndef PIO_BLINK_LED3_GPIO
#if NUM_BANK0_GPIOS <= 32
#define PIO_BLINK_LED3_GPIO 5
#else
#define PIO_BLINK_LED3_GPIO 32
#endif
#endif

int main() {
    setup_default_uart();

    assert(PIO_BLINK_LED1_GPIO < 31);
    assert(PIO_BLINK_LED3_GPIO < 31 || PIO_BLINK_LED3_GPIO >= 32);

    PIO pio[2];
    uint sm[2];
    uint offset[2];

    // Find a free pio and state machine and add the program
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&blink_program, &pio[0], &sm[0], &offset[0], PIO_BLINK_LED1_GPIO, 2, true);
    hard_assert(rc);
    printf("Loaded program at %u on pio %u\n", offset[0], PIO_NUM(pio[0]));

    // Start led1 flashing
    blink_pin_forever(pio[0], sm[0], offset[0], PIO_BLINK_LED1_GPIO, 4);

    // Claim the next state machine and start led2 flashing
    pio_sm_claim(pio[0], sm[0] + 1);
    blink_pin_forever(pio[0], sm[0] + 1, offset[0], PIO_BLINK_LED1_GPIO + 1, 3);

    if (PIO_BLINK_LED3_GPIO >= 32) {
        // Find a free pio and state machine and add the program
        rc = pio_claim_free_sm_and_add_program_for_gpio_range(&blink_program, &pio[1], &sm[1], &offset[1], PIO_BLINK_LED3_GPIO, 2, true);
        printf("Loaded program at %u on pio %u\n", offset[1], PIO_NUM(pio[1]));
    } else {
        // no need to load the program again
        rc = true;
        pio[1] = pio[0];
        sm[1] = sm[0] + 2;
        offset[1] = offset[0];
        pio_sm_claim(pio[1], sm[1]);
    }
    hard_assert(rc);

    // Start led3 flashing
    blink_pin_forever(pio[1], sm[1], offset[1], PIO_BLINK_LED3_GPIO, 2);

    // Claim the next state machine and start led4 flashing
    pio_sm_claim(pio[1], sm[1] + 1);
    blink_pin_forever(pio[1], sm[1] + 1, offset[1], PIO_BLINK_LED3_GPIO + 1, 1);

    // free up pio resources
    pio_sm_unclaim(pio[1], sm[1] + 1);
    if (PIO_BLINK_LED3_GPIO >= 32) {
        pio_remove_program_and_unclaim_sm(&blink_program, pio[1], sm[1], offset[1]);
    } else {
        pio_sm_unclaim(pio[1], sm[1]);
    }
    pio_sm_unclaim(pio[0], sm[0] + 1);
    pio_remove_program_and_unclaim_sm(&blink_program, pio[0], sm[0], offset[0]);

    // the program exits but the pio keeps running!
    printf("All leds should be flashing\n");
}

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (clock_get_hz(clk_sys) / (2 * freq)) - 3;
}
