/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/dma.h"
#include "uart_rx.pio.h"
#include "uart_tx.pio.h"

// Sends data via GPIO 4 and receives it on GPIO 5
// Connect these pins with a wire
#define GPIO_TX 4
#define GPIO_RX 5

#define SERIAL_BAUD 921600
#define HARD_UART_INST uart1

#define USE_PIO_FOR_RX 1
#define USE_DMA_FOR_RX 1
#define USE_PIO_FOR_TX 1
#define USE_DMA_FOR_TX 1

#ifndef DMA_IRQ_PRIORITY
#define DMA_IRQ_PRIORITY PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY
#endif

#ifndef PIO_IRQ_PRIORITY
#define PIO_IRQ_PRIORITY PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY
#endif

#define PIO_IRQ_NUM 0
#define DMA_IRQ_NUM 0

#define DMA_IRQX(N) DMA_IRQ_ ## N
#define DMA_IRQN(N) DMA_IRQX(N)

// dma channels
static uint dma_channel_rx;
static uint dma_channel_tx;

// pio hardware
#if USE_PIO_FOR_RX
static PIO pio_hw_rx;
static uint pio_sm_rx;
static uint offset_rx;
#endif

#if USE_PIO_FOR_TX
static PIO pio_hw_tx;
static uint pio_sm_tx;
static uint offset_tx;
#endif

// read request size for progress
static uint32_t read_size;

// PIO interrupt handler, called when the state machine fifo is not empty
// note: shouldn't printf in an irq normally!
static void pio_irq_handler() {
    dma_channel_hw_t *dma_chan = dma_channel_hw_addr(dma_channel_rx);
    printf("pio_rx dma_rx=%u/%u\n", read_size - dma_chan->transfer_count, read_size);
}

// DMA interrupt handler, called when a DMA channel has transmitted its data
static void dma_irq_handler() {
    if (dma_channel_tx >= 0 && dma_irqn_get_channel_status(DMA_IRQ_NUM, dma_channel_tx)) {
        dma_irqn_acknowledge_channel(DMA_IRQ_NUM, dma_channel_tx);
        printf("dma_tx done\n");
    }
    if (dma_channel_rx >= 0 && dma_irqn_get_channel_status(DMA_IRQ_NUM, dma_channel_rx)) {
        dma_irqn_acknowledge_channel(DMA_IRQ_NUM, dma_channel_rx);
        printf("dma_rx done\n");
    }
}

// Return a pointer to pio hardware
static PIO pio_hardware(int n) {
    static_assert(NUM_PIOS == 2, "");
    const PIO pios[] = {pio0, pio1};
    return pios[n];
}

// Return irqn for the pio
static int pio_irqn(PIO pio_hw, int irqn) {
    assert(irqn < (PIO1_IRQ_0 - PIO0_IRQ_0));
    return PIO0_IRQ_0 + (PIO1_IRQ_0 - PIO0_IRQ_0) * pio_get_index(pio_hw) + irqn;
}

// Allocates a pio and statemachine and loads the pio program into memory
// Return true on success
static bool pio_claim_free_sm_add_program(const pio_program_t *program, PIO *pio_hw, uint *sm, uint *offset) {
    // Find a free pio
    int count = NUM_PIOS;
    while(count--) {
        *pio_hw = pio_hardware(count);
        if (!pio_can_add_program(*pio_hw, program)) {
            continue;
        }
        // Find a state machine
        *sm = (int8_t)pio_claim_unused_sm(*pio_hw, false);
        if (*sm < 0) {
            continue;
        }
        *offset = pio_add_program(*pio_hw, program);
        return true;
    }
    return false;
}

// free up pio resources
static void pio_remove_program_unclaim_sm(const pio_program_t *program, PIO pio_hw, uint sm, uint offset) {
    pio_remove_program(pio_hw, program, offset);
    pio_sm_unclaim(pio_hw, sm);
}

static void dump_bytes(const char *bptr, uint32_t len) {
    unsigned int i = 0;
    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            printf("\n");
        } else if ((i & 0x07) == 0) {
            printf(" ");
        }
        printf("%02x ", bptr[i++]);
    }
    printf("\n");
}

