/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/structs/watchdog.h"

// This app shows the effect of byte and halfword writes on IO registers. All
// IO registers on RP2040 will sample the entire 32 bit write data bus on any
// write; the transfer size and the 2 LSBs of the address are *ignored*.
//
// This can have unintuitive results, especially given the way RP2040
// busmasters replicate narrow write data across the entire 32-bit write data
// bus. However, this behaviour can be quite useful if you are aware of it!

int main() {
    stdio_init_all();

    // We'll use WATCHDOG_SCRATCH0 as a convenient 32 bit read/write register
    // that we can assign arbitrary values to
    io_rw_32 *scratch32 = &watchdog_hw->scratch[0];
    // Alias the scratch register as two halfwords at offsets +0x0 and +0x2
    volatile uint16_t *scratch16 = (volatile uint16_t *) scratch32;
    // Alias the scratch register as four bytes at offsets +0x0, +0x1, +0x2, +0x3:
    volatile uint8_t *scratch8 = (volatile uint8_t *) scratch32;

    // Show that we can read/write the scratch register as normal:
    printf("Writing 32 bit value\n");
    *scratch32 = 0xdeadbeef;
    printf("Should be 0xdeadbeef: 0x%08x\n", *scratch32);

    // We can do narrow reads just fine -- IO registers treat this as a 32 bit
    // read, and the processor/DMA will pick out the correct byte lanes based
    // on transfer size and address LSBs
    printf("\nReading back 1 byte at a time\n");
    // Little-endian!
    printf("Should be ef be ad de: %02x ", scratch8[0]);
    printf("%02x ", scratch8[1]);
    printf("%02x ", scratch8[2]);
    printf("%02x\n", scratch8[3]);

    // The Cortex-M0+ and the RP2040 DMA replicate byte writes across the bus,
    // and IO registers will sample the entire write bus always.
    printf("\nWriting 8 bit value 0xa5 at offset 0\n");
    scratch8[0] = 0xa5;
    // Read back the whole scratch register in one go
    printf("Should be 0xa5a5a5a5: 0x%08x\n", *scratch32);

    // The IO register ignores the address LSBs [1:0] as well as the transfer
    // size, so it doesn't matter what byte offset we use
    printf("\nWriting 8 bit value at offset 1\n");
    scratch8[1] = 0x3c;
    printf("Should be 0x3c3c3c3c: 0x%08x\n", *scratch32);

    // Halfword writes are also replicated across the write data bus
    printf("\nWriting 16 bit value at offset 0\n");
    scratch16[0] = 0xf00d;
    printf("Should be 0xf00df00d: 0x%08x\n", *scratch32);
}
