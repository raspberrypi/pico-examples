#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"
#include "pico/lwip_nosys.h"
#include "lwip/netif.h"
#include "lwip/udp.h"
#include "lwip/dns.h"

#include "dtls_common.h"

typedef struct client_state_t_ {
    dtls_base_t base;
    dtls_session_t session;
    bool complete;
    int test_string_id;
    int expected_len;
} client_state_t;

static const char *test_data[] = {
    "if you can see this, it worked!",
    "This is the second request",
    "The quick brown fox jumped over the lazy dog",
};
static bool test_passed = false;

#ifndef DTLS_SERVER_PORT
#define DTLS_SERVER_PORT 4433
#endif
#ifndef DTLS_SERVER
#error Need to define DTLS_SERVER
#endif

// Forward declarations
static int client_data_rx(dtls_session_t *session, struct pbuf *p);
static int client_handshake_done(dtls_session_t *session);
static void client_session_closed(dtls_session_t *session);

// Callback to receive data
static void client_udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    client_state_t *state = (client_state_t*)arg;
    assert(state && pcb && state->session.udp_pcb == pcb);
    if (p == NULL) {
        return;
    }
    if (state->session.port == 0 || ip_addr_isany_val(state->session.addr)) {
        return;
    }

    assert(port == state->session.port);
    assert(ip_addr_cmp(addr, &state->session.addr));

    if (state->session.rx == NULL) {
        state->session.rx = p;
    } else {
        assert(p->tot_len + (int)p->len <= 0xFFFF);
        pbuf_cat(state->session.rx, p);
    }

    DTLS_DEBUG("client_udp_recv_cb %d\n", p->tot_len);
    dtls_processing(&state->session, false);
}

static int start_client(client_state_t *state, const ip_addr_t *addr, int port)
{
    assert(ip_addr_isany_val(state->session.addr) && !state->session.active);
    // initialise the session
    ip_addr_copy(state->session.addr, *addr);
    state->session.port = port;
    state->session.active = true;
    state->session.user_data = state;
    state->session.udp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);

    // set the callbacks
    state->session.handle_data_rx_cb = client_data_rx;
    state->session.handle_handshake_done_cb = client_handshake_done;
    state->session.handle_session_closed_cb = client_session_closed;

    if (!state->session.udp_pcb) {
        DTLS_ERROR("Failed to allocate udp pcb");
        return ERR_MEM;
    }
    int ret = udp_connect(state->session.udp_pcb, addr, port);
    if (ret != ERR_OK) {
        DTLS_ERROR("udp_connect failed %d", ret);
        return ret;
    }
    // set receive callback
    udp_recv(state->session.udp_pcb, client_udp_recv_cb, state);
    // start handshake
    ret = dtls_processing(&state->session, false);
    if (ret != ERR_OK) {
        DTLS_ERROR("dtls_processing failed %d", ret);
        return ret;
    }
    return ERR_OK;
}

// Call back with a DNS result
static void server_address_found(const char *hostname, const ip_addr_t *addr, void *arg) {
    client_state_t *state = (client_state_t*)arg;
    if (addr) {
        DTLS_DEBUG("server address %s\n", ipaddr_ntoa(addr));
        if (start_client(state, addr, DTLS_SERVER_PORT) != 0) {
            panic("Client request failed");
        }
    } else {
        panic("dns request failed"); // todo: retry?
    }
}

// Send a string to the server
static int send_request(dtls_session_t *session, const char *string) {
    // send some data to the server
    DTLS_DEBUG("CLIENT OUT: %s\n", string);
    int ret = dtls_write(session, (uint8_t*)string, strlen(string));
    if (ret < 0) {
        return ret;
    }
    assert(ret == strlen(string));
    DTLS_DEBUG("client sent: %d bytes\n", ret);
    client_state_t *state = (client_state_t*)session->user_data;
    state->expected_len = ret;
    return ERR_OK;
}

// Handle received data
static int client_data_rx(dtls_session_t *session, struct pbuf *p)
{
    client_state_t *state = (client_state_t*)session->user_data;
    assert(state->expected_len > 0);
    uint16_t tot_len = p->tot_len;
    // print data received
    int offset = 0;
    while(offset < tot_len) {
        char data_in[32];
        uint16_t req_len = LWIP_MIN(tot_len - offset, sizeof(data_in));
        assert(req_len > 0);
        uint16_t data_in_len = pbuf_copy_partial(p, data_in, sizeof(data_in), offset);
        assert(data_in_len > 0);
        DTLS_DEBUG("CLIENT IN: (got %d need %d) %.*s\n", data_in_len, state->expected_len, data_in_len, data_in);
        // check the echo is correct
        const uint16_t string_len = (uint16_t)strlen(test_data[state->test_string_id]);
        if (strncmp(test_data[state->test_string_id] + string_len - state->expected_len, data_in, LWIP_MIN(state->expected_len, data_in_len)) != 0) {
            DTLS_ERROR("Echo mismatch want \"%s\" got \"%s\"\n", test_data[state->test_string_id] + string_len - state->expected_len, data_in);
            assert(false);
        }
        offset += data_in_len;
        state->expected_len -= data_in_len;
    }
    pbuf_free(p);
    if (state->expected_len <= 0) {
        // Send the next request or close the session
        client_state_t *state = (client_state_t*)session->user_data;
        if (++state->test_string_id < count_of(test_data)) {
            return send_request(session, test_data[state->test_string_id]);
        } else {
            test_passed = true;
            return dtls_session_close(session);
        }
    }
    return ERR_OK;
}

// Called when handshake has completed
static int client_handshake_done(dtls_session_t *session)
{
    client_state_t *state = (client_state_t*)session->user_data;
    return send_request(session, test_data[state->test_string_id]);
}

// Called when session is actually ready to close
static void client_session_closed(dtls_session_t *session)
{
    client_state_t *state = (client_state_t*)session->user_data;
    state->complete = true;
    session->active = false;
    // Get rid of udp pcb
    if (session->udp_pcb) {
        udp_remove(session->udp_pcb);
        session->udp_pcb = NULL;
    }
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
    DTLS_INFO("Client ip address %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));

    client_state_t state;
    memset(&state, 0, sizeof(state));

    int ret = dtls_base_init(&state.base);
    if (ret != ERR_OK) {
        panic("Failed to initialise dtls");
    }
    ret = dtls_session_init(&state.session, &state.base, cyw43_arch_async_context(), DTLS_SERVER);
    if (ret != ERR_OK) {
        panic("Failed to initialise dtls session");
    }

    cyw43_arch_lwip_begin();
    ip_addr_t addr;
    ret = dns_gethostbyname(DTLS_SERVER, &addr, server_address_found, &state);
    cyw43_arch_lwip_end();
    if (ret == ERR_OK) {
        cyw43_arch_lwip_begin();
        ret = start_client(&state, &addr, DTLS_SERVER_PORT);
        cyw43_arch_lwip_end();
        if (ret != 0) {
            panic("client request failed %d", ret);
        }
    } else if (ret != ERR_INPROGRESS) {
        panic("dns request failed");
    }

    while(!state.complete) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
    lwip_nosys_deinit(cyw43_arch_async_context());
    cyw43_arch_deinit();

    dtls_session_deinit(&state.session);
    dtls_base_deinit(&state.base);

    DTLS_INFO("All done. Test %s\n", test_passed ? "passed" : "failed");
    sleep_ms(100);
}
