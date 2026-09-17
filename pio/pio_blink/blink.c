/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "blink.pio.h"

/**
 * This example demonstrates using PIO to flash four LEDs at different frequencies.
 * On RP2040 and RP2350A those GPIOs are all in the same "PIO range" of 0 - 31 (so a single PIO is used).
 * On RP2350B the first two GPIOs are in the "lower PIO range" of 0 - 31 and the
 * next two LEDs are in the "higher PIO range" of 16 - 47 (so two PIOs are used).
 */

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq);

// Time to flash the LEDs for (in seconds), before cleanly shutting everything down. Set to -1 to keep flashing forever.
#ifndef PIO_BLINK_FLASH_TIME_SECONDS
#define PIO_BLINK_FLASH_TIME_SECONDS 60
#endif

// By default flash LEDs on GPIOs 3 and 4
#ifndef PIO_BLINK_LED1_GPIO
#define PIO_BLINK_LED1_GPIO 3
#define PIO_BLINK_LED2_GPIO (PIO_BLINK_LED1_GPIO + 1)
#endif

// and also flash LEDs on GPIOs 5 and 6
// Or if the device has more than 32 gpios, also flash LEDs on GPIOs 32 and 33
#ifndef PIO_BLINK_LED3_GPIO
#if NUM_BANK0_GPIOS <= 32
#define PIO_BLINK_LED3_GPIO 5
#else
#define PIO_BLINK_LED3_GPIO 32
#endif
#define PIO_BLINK_LED4_GPIO (PIO_BLINK_LED3_GPIO + 1)
#endif

#ifndef PIO_BLINK_LED1_FREQUENCY
#define PIO_BLINK_LED1_FREQUENCY 4
#endif
#ifndef PIO_BLINK_LED2_FREQUENCY
#define PIO_BLINK_LED2_FREQUENCY 3
#endif
#ifndef PIO_BLINK_LED3_FREQUENCY
#define PIO_BLINK_LED3_FREQUENCY 2
#endif
#ifndef PIO_BLINK_LED4_FREQUENCY
#define PIO_BLINK_LED4_FREQUENCY 1
#endif

