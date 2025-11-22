/**
 *  Copyright (c) 2025 mjcross
 * 
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/async_context_threadsafe_background.h"

// for Pico W devices the on-board LED is controlled via the WiFi module
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

// generic function to initialise the on-board LED
int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);   // non-WiFi boards
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    return cyw43_arch_init();   // WiFi boards
#endif
}

// generic function to turn the on-board LED on or off
void pico_led_set(bool state) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, state); // non-WiFi boards
#elif defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state); // WiFi boards
#endif    
}

// a simple user data structure for our async at-time worker
typedef struct {
    bool state;
} led_state_t;

// callback function for our async at-time worker
// note: this function MUST be safe to call from an IRQ
void worker_cb(async_context_t *p_ctx, async_at_time_worker_t *p_worker) {
    // read user data from worker
    led_state_t *p_led = (led_state_t *)(p_worker->user_data);

    // toggle the LED
    p_led->state = !p_led->state;
    pico_led_set(p_led->state);

    // at-time workers are automatically removed from the context just before they run
    // so to keep the LED flashing we must now re-schedule the task
    async_context_add_at_time_worker_in_ms(p_ctx, p_worker, LED_DELAY_MS);
}


int main() {
    // initialise the LED and our user data structure
    hard_assert(
        pico_led_init() == PICO_OK
    );
    led_state_t led_state = {
        .state = false
    };

    // create and initialise an async background context
    // note: in a networking application we might typically use the context returned by
    // cyw43_arch_async_context() instead of creating a new one here
    async_context_threadsafe_background_t ctx;
    hard_assert(
        async_context_threadsafe_background_init_with_defaults(&ctx) == true
    );

    // define an async at-time worker that will run our callback function
    async_at_time_worker_t worker = {
        .do_work = worker_cb,
        .user_data = &led_state
    };

    // add an at-time worker to the context, scheduled to run after LED_DELAY_MS
    // note: ctx.core is the underlying async_context_t of our threadsafe background
    hard_assert(
        async_context_add_at_time_worker_in_ms(&ctx.core, &worker, LED_DELAY_MS) == true
    );

    // the LED will flash in the background 
    while(true) {
        sleep_ms(5000);
    }
}
