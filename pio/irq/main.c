/**
 * Copyright 2022 (c) Daniel Garcia-Briseno
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 *    disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include "pico/stdlib.h"

#include "simple_irq.pio.h"

/**
 * These defines represent each state machine.
 * I'll use these to flag which state machine fired the ISR
 */
#define PIO_SM_0_IRQ 0b0001
#define PIO_SM_1_IRQ 0b0010
#define PIO_SM_2_IRQ 0b0100
#define PIO_SM_3_IRQ 0b1000


/**
 * This variable will shadow the IRQ flags set by the PIO state machines.
 * Typically you do not want to do work in ISRs because the main thread
 * has more important things to do. Because of that, when we get the ISR
 * I'm going to copy the state machine that fired the ISR into this
 * variable.
 *
 * Variable is volatile so that it doesn't get cached in a CPU register
 * in the main thread. Without this it's possible that you never see
 * irq_flags get set even though the ISR is firing all the time.
 *
 * Of course, you can really do whatever you want in the ISR, it's up to you.
 */
volatile uint8_t irq_flags = 0;

/**
 * This function is called when the IRQ is fired by the state machine.
 * @note See enable_pio_irqs for how to register this function to be called
 */
void simple_irq_handler() {
    // Check the interrupt source and set the flag accordingly
    // This method can handle multiple simultaneous interrupts from the PIO
    // since it checks all the irq flags. The advantage to this is that the
    // ISR would execute only once when simultaneous interrupts happen instead
    // of being executed for each interrupt individually.
    // For other applications, make sure to check the correct pio (pio0/pio1)
    irq_flags = pio0_hw->irq;
    // Clear the flags since we've saved them to check later.
    hw_clear_bits(&pio0_hw->irq, irq_flags);

    // alternatively you could use pio_interrupt_get(pio0, #) to check a specific
    // irq flag.
}

/**
 * Lets the pico know that we want it to notify us of the PIO ISRs.
 * @note in simple_irq.pio we enable irq0. This tells the state machine
 *       to send the ISRs to the core, we still need to tell the core
 *       to send them to our program.
 */
void enable_pio_irqs() {
    // Set the function that will be called when the PIO IRQ comes in.
    irq_set_exclusive_handler(PIO0_IRQ_0, simple_irq_handler);

    // Once that function is set, we can go ahead and allow the interrupts
    // to come in. You want to set the function before enabling the interrupt
    // just in case. The docs say if an IRQ comes in and there's no handler
    // then it will work like a breakpoint, which seems bad.
    irq_set_enabled(PIO0_IRQ_0, true);
}

/**
 * Loads simple_irq pio program into PIO memory
 */
void load_pio_programs() {
    PIO pio = pio0;

    // Load the program into PIO memory
    uint offset = pio_add_program(pio, &simple_irq_program);

    // Tell each state machine to run the program.
    // They are allowed to run the same program in memory.
    simple_irq_program_init(pio, 0, offset);
    simple_irq_program_init(pio, 1, offset);
    simple_irq_program_init(pio, 2, offset);
    simple_irq_program_init(pio, 3, offset);
}

/**
 * Writes to the tx fifo of the given state machine.
 * This will make the simple_irq program send an ISR to us!
 */
void trigger_irq(int sm) {
    printf("Triggering ISR from state machine %d\n", sm);
    pio_sm_put_blocking(pio0, sm, 1);
    // Interrupt will fire from the pio right here since they trigger
    // whenever data goes into the TX FIFO, see simple_irq.pio

    // Print the irq we expect based on the given state machine
    printf("Expected IRQ flags: 0x%08X\n", (1 << sm));
    printf("Actual IRQ Flags: 0x%08X\n", irq_flags);

    // Here you could dispatch work for the irq depending one
    // flagged it. The work should be done here rather than inside
    // the irq function. While executing code in an irq, no other
    // irq's can execute. By having the processing code outside of
    // the irq, we can still be responsive to new irqs.
    if (irq_flags & PIO_SM_0_IRQ) {
        // handle_sm0_irq();
    }
    if (irq_flags & PIO_SM_1_IRQ) {
        // handle_sm1_irq();
    }
    if (irq_flags & PIO_SM_2_IRQ) {
        // handle_sm2_irq();
    }
    if (irq_flags & PIO_SM_3_IRQ) {
        // handle_sm3_irq();
    }
    
    // clear irq flags now.
    irq_flags = 0;
}

int main() {
    // Init stdio
    stdio_init_all();

    // Load simple_irq into memory
    load_pio_programs();

    // Enable IRQs to respond to simple_irq
    enable_pio_irqs();

    // simple_irq is programmed to fire an ISR when we write
    // to their tx fifo. So let's do that now.
    while (true) {
        // Fire state machine 0
        trigger_irq(0);
        sleep_ms(1000);

        // Fire state machine 1
        trigger_irq(1);
        sleep_ms(1000);

        // Fire state machine 2
        trigger_irq(2);
        sleep_ms(1000);

        // Fire state machine 3
        trigger_irq(3);
        sleep_ms(1000);
    }
}

