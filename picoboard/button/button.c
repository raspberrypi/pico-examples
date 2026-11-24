/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "pico/status_led.h"
#include "hardware/gpio.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

// This example blinks the Pico LED when the BOOTSEL button is pressed.
//
// Picoboard has a button attached to the flash CS pin, which the bootrom
// checks, and jumps straight to the USB bootcode if the button is pressed
// (pulling flash CS low). We can check this pin in by jumping to some code in
// SRAM (so that the XIP interface is not required), floating the flash CS
// pin, and observing whether it is pulled low.
//
// Use flash_safe_execute() so the other core (if running) is also kept off
// flash while CS is floated; disabling interrupts on this core alone is not
// enough (e.g. with an XIP streamer or code on core 1).

static void __no_inline_not_in_flash_func(get_bootsel_button_inner)(void *param) {
    bool *button_state = (bool *)param;
    const uint CS_PIN_INDEX = 1;

    // Set chip select to Hi-Z
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    // Note we can't call into any sleep functions in flash right now
    for (volatile int i = 0; i < 1000; ++i);

    // The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
    // Note the button pulls the pin *low* when pressed.
#if PICO_RP2040
    #define CS_BIT (1u << 1)
#else
    #define CS_BIT SIO_GPIO_HI_IN_QSPI_CSN_BITS
#endif
    *button_state = !(sio_hw->gpio_hi_in & CS_BIT);

    // Need to restore the state of chip select, else we are going to have a
    // bad time when we return to code in flash!
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
}

bool get_bootsel_button() {
    bool button_state = false;
    int rc = flash_safe_execute(get_bootsel_button_inner, &button_state, 100);
    if (rc != PICO_OK) {
        printf("get_bootsel_button: flash_safe_execute failed (%d)\n", rc);
    }
    return button_state;
}

int main() {
#ifndef PICO_STATUS_LED_AVAILABLE
#warning picoboard/button example requires a board with status LED
#else
    int result = status_led_init();
    hard_assert(result);
    result = status_led_supported();
    hard_assert(result);
    while (true) {
        result = status_led_set_state(get_bootsel_button());
        hard_assert(result);
        sleep_ms(10);
    }
#endif
}
