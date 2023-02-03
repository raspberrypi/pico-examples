/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/divider.h"

/// \tag::hello_divider[]
int main() {
    stdio_init_all();
    printf("Hello, divider!\n");

    // This is the basic hardware divider function
    int32_t dividend = 123456;
    int32_t divisor = -321;
    divmod_result_t result = hw_divider_divmod_s32(dividend, divisor);

    printf("%d/%d = %d remainder %d\n", dividend, divisor, to_quotient_s32(result), to_remainder_s32(result));

    // Is it right?

    printf("Working backwards! Result %d should equal %d!\n\n",
           to_quotient_s32(result) * divisor + to_remainder_s32(result), dividend);

    // This is the recommended unsigned fast divider for general use.
    int32_t udividend = 123456;
    int32_t udivisor = 321;
    divmod_result_t uresult = hw_divider_divmod_u32(udividend, udivisor);

    printf("%d/%d = %d remainder %d\n", udividend, udivisor, to_quotient_u32(uresult), to_remainder_u32(uresult));

    // Is it right?

    printf("Working backwards! Result %d should equal %d!\n\n",
           to_quotient_u32(result) * divisor + to_remainder_u32(result), dividend);

    // You can also do divides asynchronously. Divides will be complete after 8 cycles.

    hw_divider_divmod_s32_start(dividend, divisor);

    // Do something for 8 cycles!

    // In this example, our results function will wait for completion.
    // Use hw_divider_result_nowait() if you don't want to wait, but are sure you have delayed at least 8 cycles

    result = hw_divider_result_wait();

    printf("Async result %d/%d = %d remainder %d\n", dividend, divisor, to_quotient_s32(result),
           to_remainder_s32(result));

    // For a really fast divide, you can use the inlined versions... the / involves a function call as / always does
    // when using the ARM AEABI, so if you really want the best performance use the inlined versions.
    // Note that the / operator function DOES use the hardware divider by default, although you can change
    // that behavior by calling pico_set_divider_implementation in the cmake build for your target.
    printf("%d / %d = (by operator %d) (inlined %d)\n", dividend, divisor,
           dividend / divisor, hw_divider_s32_quotient_inlined(dividend, divisor));

    // Note however you must manually save/restore the divider state if you call the inlined methods from within an IRQ
    // handler.
    hw_divider_state_t state;
    hw_divider_divmod_s32_start(dividend, divisor);
    hw_divider_save_state(&state);

    hw_divider_divmod_s32_start(123, 7);
    printf("inner %d / %d = %d\n", 123, 7, hw_divider_s32_quotient_wait());

    hw_divider_restore_state(&state);
    int32_t tmp = hw_divider_s32_quotient_wait();
    printf("outer divide %d / %d = %d\n", dividend, divisor, tmp);
    return 0;
}
/// \end::hello_divider[]
