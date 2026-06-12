/**
 *  Copyright (c) 2026 mjcross
 * 
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/async_context_threadsafe_background.h"
#include "pico/status_led.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

// define a user data type to be passed to our worker's callback function
typedef struct {
    bool led_state;
} my_data_t;


// define a callback function for our worker (MUST be safe to call from an IRQ)
void worker_cb(async_context_t *p_ctx, async_at_time_worker_t *p_worker) {
    // unpack our user data instance from `(void *)worker.user_data`
    my_data_t *p_my_data = p_worker->user_data;

    // update the status LED from our user data and toggle it
    status_led_set_state(p_my_data->led_state);
    p_my_data->led_state = !p_my_data->led_state;

    // re-schedule the worker, so as to flash the LED continuously (an at_time worker is 
    // automatically removed from the context just before it runs)
    async_context_add_at_time_worker_in_ms(p_ctx, p_worker, LED_DELAY_MS);
}

// create an instance of our user data type (MUST still exist when the worker runs)
static my_data_t my_data = {
    .led_state = true 
};

// create an asynchronous at-time worker that points to our callback function and user
// data (MUST still exist when the worker runs)
static async_at_time_worker_t worker = {
    .do_work = &worker_cb,
    .user_data = &my_data
};


int main() {
    // initialise the status LED
    hard_assert(
        status_led_init() == true
    );

    // create and initialise an asynchronous background context with default settings
    // note: in a networking application we'd probably reuse the existing cyw43_arch
    // context directly with `async_context_t ctx = cyw43_arch_async_context()`
    async_context_threadsafe_background_t background;
    hard_assert(
        async_context_threadsafe_background_init_with_defaults(&background) == true
    );

    // add our at-time worker to the background context, to run after LED_DELAY_MS
    // note: `background.core` is the async_context_t of our threadsafe background. If
    // we were using the cyw43 context instead (see above) we'd just put `&ctx` here.
    hard_assert(
        async_context_add_at_time_worker_in_ms(&background.core, &worker, LED_DELAY_MS) == true
    );

    // the LED will flash in the background 
    while(true) {
        sleep_ms(5000);
    }
}
