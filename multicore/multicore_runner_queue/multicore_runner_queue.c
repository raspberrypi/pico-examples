/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"

#define FLAG_VALUE 123

typedef struct
{
    int32_t (*func)(int32_t);
    int32_t data;
} queue_entry_t;

queue_t call_queue;
queue_t results_queue;

void core1_entry() {
    while (1) {
        // Function pointer is passed to us via the queue_entry_t which also
        // contains the function parameter.
        // We provide an int32_t return value by simply pushing it back on the
        // return queue which also indicates the result is ready.

        queue_entry_t entry;

        queue_remove_blocking(&call_queue, &entry);

        int32_t result = entry.func(entry.data);

        queue_add_blocking(&results_queue, &result);
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
    int32_t res;

    stdio_init_all();
    printf("Hello, multicore_runner using queues!\n");

    // This example dispatches arbitrary functions to run on the second core
    // To do this we run a dispatcher on the second core that accepts a function
    // pointer and runs it. The data is passed over using the queue library from
    // pico_utils

    queue_init(&call_queue, sizeof(queue_entry_t), 2);
    queue_init(&results_queue, sizeof(int32_t), 2);

    multicore_launch_core1(core1_entry);

    queue_entry_t entry = {factorial, TEST_NUM};
    queue_add_blocking(&call_queue, &entry);

    // We could now do a load of stuff on core 0 and get our result later

    queue_remove_blocking(&results_queue, &res);

    printf("Factorial %d is %d\n", TEST_NUM, res);

    // Now try a different function
    entry.func = fibonacci;
    queue_add_blocking(&call_queue, &entry);

    queue_remove_blocking(&results_queue, &res);

    printf("Fibonacci %d is %d\n", TEST_NUM, res);
}
