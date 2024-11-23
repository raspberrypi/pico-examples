/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdio.h"
#include "pico/cyw43_arch.h"
#include "pico/async_context.h"
#include "lwip/altcp_tls.h"
#include "example_http_client_util.h"

#define HOST "fw-download-alias1.raspberrypi.com"
#define URL_REQUEST "/net_install/boot.sig"

int main() {
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("failed to connect\n");
        return 1;
    }

    EXAMPLE_HTTP_REQUEST_T req1 = {0};
    req1.hostname = HOST;
    req1.url = URL_REQUEST;
    req1.headers_fn = http_client_header_print_fn;
    req1.recv_fn = http_client_receive_print_fn;
    int result = http_client_request_sync(cyw43_arch_async_context(), &req1);
    result += http_client_request_sync(cyw43_arch_async_context(), &req1); // repeat

    // test async
    EXAMPLE_HTTP_REQUEST_T req2 = req1;
    result += http_client_request_async(cyw43_arch_async_context(), &req1);
    result += http_client_request_async(cyw43_arch_async_context(), &req2);
    while(!req1.complete && !req2.complete) {
        async_context_poll(cyw43_arch_async_context());
        async_context_wait_for_work_ms(cyw43_arch_async_context(), 1000);
    }

    req1.tls_config = altcp_tls_create_config_client(NULL, 0); // https
    result += http_client_request_sync(cyw43_arch_async_context(), &req1);
    result += http_client_request_sync(cyw43_arch_async_context(), &req1); // repeat
    altcp_tls_free_config(req1.tls_config);

    if (result != 0) {
        panic("test failed");
    }
    cyw43_arch_deinit();
    printf("Test passed\n");
    sleep_ms(100);
    return 0;
}