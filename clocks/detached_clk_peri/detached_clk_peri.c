/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// By default, clk_peri (which drives the serial parts of SPI and UART) is
// attached directly to clk_sys, so varies if the system clock is scaled up
// and down. clk_peri has a multiplexer (though no divider) which allows it to
// be attached to other sources.
//
// If set_sys_clock_khz is called (here setting the system clock to 133 MHz),
// this automatically attaches clk_peri to the USB PLL, which by default we run
// at 48 MHz. As long as you call this *before* configuring your UART etc, the
// UART baud rate will then not be affected by subsequent changes to clk_sys.
//
// However, dropping clk_peri to 48 MHz limits the maximum serial frequencies
// that can be attained by UART and particularly SPI. This example shows how
// clk_peri can be attached directly to the system PLL, and the clk_sys
// divider can then be varied to scale the system (CPUs, DMA, bus fabric etc)
// frequency up and down whilst keeping clk_peri at a constant 133 MHz.
//
// The complete list of clock sources available on clk_peri can be found in
// hardware/regs/clocks.h:
//
// Field       : CLOCKS_CLK_PERI_CTRL_AUXSRC
//               0x0 -> clk_sys
//               0x1 -> clksrc_pll_sys
//               0x2 -> clksrc_pll_usb
//               0x3 -> rosc_clksrc_ph
//               0x4 -> xosc_clksrc
//               0x5 -> clksrc_gpin0
//               0x6 -> clksrc_gpin1

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"

#define PLL_SYS_KHZ (133 * 1000)

int main() {
    // Set the system frequency to 133 MHz. vco_calc.py from the SDK tells us
    // this is exactly attainable at the PLL from a 12 MHz crystal: FBDIV =
    // 133 (so VCO of 1596 MHz), PD1 = 6, PD2 = 2. This function will set the
    // system PLL to 133 MHz and set the clk_sys divisor to 1.
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // The previous line automatically detached clk_peri from clk_sys, and
    // attached it to pll_usb, so that clk_peri won't be disturbed by future
    // changes to system clock or system PLL. If we need higher clk_peri
    // frequencies, we can attach clk_peri directly back to system PLL (no
    // divider available) and then use the clk_sys divider to scale clk_sys
    // independently of clk_peri.
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );

    // The serial clock won't vary from this point onward, so we can configure
    // the UART etc.
    stdio_init_all();

    puts("Peripheral clock is attached directly to system PLL.");
    puts("We can vary the system clock divisor while printing from the UART:");

    for (uint div = 1; div <= 10; ++div) {
        printf("Setting system clock divisor to %u\n", div);
        clock_configure(
            clk_sys,
            CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
            CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
            PLL_SYS_KHZ,
            PLL_SYS_KHZ / div
        );
        printf("Measuring system clock with frequency counter:");
        // Note that the numbering of frequency counter sources is not the
        // same as the numbering of clock slice register blocks. (If we passed
        // the clk_sys enum here we would actually end up measuring XOSC.)
        printf("%u kHz\n", frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS));
    }
    return 0;
}

