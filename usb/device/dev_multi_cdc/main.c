/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <bsp/board_api.h>
#include <tusb.h>

#include <pico/stdio.h>
#include <pico/stdlib.h>

void custom_cdc_task(void);

int main(void)
{
    // Initialize TinyUSB stack
    board_init();
    tusb_init();

    // TinyUSB board init callback after init
    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    // let pico sdk use the first cdc interface for std io
    stdio_init_all();

    // main run loop
    while (1) {
        // TinyUSB device task | must be called regurlarly
        tud_task();

        // custom tasks
        custom_cdc_task();
    }

    // indicate no error
    return 0;
}

void custom_cdc_task(void)
{
    // polling CDC interfaces if wanted

    // Check if CDC interface 0 (for pico sdk stdio) is connected and ready

    static absolute_time_t next_check = 0;
    absolute_time_t now = get_absolute_time();

    // check every 5 seconds after first connected
    if (tud_cdc_n_connected(0) && (absolute_time_diff_us(next_check, now) >= 0)) {
        // print on CDC 0 some debug message
        printf("Connected to CDC 0\n");
        next_check = delayed_by_ms(now, 5000);
    }
}

// callback when data is received on a CDC interface
void tud_cdc_rx_cb(uint8_t itf)
{
    // allocate buffer for the data in the stack
    uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];

    printf("RX CDC %d\n", itf);

    // read the available data 
    // | IMPORTANT: also do this for CDC0 because otherwise
    // | you won't be able to print anymore to CDC0
    // | next time this function is called
    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

    // check if the data was received on the second cdc interface
    if (itf == 1) {
        // process the received data
        buf[count] = 0; // null-terminate the string
        // now echo data back to the console on CDC 0
        printf("Received on CDC 1: %s\n", buf);

        // and echo back OK on CDC 1
        tud_cdc_n_write(itf, (uint8_t const *) "OK\r\n", 4);
        tud_cdc_n_write_flush(itf);
    }
}
