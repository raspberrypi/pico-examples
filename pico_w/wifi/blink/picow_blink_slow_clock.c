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
    // Slow the sys clock down and speed up the cyw43 pio to compensate
    // By default the pio used to communicate with cyw43 runs with a clock divisor of 2
    // if you modify the clock you will have to compensate for this
    // As an alternative you could specify CYW43_PIO_CLOCK_DIV_INT=x CYW43_PIO_CLOCK_DIV_FRAC=y in your cmake file
    // To call cyw43_set_pio_clock_divisor you have to add CYW43_PIO_CLOCK_DIV_DYNAMIC=1 to your cmake file
    set_sys_clock_khz(18000, true);
    cyw43_set_pio_clock_divisor(1, 0);

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
