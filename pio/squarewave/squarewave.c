/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Output a 12.5 MHz square wave (if system clock frequency is 125 MHz).
//
// Note this program is accessing the PIO registers directly, for illustrative
// purposes. We pull this program into the datasheet so we can talk a little
// about PIO's hardware register interface. The `hardware_pio` SDK library
// provides simpler or better interfaces for all of these operations.
//
// _*This is not best practice! I don't want to see you copy/pasting this*_
//
// For a minimal example of loading and running a program using the SDK
// functions (which is what you generally want to do) have a look at
// `hello_pio` instead. That example is also the subject of a tutorial in the
// SDK book, which walks you through building your first PIO program.

#include "pico/stdlib.h"
#include "hardware/pio.h"

// Our assembled program:
#include "squarewave.pio.h"

int main() {
    // Pick one PIO instance arbitrarily. We're also arbitrarily picking state
    // machine 0 on this PIO instance (the state machines are numbered 0 to 3
    // inclusive).
    PIO pio = pio0;

    /// \tag::load_program[]
    // Load the assembled program directly into the PIO's instruction memory.
    // Each PIO instance has a 32-slot instruction memory, which all 4 state
    // machines can see. The system has write-only access.
    for (int i = 0; i < count_of(squarewave_program_instructions); ++i)
        pio->instr_mem[i] = squarewave_program_instructions[i];
    /// \end::load_program[]

    /// \tag::clock_divider[]
    // Configure state machine 0 to run at sysclk/2.5. The state machines can
    // run as fast as one instruction per clock cycle, but we can scale their
    // speed down uniformly to meet some precise frequency target, e.g. for a
    // UART baud rate. This register has 16 integer divisor bits and 8
    // fractional divisor bits.
    pio->sm[0].clkdiv = (uint32_t) (2.5f * (1 << 16));
    /// \end::clock_divider[]

    /// \tag::setup_pins[]
    // There are five pin mapping groups (out, in, set, side-set, jmp pin)
    // which are used by different instructions or in different circumstances.
    // Here we're just using SET instructions. Configure state machine 0 SETs
    // to affect GPIO 0 only; then configure GPIO0 to be controlled by PIO0,
    // as opposed to e.g. the processors.
    pio->sm[0].pinctrl =
            (1 << PIO_SM0_PINCTRL_SET_COUNT_LSB) |
            (0 << PIO_SM0_PINCTRL_SET_BASE_LSB);
    gpio_set_function(0, GPIO_FUNC_PIO0);
    /// \end::setup_pins[]

    /// \tag::start_sm[]
    // Set the state machine running. The PIO CTRL register is global within a
    // PIO instance, so you can start/stop multiple state machines
    // simultaneously. We're using the register's hardware atomic set alias to
    // make one bit high without doing a read-modify-write on the register.
    hw_set_bits(&pio->ctrl, 1 << (PIO_CTRL_SM_ENABLE_LSB + 0));
    /// \end::start_sm[]

    return 0;

}
