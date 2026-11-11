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
    // Reconfigure PLL sys back to the default state
    pll_init(pll_sys, PLL_SYS_REFDIV, PLL_SYS_VCO_FREQ_HZ, PLL_SYS_POSTDIV1, PLL_SYS_POSTDIV2);

    // CLK SYS = PLL SYS
    clock_configure_undivided(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    SYS_CLK_HZ);

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
