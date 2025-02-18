/**
 * Copyright (c) 2023 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"
#include "pico/lwip_nosys.h"
#include "lwip/netif.h"

#include "dtls_common.h"

typedef struct server_state_t_ {
    dtls_base_t base;
    dtls_session_t session;
    bool complete;
} server_state_t;

#define LISTEN_PORT 4433

static int server_result(server_state_t *state, int status) {
    if (status == 0) {
        DTLS_DEBUG("test success\n");
    } else {
        DTLS_DEBUG("test failed %d\n", status);
    }
    state->complete = true;
    return dtls_session_close(&state->session);
}

static int server_data_rx(dtls_session_t *session, struct pbuf *p)
{
    int ret = ERR_OK;
    uint16_t tot_len = p->tot_len;
    // Print data received
    int offset = 0;
    while(offset < tot_len) {
        char data_in[32];
        uint16_t req_len = LWIP_MIN(tot_len - offset, sizeof(data_in));
        assert(req_len > 0);
        (void)req_len;
        uint16_t data_in_len = pbuf_copy_partial(p, data_in, sizeof(data_in), offset);
        assert(data_in_len > 0);
        DTLS_DEBUG("SERVER IN: %.*s\n", data_in_len, data_in);
        offset += data_in_len;
    }
    // echo it back as a reply
    offset = 0;
    while(offset < tot_len) {
        char data_out[32];
        uint16_t resp_len = LWIP_MIN(tot_len - offset, sizeof(data_out));
        assert(resp_len > 0);
        (void)resp_len;
        uint16_t data_out_len = pbuf_copy_partial(p, data_out, sizeof(data_out), offset);
        assert(data_out_len > 0);
        DTLS_DEBUG("SERVER OUT: %.*s\n", data_out_len, data_out);
        int ret = dtls_write(session, (uint8_t*)data_out, data_out_len);
        if (ret < 0) {
            break;
        }
        DTLS_DEBUG("server sent: %d bytes\n", ret);
        offset += data_out_len;
    }
    pbuf_free(p);
    return ret;
}

// Callback to receive data
static void server_udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    server_state_t *state = (server_state_t*)arg;
    assert(state && pcb && state->session.udp_pcb == pcb);
    if (p == NULL) {
        return;
    }
    // Ignore?
    if (((state->session.port != 0) && (port != state->session.port)) ||
            (!ip_addr_isany_val(state->session.addr) && !ip_addr_eq(&state->session.addr, addr))) {
        pbuf_free(p);
        return;
    }
    // New client connected?
    if (state->session.port == 0 && ip_addr_isany_val(state->session.addr)) {
        int ret = dtls_set_client(&state->session, addr, port);
        if (ret != ERR_OK) {
            DTLS_ERROR("failed to set client details %d", ret);
            pbuf_free(p);
            return;
        }
    }
    if (state->session.rx == NULL) {
        state->session.rx = p;
    } else {
        assert(p->tot_len + (int)p->len <= 0xFFFF);
        pbuf_cat(state->session.rx, p);
    }
    DTLS_DEBUG("server_udp_recv_cb %d\n", p->tot_len);
    dtls_processing(&state->session, false);
}

int main() {
    stdio_init_all();

    // Create a context
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect\n");
        return 1;
    }

    server_state_t state;
    memset(&state, 0, sizeof(state));

    int ret = dtls_base_init(&state.base);
    if (ret != ERR_OK) {
        panic("Failed to initialise dtls");
    }
    ret = dtls_session_init(&state.session, &state.base, cyw43_arch_async_context(), NULL);
    if (ret != ERR_OK) {
        panic("Failed to initialise dtls session");
    }

    DTLS_INFO("Waiting for remote connection on %s:%d\n", ipaddr_ntoa(&(netif_list->ip_addr)), LISTEN_PORT);
    state.session.udp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state.session.udp_pcb) {
        panic("Failed to allocate pcb");
    }
    ret = udp_bind(state.session.udp_pcb, &(netif_list->ip_addr), LISTEN_PORT);
    if (ret != ERR_OK) {
        panic("bind failed");
    }
    udp_recv(state.session.udp_pcb, server_udp_recv_cb, &state);
    state.session.handle_data_rx_cb = server_data_rx;
    state.session.active = true;

    while(!state.complete) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
    lwip_nosys_deinit(cyw43_arch_async_context());
    cyw43_arch_deinit();

    dtls_base_deinit(&state.base);
    dtls_session_deinit(&state.session);

    DTLS_INFO("All done\n");
    sleep_ms(100);
}
