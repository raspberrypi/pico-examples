/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/structs/pads_qspi.h"
#include "hardware/structs/io_qspi.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

// Initialize the GPIO for the LED
void pico_led_init(void) {
#ifdef PICO_DEFAULT_LED_PIN
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

// Turn the LED on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#endif
}

enum qspi_gpio {
    // arbitrarily choose PAD register bank to set the order
    QSPI_GPIO_SCLK = 0,
    QSPI_GPIO_SD0 = 1,
    QSPI_GPIO_SD1 = 2,
    QSPI_GPIO_SD2 = 3,
    QSPI_GPIO_SD3 = 4,
    QSPI_GPIO_SS = 5,
};

// curiously the IO and PAD register banks for the QSPI GPIOs are not in the same order
// This look up table will map the PAD offset to the IO offset for the same pin
const uint QSPI_GPIO_PAD_TO_IO_OFFSET[] = {
    0, // SCLK
    2, // SD0
    3, // SD1
    4, // SD2
    5, // SD3
    1, // SS
};

// Set function for QSPI GPIO pin
void qspi_gpio_set_function(enum qspi_gpio gpio, gpio_function_t fn) {
    // Set input enable on, output disable off
    hw_write_masked(&pads_qspi_hw->io[gpio],
                   PADS_QSPI_GPIO_QSPI_SD2_IE_BITS,
                   PADS_QSPI_GPIO_QSPI_SD2_IE_BITS | PADS_QSPI_GPIO_QSPI_SD2_OD_BITS
    );
    // Zero all fields apart from fsel; we want this IO to do what the peripheral tells it.
    // This doesn't affect e.g. pullup/pulldown, as these are in pad controls.
    io_qspi_hw->io[QSPI_GPIO_PAD_TO_IO_OFFSET[gpio]].ctrl = fn << IO_QSPI_GPIO_QSPI_SD2_CTRL_FUNCSEL_LSB;

    // Remove pad isolation now that the correct peripheral is in control of the pad
    hw_clear_bits(&pads_qspi_hw->io[gpio], PADS_QSPI_GPIO_QSPI_SD2_ISO_BITS);
}

int main() {
    pico_led_init();

    // SD2 is UART0 TX, SD3 is UART0 RX (same as the ROM UART bootloader)
    qspi_gpio_set_function(QSPI_GPIO_SD2, GPIO_FUNC1_UART_AUX);
    qspi_gpio_set_function(QSPI_GPIO_SD3, GPIO_FUNC1_UART_AUX);

    uart_init(uart0, 1000000);

    while (true) {
        uart_puts(uart0, "Hello, world\n");
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
    }
}
