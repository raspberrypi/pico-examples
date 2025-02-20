/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// A simple coap client that requests "coap://<COAP_SERVER>/.well-known/core" and "coap://<COAP_SERVER>/time"
// Run coap-server from the libcoap library, e.g. for udp // ./coap-server -v 7
// coap with dtls currently only works with tinydtls and it only supports raw public keys
// build the server like this cmake .. -DDTLS_BACKEND=tinydtls
// then run the server like this: ./coap-server -M server.key -v 7

#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/lwip_nosys.h"

#include "lwip/netif.h"
#include "lwip/dns.h"

#include "coap_config.h"
#include <coap3/coap.h>

#ifdef COAP_CERT_INC
#include COAP_CERT_INC
#endif

#ifndef COAP_SERVER
#error Need to define COAP_SERVER
#endif

static coap_context_t *main_coap_context;

// token for observing time requests
static uint8_t time_observe_token_data[COAP_TOKEN_DEFAULT_MAX];
coap_binary_t time_observe_token = { 0, time_observe_token_data };

// token for observing led requests
static uint8_t led_observe_token_data[COAP_TOKEN_DEFAULT_MAX];
coap_binary_t led_observe_token = { 0, led_observe_token_data };

// Give up after getting a few responses
static int time_counter = 10;
static bool test_complete;

// The client sends a ping expecting a confirmation. It retries 4 times
// If the server is dead we'll see a COAP_NACK_TOO_MANY_RETRIES error
#define KEEP_ALIVE_TIME_S 30

#ifndef MAX_COAP_URI_STR
#define MAX_COAP_URI_STR 100
#endif

#define COAP_CN "pico"

#ifdef COAP_CERT_INC
#define COAP_PORT COAPS_DEFAULT_PORT
#else
#define COAP_PORT COAP_DEFAULT_PORT
#endif

void coap_io_process_worker_func(async_context_t *context, async_at_time_worker_t *worker)
{
    assert(worker->user_data);
    coap_context_t *coap_context = (coap_context_t*)worker->user_data;

    coap_tick_t before;
    unsigned int num_sockets;
    unsigned int timeout;

    coap_ticks(&before);
    timeout = coap_io_prepare_io(coap_context, NULL, 0, &num_sockets, before);
    if (timeout < 1000) {
        timeout = 1000;
    }
    async_context_add_at_time_worker_in_ms(context, worker, timeout);
    coap_context->timer_configured = 1;
}
static async_at_time_worker_t coap_worker = {
    .do_work = coap_io_process_worker_func
};

static coap_response_t message_handler(coap_session_t *session, const coap_pdu_t *sent, const coap_pdu_t *received, const coap_mid_t id) {
    const uint8_t *data;
    size_t len, offset, total;
    if (coap_get_data_large(received, &len, &data, &offset, &total)) {
        printf("%*.*s", (int)len, (int)len, (const char *)data);
        if (len + offset == total) {
            printf("\n");
        }
    }
    coap_bin_const_t token = coap_pdu_get_token(received);
    if (coap_binary_equal(&token, &time_observe_token)) {
        time_counter--;
        if (time_counter <= 0 && time_observe_token.length > 0) {
            coap_cancel_observe(session, &time_observe_token, coap_pdu_get_type(sent));
            time_observe_token.length = 0;
        }
    } else if (coap_binary_equal(&token, &led_observe_token)) {
        if (len >= 1) {
            cyw43_gpio_set(&cyw43_state, CYW43_WL_GPIO_LED_PIN, data[0] != '0');
        }
    }
    return COAP_RESPONSE_OK;
}

static void nack_handler(coap_session_t *session COAP_UNUSED, const coap_pdu_t *sent COAP_UNUSED, const coap_nack_reason_t reason, const coap_mid_t id COAP_UNUSED) {
    coap_log_debug("nack_handler: %d\n", reason);
    switch (reason) {
        case COAP_NACK_TOO_MANY_RETRIES:
        case COAP_NACK_NOT_DELIVERABLE:
        case COAP_NACK_RST:
        case COAP_NACK_TLS_FAILED:
            panic("cannot send coap pdu");
            break;
        case COAP_NACK_ICMP_ISSUE:
        default:
            break;
    }
    return;
}

