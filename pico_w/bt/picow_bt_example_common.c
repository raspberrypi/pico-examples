/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if TEST_AUDIO
#include "btstack_audio.h"
#endif
#include "btstack_event.h"
#include "hal_led.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "btstack.h"

#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
#define TEST_BTWIFI 1
#endif

#if TEST_BTWIFI
#include "lwip/ip4_addr.h"
#include "lwip/apps/lwiperf.h"
#endif

// Start the btstack example
int btstack_main(int argc, const char * argv[]);

#if TEST_AUDIO
const btstack_audio_sink_t * btstack_audio_pico_sink_get_instance(void);
#endif

static btstack_packet_callback_registration_t hci_event_callback_registration;
static int led_state = 0;

void hal_led_toggle(void){
    led_state = 1 - led_state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;
    switch(hci_event_packet_get_type(packet)){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
            break;
        default:
            break;
    }
}

#if TEST_BTWIFI
static void iperf_report(void *arg, enum lwiperf_report_type report_type,
                         const ip_addr_t *local_addr, u16_t local_port, const ip_addr_t *remote_addr, u16_t remote_port,
                         u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec) {
    static uint32_t total_iperf_megabytes = 0;
    uint32_t mbytes = bytes_transferred / 1024 / 1024;
    float mbits = bandwidth_kbitpsec / 1000.0;
    total_iperf_megabytes += mbytes;
    printf("Completed iperf transfer of %u MBytes @ %.1f Mbits/sec\n", mbytes, mbits);
    printf("Total iperf megabytes since start %u Mbytes\n", total_iperf_megabytes);
}
#endif

int picow_bt_example_init(void) {
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // setup i2s audio for sink
#if TEST_AUDIO
    btstack_audio_sink_set_instance(btstack_audio_pico_sink_get_instance());
#endif
    return 0;
}

void picow_bt_example_main(void) {

    btstack_main(0, NULL);

#if TEST_BTWIFI
    uint32_t start_ms = to_ms_since_boot(get_absolute_time());
    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi \"%s\"...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        panic("failed to connect");
    } else {
        printf("Connected in %lus.\n", (to_ms_since_boot(get_absolute_time()) - start_ms) / 1000);
    }
    printf("\nReady, running iperf server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    lwiperf_start_tcp_server_default(&iperf_report, NULL);
#endif
}
