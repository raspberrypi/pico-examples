/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
// Our assembled program:
#include "hello.pio.h"

// This example uses the default led pin
// You can change this by defining HELLO_PIO_LED_PIN to use a different gpio
#if !defined HELLO_PIO_LED_PIN && defined PICO_DEFAULT_LED_PIN
#define HELLO_PIO_LED_PIN PICO_DEFAULT_LED_PIN
#endif

// Check the pin is compatible with the platform
#if HELLO_PIO_LED_PIN >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

int main() {
#ifndef HELLO_PIO_LED_PIN
#warning pio/hello_pio example requires a board with a regular LED
#else
    PIO pio;
    uint sm;
    uint offset;

    setup_default_uart();

    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range so we can address gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&hello_program, &pio, &sm, &offset, HELLO_PIO_LED_PIN, 1, true);
    hard_assert(success);

    // Configure it to run our program, and start it, using the
    // helper function we included in our .pio file.
    printf("Using gpio %d\n", HELLO_PIO_LED_PIN);
    hello_program_init(pio, sm, offset, HELLO_PIO_LED_PIN);

    // The state machine is now running. Any value we push to its TX FIFO will
    // appear on the LED pin.
    // press a key to exit
    while (getchar_timeout_us(0) == PICO_ERROR_TIMEOUT) {
        // Blink
        pio_sm_put_blocking(pio, sm, 1);
        sleep_ms(500);
        // Blonk
        pio_sm_put_blocking(pio, sm, 0);
        sleep_ms(500);
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&hello_program, pio, sm, offset);
#endif
}
