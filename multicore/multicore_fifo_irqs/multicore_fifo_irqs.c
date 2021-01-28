/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/irq.h"

/// \tag::multicore_fifo_irqs[]

#define FLAG_VALUE1 123
#define FLAG_VALUE2 321

static int core0_rx_val = 0, core1_rx_val = 0;

void core0_sio_irq() {
    // Just record the latest entry
    while (multicore_fifo_rvalid())
        core0_rx_val = multicore_fifo_pop_blocking();

    multicore_fifo_clear_irq();
}

void core1_sio_irq() {
    // Just record the latest entry
    while (multicore_fifo_rvalid())
        core1_rx_val = multicore_fifo_pop_blocking();

    multicore_fifo_clear_irq();
}

void core1_entry() {
    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, core1_sio_irq);

    irq_set_enabled(SIO_IRQ_PROC1, true);

    // Send something to Core0, this should fire the interrupt.
    multicore_fifo_push_blocking(FLAG_VALUE1);

    while (1)
        tight_loop_contents();
}


int main() {
    stdio_init_all();
    printf("Hello, multicore_fifo_irqs!\n");

    // We MUST start the other core before we enabled FIFO interrupts.
    // This is because the launch uses the FIFO's, enabling interrupts before
    // they are used for the launch will result in unexpected behaviour.
    multicore_launch_core1(core1_entry);

    irq_set_exclusive_handler(SIO_IRQ_PROC0, core0_sio_irq);
    irq_set_enabled(SIO_IRQ_PROC0, true);

    // Wait for a bit for things to happen
    sleep_ms(10);

    // Send something back to the other core
    multicore_fifo_push_blocking(FLAG_VALUE2);

    // Wait for a bit for things to happen
    sleep_ms(10);

    printf("Irq handlers should have rx'd some stuff - core 0 got %d, core 1 got %d!\n", core0_rx_val, core1_rx_val);

    while (1)
        tight_loop_contents();
}

/// \end::multicore_fifo_irqs[]