bool make_request(coap_session_t *session, const coap_address_t *server_addr_coap, const char *url, bool confirm, coap_binary_t *subscribe_token) {

    static char uri_str[MAX_COAP_URI_STR];
    snprintf(uri_str, sizeof(uri_str), "coap://%s/%s", ipaddr_ntoa(&server_addr_coap->addr), url);

    bool success = false;
    coap_optlist_t *optlist = NULL;
    do {
        coap_uri_t uri_coap;
        int len = coap_split_uri((const unsigned char *)uri_str, strlen(uri_str), &uri_coap);
        assert(len == 0);
        if (len != 0) {
            coap_log_err("coap_split_uri failed\n");
            break;
        }
        coap_pdu_t *pdu = coap_pdu_init(confirm ? COAP_MESSAGE_CON : COAP_MESSAGE_NON, COAP_REQUEST_CODE_GET, coap_new_message_id(session), coap_session_max_pdu_size(session));
        if (!pdu) {
            coap_log_err("coap_pdu_init failed\n");
            break;
        }

        static uint8_t buf[MAX_COAP_URI_STR];
        len = coap_uri_into_options(&uri_coap, server_addr_coap, &optlist, 1, buf, sizeof(buf));
        assert(len == 0 && optlist);
        if (len != 0 || !optlist) {
            coap_log_err("coap_uri_into_options failed\n");
            break;
        }
        if (subscribe_token) {
            // add observe option
            coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_OBSERVE, coap_encode_var_safe(buf, sizeof(buf), COAP_OBSERVE_ESTABLISH), buf));
            // add token to request
            coap_session_new_token(session, &subscribe_token->length, subscribe_token->s);
            if (!coap_add_token(pdu, subscribe_token->length, subscribe_token->s)) {
                coap_log_debug("cannot add token to request\n");
            }
        }

        int res = coap_add_optlist_pdu(pdu, &optlist);
        if (res != 1) {
            coap_log_err("coap_add_optlist_pdu failed\n");
            break;
        }

        printf("Sending %s\n", uri_str);
        success = (coap_send(session, pdu) >= 0);
        if (!success) {
            coap_log_err("coap_send failed\n");
            break;
        }
    } while(false);
    if (optlist) coap_delete_optlist(optlist);
    return success;
}

static int verify_cn_callback(const char *cn,
        const uint8_t *asn1_public_cert, size_t asn1_length COAP_UNUSED,
        coap_session_t *session COAP_UNUSED, unsigned depth,
        int validated COAP_UNUSED, void *arg COAP_UNUSED) {
    coap_log_info("CN '%s' presented by server (%s)\n", cn, depth ? "CA" : "Certificate");
    return 1;
}

