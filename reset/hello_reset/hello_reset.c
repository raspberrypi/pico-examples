/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/resets.h"

/// \tag::hello_reset[]
int main() {
    stdio_init_all();

    printf("Hello, reset!\n");

    // Put the PWM block into reset
    reset_block(RESETS_RESET_PWM_BITS);

    // And bring it out
    unreset_block_wait(RESETS_RESET_PWM_BITS);

    // Put the PWM and RTC block into reset
    reset_block(RESETS_RESET_PWM_BITS | RESETS_RESET_RTC_BITS);

    // Wait for both to come out of reset
    unreset_block_wait(RESETS_RESET_PWM_BITS | RESETS_RESET_RTC_BITS);

    return 0;
}
/// \end::hello_reset[]
