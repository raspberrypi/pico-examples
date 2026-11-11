/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"

volatile bool seen_resus;

void resus_callback(void) {
    // Restart PLL sys and run clk_sys from it at the default system clock
    // frequency (125MHz on RP2040, 150MHz on RP2350)
    set_sys_clock_khz(SYS_CLK_KHZ, true);

    // Reconfigure uart as clocks have changed
    stdio_init_all();
    printf("Resus event fired\n");

    // Wait for uart output to finish
    uart_default_tx_wait_blocking();

    seen_resus = true;
}

int main() {
    stdio_init_all();
    printf("Hello resus\n");

    seen_resus = false;

    clocks_enable_resus(&resus_callback);
    // Break PLL sys
    pll_deinit(pll_sys);

    while(!seen_resus);

    return 0;
}
