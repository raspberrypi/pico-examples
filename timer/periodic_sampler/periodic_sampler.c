/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"

bool timer_callback(repeating_timer_t *rt);

queue_t sample_fifo;

// using struct as an example, but primitive types can be used too
typedef struct element {
    uint value;
} element_t;

const int FIFO_LENGTH = 32;

int main() {
    stdio_init_all();

    int hz = 25;

    queue_init(&sample_fifo, sizeof(element_t), FIFO_LENGTH);

    repeating_timer_t timer;

    // negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us(-1000000 / hz, timer_callback, NULL, &timer)) {
        printf("Failed to add timer\n");
        return 1;
    }

    // read some blocking

    for (int i = 0; i < 10; i++) {
        element_t element;
        queue_remove_blocking(&sample_fifo, &element);
        printf("Got %d: %d\n", i, element.value);
    }

    // now retrieve all that are available periodically (simulate polling)
    for (int i = 0; i < 10; i++) {
        int count = queue_get_level(&sample_fifo);
        if (count) {
            printf("Getting %d, %d:\n", i, count);
            for (; count > 0; count--) {
                element_t element;
                queue_remove_blocking(&sample_fifo, &element);
                printf("  got %d\n", element.value);
            }
        }
        sleep_us(5000000 / hz); // sleep for 5 times the sampling period
    }

    cancel_repeating_timer(&timer);

    // drain any remaining
    element_t element;
    while (queue_try_remove(&sample_fifo, &element)) {
        printf("Got remaining %d\n", element.value);
    }

    queue_free(&sample_fifo);
    printf("Done\n");
    return 0;
}

bool timer_callback(repeating_timer_t *rt) {
    static int v = 100;
    element_t element = {
            .value = v
    };
    v += 100;

    if (!queue_try_add(&sample_fifo, &element)) {
        printf("FIFO was full\n");
    }
    return true; // keep repeating
}
