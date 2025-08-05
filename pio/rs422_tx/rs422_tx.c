/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "rs422_tx.pio.h"

// Defines the GPIO pins for PIO communication.
// PIO_TXP_PIN is the positive pin.
// PIO_TXM_PIN is the negative pin and must be the next pin in sequence.
#define PIO_TXP_PIN 0
#define PIO_TXM_PIN 1

// Check the pin is compatible with the platform
#if (PIO_TXM_PIN) != (PIO_TXP_PIN + 1)
#error "PIO_TXM_PIN must be the next pin after PIO_TXP_PIN."
#endif
#if (PIO_TXM_PIN) >= NUM_BANK0_GPIOS
#error "Pin number exceeds available GPIOs (30 on RP2040/RP2350A, 48 on RP2350B)"
#endif
#if NUM_BANK0_GPIOS > 30
// On RP2350B, ensure both pins are in the same PIO range
#if !((PIO_TXP_PIN >= 0 && PIO_TXM_PIN < 32) || (PIO_TXP_PIN >= 16 && PIO_TXM_PIN < 48))
#error "On RP2350B, both pins must be in the same PIO range: [0-31] or [16-47]"
#endif
#endif

int main() {
    // This is the same as the default UART baud rate on Pico
    const uint SERIAL_BAUD = 115200;

    PIO pio;
    uint sm;
    uint offset;

    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&rs422_tx_program, &pio, &sm, &offset, PIO_TXP_PIN, 2, true);
    hard_assert(success);

    rs422_tx_program_init(pio, sm, offset, PIO_TXP_PIN, SERIAL_BAUD);

    while (true) {
        rs422_tx_program_puts(pio, sm, "Hello, world! (from PIO!)\r\n");
        sleep_ms(1000);
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&rs422_tx_program, pio, sm, offset);
}
