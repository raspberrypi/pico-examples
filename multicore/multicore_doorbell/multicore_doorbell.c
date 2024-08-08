/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#define ITERATIONS 100000

static int doorbell_counter;
static int doorbell_exit;
static uint32_t doorbell_irq_count;
static bool exit_core0;

// Handle the doorbell set from core0
void core1_doorbell_irq() {
    // Increment counter
    if (multicore_doorbell_is_set_current_core(doorbell_counter)) {
        doorbell_irq_count++;
        multicore_doorbell_clear_current_core(doorbell_counter);
    }
    // Are we finished?
    if (multicore_doorbell_is_set_current_core(doorbell_exit)) {
        multicore_doorbell_clear_current_core(doorbell_exit);
        exit_core0 = true;
    }
}

// code for core1
void core1_entry() {
    // Setup an interrupt handler for doorbells - there's actually only one for them all
    uint32_t irq = multicore_doorbell_irq_num(doorbell_counter);
    irq_set_exclusive_handler(irq, core1_doorbell_irq);
    irq_set_enabled(irq, true);

    // Keep going until the exit doorbell is set
    while (!exit_core0) {
        tight_loop_contents();
    }

    printf("core1 is exiting\n");
    multicore_doorbell_set_other_core(doorbell_exit);
}

int main() {
    stdio_init_all();

    // claim doorbells
    doorbell_counter = multicore_doorbell_claim_unused((1 << NUM_CORES) - 1, true);
    doorbell_exit = multicore_doorbell_claim_unused((1 << NUM_CORES) - 1, true);

    // Make sure the exit doorbell is not set for this core
    multicore_doorbell_clear_current_core(doorbell_exit);

    // start core1
    multicore_launch_core1(core1_entry);

    // set doorbell_counter as quickly as possible in a loop, we'll see how many times the other core irq sees it
    int count = ITERATIONS;
    while (count--) {
        multicore_doorbell_set_other_core(doorbell_counter);
    }

    // tell core 1 to exit
    multicore_doorbell_set_other_core(doorbell_exit);

    // wait for it to tell us it's done
    while(!multicore_doorbell_is_set_current_core(doorbell_exit)) {
        tight_loop_contents();
    }
    stdio_flush();

    // display results
    printf("Final doorbell irq count %u/%d (%d%%)\n", doorbell_irq_count, ITERATIONS, doorbell_irq_count * 100 / ITERATIONS);
    return 0;
}
