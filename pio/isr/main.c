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

#include "simple_isr.pio.h"

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
 * I'm going to copy the state machine that fired the ISR into
 * this variable using the above defines.
 *
 * Variable is volatile so that it doesn't get cached in a CPU register
 * in the main thread. Without this it's possible that you never see
 * irq_flags get set even though the ISR is firing all the time.
 *
 * Of course, you can really do whatever you want in the ISR, it's up to you.
 */
volatile uint32_t irq_flags = 0;

/**
 * This function is called when the IRQ is fired by the state machine.
 * @note See enable_pio_isrs for how to register this function to be called
 */
void simple_isr_handler() {
    // Check the interrupt source and set the flag accordingly
    // Using if for each one means that if multiple interrupts came in, this
    // ISR would flag each one. That means the ISR would execute once instead
    // of being executed for each interrupt individually.
    // Another alternative intead of using pio_interrupt_get would be to
    // read the PIO0_IRQ register directly to get these flags
    for (uint i = 0; i < 4; i++) {
        // Check if the interrupt is set, flags 0-3 are used for system interrupts
        // These are the ones that will be set by the isr instruction in pio
        bool isr_set = pio_interrupt_get(pio0, i);

        if (isr_set) {
            // Set a bit flagging which state machine set the interrupt
            irq_flags |= (1 << i);

            // Clear/Acknowledge the ISR. If you use "isr wait" this is what
            // tells the sm to continue.
            pio_interrupt_clear(pio0, i);
        }
    }
}

/**
 * Lets the pico know that we want it to notify us of the PIO ISRs.
 * @note in simple_isr.pio we enable irq0. This tells the state machine
 *       to send the ISRs to the core, we still need to tell the core
 *       to send them to our program.
 */
void enable_pio_isrs() {
    // Set the function that will be called when the PIO IRQ comes in.
    irq_set_exclusive_handler(PIO0_IRQ_0, simple_isr_handler);

    // Once that function is set, we can go ahead and allow the interrupts
    // to come in. You want to set the function before enabling the interrupt
    // just in case. The docs say if an IRQ comes in and there's no handler
    // then it will work like a breakpoint, which seems bad.
    irq_set_enabled(PIO0_IRQ_0, true);
}

/**
 * Loads simple_isr pio program into PIO memory
 */
void load_pio_programs() {
    PIO pio = pio0;

    // Load the program into PIO memory
    uint offset = pio_add_program(pio, &simple_isr_program);

    // Load the program to run in each state machine.
    // They are allowed to run the same program in memory.
    simple_isr_program_init(pio, 0, offset);
    simple_isr_program_init(pio, 1, offset);
    simple_isr_program_init(pio, 2, offset);
    simple_isr_program_init(pio, 3, offset);
}

/**
 * Writes to the tx fifo of the given state machine.
 * This will make the simple_isr program send an ISR to us!
 */
void trigger_isr(int sm) {
    printf("Triggering ISR from state machine %d\n", sm);
    pio_sm_put_blocking(pio0, sm, 1);
    // ISR will fire from the pio right here thanks to above function.

    // Print the irq we expect based on the given state machine
    printf("Expected IRQ flags: 0x%08X\n", (1 << sm));
    printf("Actual IRQ Flags: 0x%08X\n", irq_flags);

    // Here you could do work for the isr depending on which one it is.
    // Something like
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

    // Load simple_isr into memory
    load_pio_programs();

    // Enable IRQs to respond to simple_isr
    enable_pio_isrs();

    // simple_isr is programmed to fire an ISR when we write
    // to their tx fifo. So let's do that now.
    while (true) {
        // Fire state machine 0
        trigger_isr(0);
        sleep_ms(1000);

        // Fire state machine 1
        trigger_isr(1);
        sleep_ms(1000);

        // Fire state machine 2
        trigger_isr(2);
        sleep_ms(1000);

        // Fire state machine 3
        trigger_isr(3);
        sleep_ms(1000);
    }
}

