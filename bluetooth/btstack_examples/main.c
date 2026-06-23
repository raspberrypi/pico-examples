/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

#if USING_IPERF
#include "lwip/apps/lwiperf.h"
#endif

// This is implemented isn pico-sdk/lib/btstack/example/<example name>.c
int btstack_main(int argc, const char * argv[]);

#if USING_I2C
// This sets up the audio sink
const btstack_audio_sink_t * btstack_audio_pico_sink_get_instance(void);
#endif

#if USING_IPERF
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
#endif

// This is used in some examples to turn the led on and off
void hal_led_toggle(void){
    static int led_state;
    led_state = 1 - led_state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init() != PICO_OK) {
        panic("failed to cyw43");
    }

#if USING_I2C
    btstack_audio_sink_set_instance(btstack_audio_pico_sink_get_instance());
#endif

    // Setup the example
    btstack_main(0, NULL);

#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
    // Connect to WiFi
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        panic("failed to connect");
    } else {
        printf("Connected.\n");
    }
#endif

#if USING_IPERF
    // Run iperf server
    cyw43_arch_lwip_begin();
    printf("\nReady, running iperf server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    lwiperf_start_tcp_server_default(&iperf_report, NULL);
    cyw43_arch_lwip_end();
#endif

    btstack_run_loop_execute(); // run until btstack_run_loop_trigger_exit is called

#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
    cyw43_arch_disable_sta_mode();
#endif
    cyw43_arch_deinit();
    return 0;
}