static coap_context_t *start_client(const ip_addr_t *server_addr) {
    coap_log_info("server address %s\n", ipaddr_ntoa(server_addr));

    // Should not call this twice
    static bool done;
    assert(!done);

#ifndef NDEBUG
    coap_set_log_level(COAP_LOG_DEBUG);
#else
    coap_set_log_level(COAP_LOG_INFO);
#endif

    coap_context_t *ctx = coap_new_context(NULL);
    assert(ctx);
    if (!ctx) {
        coap_log_err("failed to create coap context\n");
        return NULL;
    }

    coap_context_set_keepalive(ctx, KEEP_ALIVE_TIME_S);
    coap_context_set_block_mode(ctx, COAP_BLOCK_USE_LIBCOAP);

    coap_address_t server_addr_coap;
    server_addr_coap.addr = *server_addr;
    coap_address_set_port(&server_addr_coap, COAP_PORT);

    coap_session_t *session = NULL;
#ifdef COAP_CERT_INC
    // Set up security stuff
    static const uint8_t coap_key[] = COAP_KEY;
    static const uint8_t coap_pub_key[] = COAP_PUB_KEY;
    static coap_dtls_pki_t dtls_pki;
    memset(&dtls_pki, 0, sizeof(dtls_pki));
    dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
    dtls_pki.validate_cn_call_back = verify_cn_callback;
    dtls_pki.client_sni = COAP_CN;
    dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM_BUF;
    dtls_pki.is_rpk_not_cert = 1; // tinydtls only supports raw public keys
    dtls_pki.verify_peer_cert = 1; // not implemented?
    dtls_pki.pki_key.key.pem_buf.public_cert = coap_pub_key; // rpk
    dtls_pki.pki_key.key.pem_buf.public_cert_len = sizeof(coap_pub_key);
    dtls_pki.pki_key.key.pem_buf.private_key = coap_key;
    dtls_pki.pki_key.key.pem_buf.private_key_len = sizeof(coap_key);
    session = coap_new_client_session_pki(ctx, NULL, &server_addr_coap, COAP_PROTO_DTLS, &dtls_pki);
#else
    session = coap_new_client_session(ctx, NULL, &server_addr_coap, COAP_PROTO_UDP);
#endif
    assert(session);
    if (!session) {
        coap_log_err("failed to create coap session\n");
        coap_free_context(ctx);
        return NULL;
    }

    coap_register_response_handler(ctx, message_handler);
    coap_register_nack_handler(ctx, nack_handler);

    bool success = make_request(session, &server_addr_coap, ".well-known/core", true, NULL);
    assert(success);
    if (!success) {
        coap_log_err("coap request failed\n");
        coap_free_context(ctx);
        return NULL;
    }
    success = make_request(session, &server_addr_coap, "time", true, &time_observe_token);
    assert(success);
    if (!success) {
        coap_log_err("coap time request failed\n");
        coap_free_context(ctx);
        return NULL;
    }
    success = make_request(session, &server_addr_coap, "led", true, &led_observe_token);
    assert(success);
    if (!success) {
        coap_log_err("coap led request failed\n");
        coap_free_context(ctx);
        return NULL;
    }

    done = true;
    return ctx;
}

// Call back with a DNS result
static void server_address_found(__unused const char *hostname, const ip_addr_t *addr, __unused void *arg) {
    if (addr) {
        main_coap_context = start_client(addr);
        if (!main_coap_context) {
            panic("Test failed");
        }
        coap_worker.user_data = main_coap_context;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &coap_worker, 1000);
    } else {
        panic("dns request failed"); // todo: retry?
    }
}

static bool coap_client_init(async_context_t *context)
{
    coap_startup();
    coap_set_log_level(COAP_LOG_WARN);

    async_context_acquire_lock_blocking(context);
    ip_addr_t addr;
    int ret = dns_gethostbyname(COAP_SERVER, &addr, server_address_found, main_coap_context);
    async_context_release_lock(context);
    if (ret == ERR_OK) {
        async_context_acquire_lock_blocking(context);
        server_address_found(COAP_SERVER, &addr, NULL);
        coap_worker.user_data = main_coap_context;
        async_context_add_at_time_worker_in_ms(context, &coap_worker, 1000);
        async_context_release_lock(context);
    } else if (ret != ERR_INPROGRESS) {
        panic("dns request failed");
    }

    return true;
}

static void coap_client_deinit(void)
{
    coap_free_context(main_coap_context);
    main_coap_context = NULL;
    coap_cleanup();
}

void key_pressed_worker_func(async_context_t *context, async_when_pending_worker_t *worker) {
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    if (test_complete) {
        return;
    }
    if (key == 'q' || key == 'Q') {
        test_complete = true;
    }
}

static async_when_pending_worker_t key_pressed_worker = {
        .do_work = key_pressed_worker_func
};

void key_pressed_func(void *param) {
    key_pressed_worker.user_data = param;
    async_context_set_work_pending(cyw43_arch_async_context(), &key_pressed_worker);
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
    printf("Ready, using server at %s\n", COAP_SERVER);
    printf("Press 'q' to quit\n");
    cyw43_wifi_pm(&cyw43_state, CYW43_NONE_PM);

    coap_client_init(cyw43_arch_async_context());
    async_context_add_when_pending_worker(cyw43_arch_async_context(), &key_pressed_worker);
    stdio_set_chars_available_callback(key_pressed_func, NULL);

    while (!test_complete) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }

    coap_client_deinit();
    lwip_nosys_deinit(cyw43_arch_async_context());
    cyw43_arch_deinit();
    panic("Test passed");
    return 0;
}

void coap_address_init(coap_address_t *addr) {
  assert(addr);
  memset(addr, 0, sizeof(coap_address_t));
}