int main() {
    // useful information for picotool
    bi_decl(bi_program_description("PIO blink example for the Raspberry Pi Pico"));
#if (PIO_BLINK_LED3_GPIO < 32) && (PIO_BLINK_LED4_GPIO < 32)
    bi_decl(bi_4pins_with_func(PIO_BLINK_LED1_GPIO, PIO_BLINK_LED2_GPIO, PIO_BLINK_LED3_GPIO, PIO_BLINK_LED4_GPIO, GPIO_FUNC_PIO0));
#else
    bi_decl(bi_2pins_with_func(PIO_BLINK_LED1_GPIO, PIO_BLINK_LED2_GPIO, GPIO_FUNC_PIO0));
    bi_decl(bi_2pins_with_func(PIO_BLINK_LED3_GPIO, PIO_BLINK_LED4_GPIO, GPIO_FUNC_PIO1));
#endif

    setup_default_uart();

    // LED1 and LED2 are both expected to be in the "lower" range of PIO-addressable GPIOs
    assert((PIO_BLINK_LED1_GPIO < 32) && (PIO_BLINK_LED2_GPIO < 32));
    // check LED3 and LED4 are both in the same range of PIO-addressable GPIOs
    assert(((PIO_BLINK_LED3_GPIO < 32) && (PIO_BLINK_LED4_GPIO < 32)) || ((PIO_BLINK_LED3_GPIO >= 16) && (PIO_BLINK_LED3_GPIO < 48) && (PIO_BLINK_LED4_GPIO >= 16) && (PIO_BLINK_LED4_GPIO < 48)));

    // LED1 and LED2 are both controlled by the program loaded into pio[0] at offset[0]
    // LED3 and LED4 are both controlled by the program loaded into pio[1] at offset[1] (which might be the same as pio[0] and offset[0] if LED3 and LED4 are both on GPIOs < 32)
    // LED1 is controlled by sm[0]
    // LED2 is controlled by sm[1]
    // LED3 is controlled by sm[2]
    // LED4 is controlled by sm[3]
    PIO pio[2];
    uint sm[4];
    uint offset[2];

    // Find a free PIO and state machine and add the program
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&blink_program, &pio[0], &sm[0], &offset[0], PIO_BLINK_LED1_GPIO, 2, true);
    hard_assert(rc);
    printf("Loaded program at %u on pio %u\n", offset[0], PIO_NUM(pio[0]));

    // Start LED1 flashing
    blink_pin_forever(pio[0], sm[0], offset[0], PIO_BLINK_LED1_GPIO, PIO_BLINK_LED1_FREQUENCY);

    // Claim the next unused state machine and start LED2 flashing
    sm[1] = pio_claim_unused_sm(pio[0], true);
    blink_pin_forever(pio[0], sm[1], offset[0], PIO_BLINK_LED2_GPIO, PIO_BLINK_LED2_FREQUENCY);

    if ((PIO_BLINK_LED3_GPIO >= 32) || (PIO_BLINK_LED4_GPIO >= 32)) {
        // Find a free PIO and state machine and add the program
        rc = pio_claim_free_sm_and_add_program_for_gpio_range(&blink_program, &pio[1], &sm[2], &offset[1], PIO_BLINK_LED3_GPIO, 2, true);
        printf("Loaded program at %u on pio %u\n", offset[1], PIO_NUM(pio[1]));
    } else {
        // no need to load the program again
        rc = true;
        pio[1] = pio[0];
        offset[1] = offset[0];
        sm[2] = pio_claim_unused_sm(pio[1], true);
    }
    hard_assert(rc);

    // Start LED3 flashing
    blink_pin_forever(pio[1], sm[2], offset[1], PIO_BLINK_LED3_GPIO, PIO_BLINK_LED3_FREQUENCY);

    // Claim the next unused state machine and start LED4 flashing
    sm[3] = pio_claim_unused_sm(pio[1], true);
    blink_pin_forever(pio[1], sm[3], offset[1], PIO_BLINK_LED4_GPIO, PIO_BLINK_LED4_FREQUENCY);

    printf("All LEDs should be flashing\n");
    if (PIO_BLINK_FLASH_TIME_SECONDS < 0) {
        // the program exits but the PIO keeps running!
        printf("All LEDs should continue to flash after program exit\n");
    } else {
        // Sleep for a bit, and then shut everything down (demonstrates how to release claimed PIO resources)
        sleep_ms(PIO_BLINK_FLASH_TIME_SECONDS * 1000);
        printf("Stopping the LEDs\n");

        // stop the PIO state machines
        if (pio[0] == pio[1]) {
            // all state machines are running on the same PIO, so can be stopped with a single call
            pio_set_sm_mask_enabled(pio[0], (1 << sm[0]) | (1 << sm[1]) | (1 << sm[2]) | (1 << sm[3]), false);
        } else {
            pio_set_sm_mask_enabled(pio[0], (1 << sm[0]) | (1 << sm[1]), false);
            pio_set_sm_mask_enabled(pio[1], (1 << sm[2]) | (1 << sm[3]), false);
        }

        // turn off the LEDs in case any of the state machines stopped while they were on
        uint led_off_instr = pio_encode_set(pio_pins, 0);
        pio_sm_exec_wait_blocking(pio[0], sm[0], led_off_instr);
        pio_sm_exec_wait_blocking(pio[0], sm[1], led_off_instr);
        pio_sm_exec_wait_blocking(pio[1], sm[2], led_off_instr);
        pio_sm_exec_wait_blocking(pio[1], sm[3], led_off_instr);
        printf("All LEDs should be off\n");

        // free up pio resources
        pio_sm_unclaim(pio[1], sm[3]);
        if (pio[1] != pio[0]) {
            pio_remove_program_and_unclaim_sm(&blink_program, pio[1], sm[2], offset[1]);
        } else {
            pio_sm_unclaim(pio[1], sm[2]);
        }
        pio_sm_unclaim(pio[0], sm[1]);
        pio_remove_program_and_unclaim_sm(&blink_program, pio[0], sm[0], offset[0]);
    }
}

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (clock_get_hz(clk_sys) / (2 * freq)) - 3;
}