int main()
{
    setup_default_uart();

    // setup text we are going to send and what we expect to get back
    const char buffer_tx[] = "the quick brown fox jumps over the lazy dog";

    // Buffer for receiving data
    char buffer_rx[sizeof(buffer_tx) - 1] = {0};

#if !USE_PIO_FOR_RX || !USE_PIO_FOR_TX
    uart_init(HARD_UART_INST, SERIAL_BAUD);
#endif

// setup pio for rx
#if USE_PIO_FOR_RX
    if (!pio_claim_free_sm_add_program(&uart_rx_mini_program, &pio_hw_rx, &pio_sm_rx, &offset_rx)) {
        panic("failed to allocate pio for rx");
    }
    uart_rx_mini_program_init(pio_hw_rx, pio_sm_rx, offset_rx, GPIO_RX, SERIAL_BAUD);
#else
    gpio_set_function(GPIO_RX, GPIO_FUNC_UART);
#endif

// setup pio for tx
#if USE_PIO_FOR_TX
    if (!pio_claim_free_sm_add_program(&uart_tx_program, &pio_hw_tx, &pio_sm_tx, &offset_tx)) {
        panic("failed to allocate pio for tx");
    }
    uart_tx_program_init(pio_hw_tx, pio_sm_tx, offset_tx, GPIO_TX, SERIAL_BAUD);
#else
    gpio_set_function(GPIO_TX, GPIO_FUNC_UART);
#endif

// setup pio interrupt
#if USE_PIO_FOR_RX
    if (irq_get_exclusive_handler(pio_irqn(pio_hw_rx, PIO_IRQ_NUM))) {
        panic("PIO IRQ in use");
    }
#if USE_DMA_FOR_RX
    irq_add_shared_handler(pio_irqn(pio_hw_rx, PIO_IRQ_NUM), pio_irq_handler, PIO_IRQ_PRIORITY);
    pio_set_irqn_source_enabled(pio_hw_rx, PIO_IRQ_NUM, pis_sm0_rx_fifo_not_empty + pio_sm_rx, true);
    irq_set_enabled(pio_irqn(pio_hw_rx, PIO_IRQ_NUM), true);
#endif
#endif

// add dma handler
#if USE_DMA_FOR_RX || USE_DMA_FOR_TX
    if (irq_get_exclusive_handler(DMA_IRQN(DMA_IRQ_NUM))) {
        panic("DMA IRQ in use");
    }
    irq_add_shared_handler(DMA_IRQN(DMA_IRQ_NUM), dma_irq_handler, DMA_IRQ_PRIORITY);
    irq_set_enabled(DMA_IRQN(DMA_IRQ_NUM), true);
#endif

// Setup dma for read
#if USE_DMA_FOR_RX
    dma_channel_rx = dma_claim_unused_channel(false);
    if (dma_channel_rx < 0) {
        panic("No free dma channels");
    }
    dma_channel_config config_rx = dma_channel_get_default_config(dma_channel_rx);
    channel_config_set_transfer_data_size(&config_rx, DMA_SIZE_8);
    channel_config_set_read_increment(&config_rx, false);
    channel_config_set_write_increment(&config_rx, true);
    read_size = sizeof(buffer_tx) - 1;
    // enable irq for rx
    dma_irqn_set_channel_enabled(DMA_IRQ_NUM, dma_channel_rx, true);
#if USE_PIO_FOR_RX
    // read from pio fifo
    channel_config_set_dreq(&config_rx, pio_get_dreq(pio_hw_rx, pio_sm_rx, false));
    // 8-bit read from the uppermost byte of the FIFO, as data is left-justified so need to add 3. Don't forget the cast!
    dma_channel_configure(dma_channel_rx, &config_rx, buffer_rx, (io_rw_8*)&pio_hw_rx->rxf[pio_sm_rx] + 3, read_size, true); // dma started
#else
    // read from uart hardware
    channel_config_set_dreq(&config_rx, uart_get_dreq(HARD_UART_INST, false));
    dma_channel_configure(dma_channel_rx, &config_rx, buffer_rx, &uart_get_hw(HARD_UART_INST)->dr, read_size, true); // dma started
#endif
#endif

// setup dma for write
#if USE_DMA_FOR_TX
    dma_channel_tx = dma_claim_unused_channel(false);
    if (dma_channel_tx < 0) {
        panic("No free dma channels");
    }
    dma_channel_config config_tx = dma_channel_get_default_config(dma_channel_tx);
    channel_config_set_transfer_data_size(&config_tx, DMA_SIZE_8);
    channel_config_set_read_increment(&config_tx, true);
    channel_config_set_write_increment(&config_tx, false);
    // enable irq for tx
    dma_irqn_set_channel_enabled(DMA_IRQ_NUM, dma_channel_tx, true);
#if USE_PIO_FOR_RX
    // write to pio fifo
    channel_config_set_dreq(&config_tx, pio_get_dreq(pio_hw_tx, pio_sm_tx, true));
    dma_channel_configure(dma_channel_tx, &config_tx, &pio_hw_rx->txf[pio_sm_tx], buffer_tx, sizeof(buffer_tx) - 1, true); // dma started
#else
    // write to uart hardware
    channel_config_set_dreq(&config_tx, uart_get_dreq(HARD_UART_INST, true));
    dma_channel_configure(dma_channel_tx, &config_tx, &uart_get_hw(HARD_UART_INST)->dr, buffer_tx, sizeof(buffer_tx) - 1, true); // dma started
#endif
#endif

// send data
#if USE_DMA_FOR_TX
    dma_channel_wait_for_finish_blocking(dma_channel_tx); // wait for tx
#elif USE_PIO_FOR_TX
    // write to the pio fifo
    int count_pio_tx = 0;
    while(count_pio_tx < sizeof(buffer_tx) - 1) {
        uart_tx_program_putc(pio_hw_tx, pio_sm_tx, buffer_tx[count_pio_tx++]);
    }
#else
    uart_puts(HARD_UART_INST, buffer_tx);
#endif

// Receive the data
#if USE_DMA_FOR_RX
    // wait for dma rx
    dma_channel_wait_for_finish_blocking(dma_channel_rx);
#elif USE_PIO_FOR_RX
    // read from the pio fifo
    int count_pio_rx = 0;
    while(count_pio_rx < sizeof(buffer_tx) - 1) {
        buffer_rx[count_pio_rx++] = uart_rx_program_getc(pio_hw_rx, pio_sm_rx);
    }
#else
    // use the uart hardware
    int count_uart_rx = 0;
    while(count_uart_rx < sizeof(buffer_tx) - 1) {
        buffer_rx[count_uart_rx++] = uart_getc(HARD_UART_INST);
    }
#endif

    // check
    if (memcmp(buffer_rx, buffer_tx, sizeof(buffer_tx) - 1) == 0) {
        printf("Test passed\n");
    } else {
        printf("buffer_tx: >%s<\n", buffer_tx);
        dump_bytes(buffer_tx, sizeof(buffer_tx));
        printf("result: >%s<\n", buffer_rx);
        dump_bytes(buffer_rx, sizeof(buffer_rx));
        printf("Test failed\n");
        assert(0);
    }

    // cleanup
#if USE_DMA_FOR_TX
    dma_irqn_set_channel_enabled(DMA_IRQ_NUM, dma_channel_tx, false);
    dma_channel_unclaim(dma_channel_tx);
#endif
#if USE_DMA_FOR_RX
    dma_irqn_set_channel_enabled(DMA_IRQ_NUM, dma_channel_rx, false);
    dma_channel_unclaim(dma_channel_rx);
#endif
#if USE_DMA_FOR_RX || USE_DMA_FOR_TX
    irq_remove_handler(DMA_IRQN(DMA_IRQ_NUM), dma_irq_handler);
    if (!irq_has_shared_handler(DMA_IRQN(DMA_IRQ_NUM))) {
        irq_set_enabled(DMA_IRQN(DMA_IRQ_NUM), false);
    }
#endif
#if USE_PIO_FOR_RX
    pio_set_irqn_source_enabled(pio_hw_rx, PIO_IRQ_NUM, pis_sm0_rx_fifo_not_empty + pio_sm_rx, false);
    irq_remove_handler(pio_irqn(pio_hw_rx, PIO_IRQ_NUM), pio_irq_handler);
    if (!irq_has_shared_handler(pio_irqn(pio_hw_rx, PIO_IRQ_NUM))) {
        irq_set_enabled(pio_irqn(pio_hw_rx, PIO_IRQ_NUM), false);
    }
    pio_remove_program_unclaim_sm(&uart_rx_mini_program, pio_hw_rx, pio_sm_rx, offset_rx);
#endif
#if USE_PIO_FOR_TX
    pio_remove_program_unclaim_sm(&uart_tx_program, pio_hw_tx, pio_sm_tx, offset_tx);
#endif
#if !USE_PIO_FOR_RX || !USE_PIO_FOR_TX
    uart_deinit(HARD_UART_INST);
#endif
    return 0;
}
