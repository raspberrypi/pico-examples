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
    // Reconfigure PLL sys back to the default state of 1500 / 6 / 2 = 125MHz
    pll_init(pll_sys, 1, 1500 * MHZ, 6, 2);

    // CLK SYS = PLL SYS (125MHz) / 1 = 125MHz
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    125 * MHZ,
                    125 * MHZ);

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
