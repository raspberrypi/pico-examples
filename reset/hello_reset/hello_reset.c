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
    reset_block_num(RESET_PWM);

    // And bring it out
    unreset_block_num_wait_blocking(RESET_PWM);

    // Put the PWM and ADC block into reset
    reset_block_mask((1u << RESET_PWM) | (1u << RESET_ADC));

    // Wait for both to come out of reset
    unreset_block_mask_wait_blocking((1u << RESET_PWM) | (1u << RESET_ADC));

    return 0;
}
/// \end::hello_reset[]
