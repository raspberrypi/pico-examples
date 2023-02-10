/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps/lwiperf.h"

#ifndef USE_LED
#define USE_LED 1
#endif

#if CLIENT_TEST && !defined(IPERF_SERVER_IP)
#error IPERF_SERVER_IP not defined
#endif

// Report IP results and exit
static void iperf_report(void *arg, enum lwiperf_report_type report_type,
                         const ip_addr_t *local_addr, u16_t local_port, const ip_addr_t *remote_addr, u16_t remote_port,
                         u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec) {
    static uint32_t total_iperf_megabytes = 0;
    uint32_t mbytes = bytes_transferred / 1024 / 1024;
    float mbits = bandwidth_kbitpsec / 1000.0;

    total_iperf_megabytes += mbytes;

    printf("Completed iperf transfer of %d MBytes @ %.1f Mbits/sec\n", mbytes, mbits);
    printf("Total iperf megabytes since start %d Mbytes\n", total_iperf_megabytes);
#if CYW43_USE_STATS
    printf("packets in %u packets out %u\n", CYW43_STAT_GET(PACKET_IN_COUNT), CYW43_STAT_GET(PACKET_OUT_COUNT));
#endif
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
    }

    cyw43_arch_lwip_begin();
#if CLIENT_TEST
    printf("\nReady, running iperf client\n");
    ip_addr_t clientaddr;
    ip4_addr_set_u32(&clientaddr, ipaddr_addr(xstr(IPERF_SERVER_IP)));
    assert(lwiperf_start_tcp_client_default(&clientaddr, &iperf_report, NULL) != NULL);
#else
    printf("\nReady, running iperf server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    lwiperf_start_tcp_server_default(&iperf_report, NULL);
#endif
    cyw43_arch_lwip_end();

    while(true) {
#if USE_LED
        static absolute_time_t led_time;
        static int led_on = true;

        // Invert the led
        if (absolute_time_diff_us(get_absolute_time(), led_time) < 0) {
            led_on = !led_on;
            cyw43_gpio_set(&cyw43_state, 0, led_on);
            led_time = make_timeout_time_ms(1000);

            // Check we can read back the led value
            bool actual_led_val = !led_on;
            cyw43_gpio_get(&cyw43_state, 0, &actual_led_val);
            assert(led_on == actual_led_val);
        }
#endif
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer interrupt) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(led_time);
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }

    cyw43_arch_deinit();
    return 0;
}
