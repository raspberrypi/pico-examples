/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/ip4_addr.h"
#include "lwip/init.h"
#include "lwip/apps/httpd.h" // use lwip's httpd server to simplify example

#include "pico/status_led.h" // libary for simplifying onboard LED usage on Pico W

#include "dhcpserver.h"
#include "dnsserver.h"

#define DEBUG_printf printf

static absolute_time_t wifi_connected_time;
bool led_state;
bool complete;

static const char *switch_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("Switch cgi handler called\n");

    // Get led state
    led_state = status_led_get_state();
    printf("LED state: %d\n", led_state);

    // Now flip the state
    status_led_set_state(!led_state);

    // Update led state
    led_state = status_led_get_state();

    return "/index.shtml";
}

static tCGI cgi_handlers[] = {
    { "/switch.cgi", switch_cgi_handler }
};

static const char *ssi_tags[] = {
    "led",
    "not_led"
};

// Note that the buffer size is limited by LWIP_HTTPD_MAX_TAG_INSERT_LEN, so use LWIP_HTTPD_SSI_MULTIPART to return larger amounts of data
static u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen 
#if LWIP_HTTPD_SSI_MULTIPART
    , uint16_t current_tag_part, uint16_t *next_tag_part
#endif
){
    int printed = 0;
    switch (iIndex) {
        case 0: { // "led"
            if (led_state) {
                printed = snprintf(pcInsert, iInsertLen, "ON");
            } else {
                printed = snprintf(pcInsert, iInsertLen, "OFF");
            }
            break;
        }
        case 1: { // "not_led"
            if (led_state) {
                printed = snprintf(pcInsert, iInsertLen, "OFF");
            } else {
                printed = snprintf(pcInsert, iInsertLen, "ON");
            }
            break;
        }
        default: { // unknown tag
            printed = 0;
            break;
        }
    }
  return (u16_t)printed;
}

void key_pressed_func(void *param) {
    bool *complete = (bool *)param;
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    if (key == 'd' || key == 'D') {
        *complete = true;
    }
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialise\n");
        return 1;
    }

    // Get notified if the user presses a key
    stdio_set_chars_available_callback(key_pressed_func, &complete);

    const char *ap_name = "picow_test";
#if 1
    const char *password = "password";
#else
    const char *password = NULL;
#endif

    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    #if LWIP_IPV6
    #define IP(x) ((x).u_addr.ip4)
    #else
    #define IP(x) (x)
    #endif

    ip4_addr_t mask;
    ip4_addr_t gw;
    IP(gw).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
    IP(mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);

    #undef IP

    // Start the dhcp server
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &gw, &mask);

    // Start the dns server
    dns_server_t dns_server;
    dns_server_init(&dns_server, &gw);

    // setup http server
    wifi_connected_time = get_absolute_time();
    cyw43_arch_lwip_begin();
    httpd_init();
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
    cyw43_arch_lwip_end();

    complete = false;
    while(!complete) {
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer interrupt) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        // if you are not using pico_cyw43_arch_poll, then Wi-FI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }

    // Shutdown
    cyw43_arch_disable_ap_mode();
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();
    printf("Complete\n");
    return 0;
}
