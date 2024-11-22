/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "pico/async_context_threadsafe_background.h"

#include "hardware/pio.h"
#include "hardware/uart.h"
#include "uart_rx.pio.h"

// This program
// - Uses UART1 (the spare UART, by default) to transmit some text
// - Uses a PIO state machine to receive that text
// - Use an interrupt to determine when the PIO FIFO has some data
// - Saves characters in a queue
// - Uses an async context to perform work when notified by the irq
// - Prints out the received text to the default console (UART0)
// This might require some reconfiguration on boards where UART1 is the
// default UART.

#define SERIAL_BAUD PICO_DEFAULT_UART_BAUD_RATE
#define HARD_UART_INST uart1

// You'll need a wire from GPIO4 -> GPIO3
#define HARD_UART_TX_PIN 4
#define PIO_RX_PIN 3
#define FIFO_SIZE 64
#define MAX_COUNTER 10

// Check the pin is compatible with the platform
#if PIO_RX_PIN >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

static PIO pio;
static uint sm;
static int8_t pio_irq;
static queue_t fifo;
static uint offset;
static uint32_t counter;
static bool work_done;

// Ask core 1 to print a string, to make things easier on core 0
static void core1_main() {
    while(counter < MAX_COUNTER) {
        sleep_ms(1000 + (rand() % 1000));
        static char text[64];
        sprintf(text, "Hello, world from PIO with interrupts! %u\n", counter++);
        uart_puts(HARD_UART_INST, text);
    }
}

static void async_worker_func(async_context_t *async_context, async_when_pending_worker_t *worker);

// An async context is notified by the irq to "do some work"
static async_context_threadsafe_background_t async_context;
static async_when_pending_worker_t worker = { .do_work = async_worker_func };

// IRQ called when the pio fifo is not empty, i.e. there are some characters on the uart
// This needs to run as quickly as possible or else you will lose characters (in particular don't printf!)
static void pio_irq_func(void) {
    while(!pio_sm_is_rx_fifo_empty(pio, sm)) {
        char c = uart_rx_program_getc(pio, sm);
        if (!queue_try_add(&fifo, &c)) {
            panic("fifo full");
        }
    }
    // Tell the async worker that there are some characters waiting for us
    async_context_set_work_pending(&async_context.core, &worker);
}

// Process characters
static void async_worker_func(__unused async_context_t *async_context, __unused async_when_pending_worker_t *worker) {
    work_done = true;
    while(!queue_is_empty(&fifo)) {
        char c;
        if (!queue_try_remove(&fifo, &c)) {
            panic("fifo empty");
        }
        putchar(c); // Display character in the console
    }
}

int main() {
    // Console output (also a UART, yes it's confusing)
    setup_default_uart();
    printf("Starting PIO UART RX interrupt example\n");

    // Set up the hard UART we're going to use to print characters
    uart_init(HARD_UART_INST, SERIAL_BAUD);
    gpio_set_function(HARD_UART_TX_PIN, GPIO_FUNC_UART);

    // create a queue so the irq can save the data somewhere
    queue_init(&fifo, 1, FIFO_SIZE);

    // Setup an async context and worker to perform work when needed
    if (!async_context_threadsafe_background_init_with_defaults(&async_context)) {
        panic("failed to setup context");
    }
    async_context_add_when_pending_worker(&async_context.core, &worker);

    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&uart_rx_program, &pio, &sm, &offset, PIO_RX_PIN, 1, true);
    hard_assert(success);

    uart_rx_program_init(pio, sm, offset, PIO_RX_PIN, SERIAL_BAUD);

    // Find a free irq
    pio_irq = pio_get_irq_num(pio, 0);
    if (irq_get_exclusive_handler(pio_irq)) {
        pio_irq++;
        if (irq_get_exclusive_handler(pio_irq)) {
            panic("All IRQs are in use");
        }
    }

    // Enable interrupt
    irq_add_shared_handler(pio_irq, pio_irq_func, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY); // Add a shared IRQ handler
    irq_set_enabled(pio_irq, true); // Enable the IRQ
    const uint irq_index = pio_irq - pio_get_irq_num(pio, 0); // Get index of the IRQ
    pio_set_irqn_source_enabled(pio, irq_index, pio_get_rx_fifo_not_empty_interrupt_source(sm), true); // Set pio to tell us when the FIFO is NOT empty

    // Tell core 1 to print text to uart1
    multicore_launch_core1(core1_main);

    // Echo characters received from PIO to the console
    while (counter < MAX_COUNTER || work_done) {
        // Note that we could just sleep here as we're using "threadsafe_background" that uses a low priority interrupt
        // But if we changed to use a "polling" context that wouldn't work. The following works for both types of context.
        // When using "threadsafe_background" the poll does nothing. This loop is just preventing main from exiting!
        work_done = false;
        async_context_poll(&async_context.core);
        async_context_wait_for_work_ms(&async_context.core, 2000);
    }

    // Disable interrupt
    pio_set_irqn_source_enabled(pio, irq_index, pio_get_rx_fifo_not_empty_interrupt_source(sm), false);
    irq_set_enabled(pio_irq, false);
    irq_remove_handler(pio_irq, pio_irq_func);

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&uart_rx_program, pio, sm, offset);

    async_context_remove_when_pending_worker(&async_context.core, &worker);
    async_context_deinit(&async_context.core);
    queue_free(&fifo);

    uart_deinit(HARD_UART_INST);

    printf("Test complete\n");
    sleep_ms(100);
    return 0;
}
