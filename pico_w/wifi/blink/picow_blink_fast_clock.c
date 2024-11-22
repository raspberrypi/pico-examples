/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/cyw43_driver.h"
#include "hardware/clocks.h"

int main() {
    // Increase sys clock. We have slowed down the cyw43 pio to compensate using CYW43_PIO_CLOCK_DIV_INT=4 CYW43_PIO_CLOCK_DIV_FRAC=0 in the cmake file
    // By default the pio used to communicate with cyw43 runs with a clock divisor of 2
    // if you modify the clock you will have to compensate for this
    // As an alternative you could specify CYW43_PIO_CLOCK_DIV_DYNAMIC=1 in your cmake file and call cyw43_set_pio_clock_divisor(4, 0)
    set_sys_clock_khz(266000, true);

    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(250);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);
    }
}
