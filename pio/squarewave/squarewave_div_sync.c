/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"

// Our assembled program:
#include "squarewave.pio.h"

// Attach a logic analyser to these gpios
// A square wave is generated on them with a period of around 1748us by using a clock div of 65535
// This example demonstrates how the call to pio_clkdiv_restart_sm_multi_mask can be used to get
// the state machines dividers in sync. After 5s it stops all the state machines and exits
#define GPIO_PIN0 2
#define GPIO_PIN1 3
#define GPIO_PIN2 4

// Clock divisor to use, as slow as possible
#define CLOCK_DIVISOR 65535

// This example enables each state machines in turn then synchronizes the divisors
// with a call to pio_clkdiv_restart_sm_multi_mask
// The same result can be achieved with one function call to pio_enable_sm_multi_mask_in_sync
// Set this to 1 to test on rp2350
#define USE_ENABLE_MULTI_MASK_IN_SYNC_FUNCTION 0

static bool setup_pio(const pio_program_t *program, PIO *pio, uint *sm, uint *offset, uint pin, uint16_t divisor) {

    // load the program on a free pio and state machine
    // Note: This uses the LAST free pio first to reduce the chance of causing problems for old code
    // that assumes the first pio is free
    if (!pio_claim_free_sm_and_add_program(program, pio, sm, offset)) {
        return false;
    }

    // Set this pin's GPIO function (connect PIO to the pad)
    pio_gpio_init(*pio, pin);

    // Set the pin direction to output with the PIO
    pio_sm_set_consecutive_pindirs(*pio, *sm, pin, 1, true);

    pio_sm_config c = squarewave_program_get_default_config(*offset);
    sm_config_set_set_pins(&c, pin, 1);

    // Load our configuration, and jump to the start of the program
    pio_sm_init(*pio, *sm, *offset, &c);

    // set the pio divisor
    pio_sm_set_clkdiv(*pio, *sm, divisor);
    return true;
}

int main() {
    PIO pio[3];
    uint sm[3];
    uint offset[3];
    bool success;

    // Load the program onto the first pio
    success = setup_pio(&squarewave_program, &pio[0], &sm[0], &offset[0], GPIO_PIN0, CLOCK_DIVISOR);
    hard_assert(success);
    assert(pio[0] == pio_get_instance(NUM_PIOS - 1)); // note: last pio

    // Load the program onto the next state machine
    success = setup_pio(&squarewave_program, &pio[1], &sm[1], &offset[1], GPIO_PIN1, CLOCK_DIVISOR);
    hard_assert(success);
    assert(pio[1] == pio_get_instance(NUM_PIOS - 1)); // note: last pio

    // claim the other state machines to make sure the next time we load the program, it goes to a different pio
    uint not_actually_used[2];
    not_actually_used[0] = pio_claim_unused_sm(pio[0], true);
    not_actually_used[1] = pio_claim_unused_sm(pio[0], true);

    // Loading the program onto a different pio
    success = setup_pio(&squarewave_program, &pio[2], &sm[2], &offset[2], GPIO_PIN2, CLOCK_DIVISOR);
    hard_assert(success);
    assert(pio[2] == pio_get_instance(NUM_PIOS - 2)); // note: second to last pio

#if PICO_PIO_VERSION == 0 || !USE_ENABLE_MULTI_MASK_IN_SYNC_FUNCTION
    // Set the first one going
    pio_sm_set_enabled(pio[0], sm[0], true);
    // set the second one running
    pio_sm_set_enabled(pio[1], sm[1], true);
    // set the last one running
    pio_sm_set_enabled(pio[2], sm[2], true);
#if PICO_PIO_VERSION > 0
    // You need a logic analyser to properly see what's going on here.
    // Without the call below, all the square waves on the 3 pins will be out of sync slightly
    // because they're all started at different times.
    // This call restarts the clock dividers on the 3 state machines so they're in sync.
    // This is only supported on newer versions of the PIO, e.g. on rp2350
    // We used pio_claim_free_sm_and_add_program which uses the last free pio first,
    // so we're setting the mask for the previous pio here
    pio_clkdiv_restart_sm_multi_mask(pio[0], 1 << sm[2], (1 << sm[0]) | (1 << sm[1]), 0);
#endif // PICO_PIO_VERSION > 0
#else // PICIO_PICO_VERSION != 0 && USE_ENABLE_MULTI_MASK_IN_SYNC_FUNCTION
    // As an alternative you could just use this to enable ALL state machines with the divisors all in sync
    pio_enable_sm_multi_mask_in_sync(pio[0], 1 << sm[2], (1 << sm[0]) | (1 << sm[1]), 0);
#endif

    // run the programs for 5s then stop the state machines
    sleep_ms(5000);
#if PICO_PIO_VERSION > 0
    // one call to stop all state machines
    pio_set_sm_multi_mask_enabled(pio[0], 1 << sm[2], (1 << sm[0]) | (1 << sm[1]), 0, false);
#else
    // two calls needed to stop all state machines on rp2040
    pio_set_sm_mask_enabled(pio[0], (1 << sm[0]) | (1 << sm[1]), false);
    pio_set_sm_mask_enabled(pio[2], 1 << sm[2], false);
#endif

    // Unclaim unused state machines
    pio_sm_unclaim(pio[0], not_actually_used[0]);
    pio_sm_unclaim(pio[0], not_actually_used[1]);

    // This will free resources and unload our program
    // We have two state machines on the same pio, the final state machine is on a different pio
    assert(pio[0] == pio[1] && pio[1] != pio[2]);
    pio_remove_program_and_unclaim_sm(&squarewave_program, pio[0], sm[0], offset[0]);
    pio_remove_program_and_unclaim_sm(&squarewave_program, pio[1], sm[1], offset[1]);
    pio_remove_program_and_unclaim_sm(&squarewave_program, pio[2], sm[2], offset[2]);

    return 0;

}
