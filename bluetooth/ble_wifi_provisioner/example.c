/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "lwip/apps/lwiperf.h"
#include "wifi_prov_lib.h"

#ifndef PROV_TIMEOUT_MS
#define PROV_TIMEOUT_MS 120000
#endif

// Report IP results
static void iperf_report(void *arg, enum lwiperf_report_type report_type,
                         const ip_addr_t *local_addr, u16_t local_port, const ip_addr_t *remote_addr, u16_t remote_port,
                         u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec) {
    static uint32_t total_iperf_megabytes = 0;
    uint32_t mbytes = bytes_transferred / 1024 / 1024;
    float mbits = bandwidth_kbitpsec / 1000.0;
    total_iperf_megabytes += mbytes;

    printf("Completed iperf transfer of %d MBytes @ %.1f Mbits/sec\n", mbytes, mbits);
    printf("Total iperf megabytes since start %d Mbytes\n", total_iperf_megabytes);
}

// Note: This is called from an interrupt handler
void key_pressed_func(void *param) {
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    if (key == 'd' || key == 'D') {
        bool *exit = (bool*)param;
        *exit = true;
    }
}

int main(void) {
    stdio_init_all();

    // This is for testing
    printf("Press 'w' in the next 3s to wipe stored ssid and password\n");
    int c = getchar_timeout_us(3000000);
    bool wipe_bonds = false;
    if (c == 'w' || c == 'W') {
        printf("Wiping stored ssid and password\n");
        erase_credentials();

        printf("Press 'w' in the next 3s to wipe bonds\n");
        c = getchar_timeout_us(3000000);
        if (c == 'w' || c == 'W') {
            printf("Wiping bonds\n");
            wipe_bonds = true;
        }
    }

    // initialize CYW43 driver architecture (will enable BT because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return PICO_ERROR_GENERIC;
    }

    // if unable to connect with saved ssid and password, waits 120 seconds
    // for new credentials to be provisioned over BLE
    int rc = start_ble_wifi_provisioning(PROV_TIMEOUT_MS, wipe_bonds);
    printf("finished provisioning result=%d\n", rc);
    if (rc != PICO_OK) {
        panic("Wifi provisioning failed");
    }

    // Run iperf server
    cyw43_arch_lwip_begin();
    printf("\nReady, running iperf server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    lwiperf_start_tcp_server_default(&iperf_report, NULL);
    cyw43_arch_lwip_end();

    bool exit = false;
    stdio_set_chars_available_callback(key_pressed_func, &exit);

    // Run forever
    printf("Press 'd' to disconnect and reboot\n");
    while(!exit) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(at_the_end_of_time);
    }

    // Switch off wifi nicely
    cyw43_arch_disable_sta_mode();

    printf("Rebooting example...\n");
    watchdog_enable(500, true);
    sleep_ms(1000);
    return 0;
}
