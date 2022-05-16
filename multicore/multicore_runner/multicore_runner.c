/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

/// \tag::multicore_dispatch[]

#define FLAG_VALUE 123

void core1_entry() {
    while (1) {
        // Function pointer is passed to us via the FIFO
        // We have one incoming int32_t as a parameter, and will provide an
        // int32_t return value by simply pushing it back on the FIFO
        // which also indicates the result is ready.
        int32_t (*func)() = (int32_t(*)()) multicore_fifo_pop_blocking();
        int32_t p = multicore_fifo_pop_blocking();
        int32_t result = (*func)(p);
        multicore_fifo_push_blocking(result);
    }
}

int32_t factorial(int32_t n) {
    int32_t f = 1;
    for (int i = 2; i <= n; i++) {
        f *= i;
    }
    return f;
}

int32_t fibonacci(int32_t n) {
    if (n == 0) return 0;
    if (n == 1) return 1;

    int n1 = 0, n2 = 1, n3 = 0;

    for (int i = 2; i <= n; i++) {
        n3 = n1 + n2;
        n1 = n2;
        n2 = n3;
    }
    return n3;
}

#define TEST_NUM 10

int main() {
    int res;

    stdio_init_all();
    printf("Hello, multicore_runner!\n");

    // This example dispatches arbitrary functions to run on the second core
    // To do this we run a dispatcher on the second core that accepts a function
    // pointer and runs it
    multicore_launch_core1(core1_entry);

    multicore_fifo_push_blocking((uintptr_t) &factorial);
    multicore_fifo_push_blocking(TEST_NUM);

    // We could now do a load of stuff on core 0 and get our result later

    res = multicore_fifo_pop_blocking();

    printf("Factorial %d is %d\n", TEST_NUM, res);

    // Now try a different function
    multicore_fifo_push_blocking((uintptr_t) &fibonacci);
    multicore_fifo_push_blocking(TEST_NUM);

    res = multicore_fifo_pop_blocking();

    printf("Fibonacci %d is %d\n", TEST_NUM, res);


}

/// \end::multicore_dispatch[]
