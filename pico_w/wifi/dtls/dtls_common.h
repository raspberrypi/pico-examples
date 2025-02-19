/**
 * Copyright (c) 2023 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _DTLS_COMMON_H_
#define _DTLS_COMMON_H_

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/debug.h"

#include "lwip/udp.h"
#include "pico/async_context.h"
#include "pico/time.h"

#ifndef DTLS_PRINT
#define DTLS_PRINT printf
#endif
#define DTLS_DEBUGV(...)
#ifdef NDEBUG
#define DTLS_DEBUG(...)
#define DTLS_INFO(...)
#else
#define DTLS_DEBUG DTLS_PRINT
#define DTLS_INFO DTLS_PRINT
#endif
#define DTLS_ERROR DTLS_PRINT

typedef struct pico_mbedtls_timer_context_t_ {
    async_context_t *async_context;
    async_at_time_worker_t worker;
    absolute_time_t fin_time;
    absolute_time_t int_time;
} pico_mbedtls_timer_context_t;

typedef struct dtls_base_t_ {
    mbedtls_ssl_config conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    pico_mbedtls_timer_context_t timer_ctx;
    mbedtls_x509_crt cert;
#ifndef DTLS_SERVER
    mbedtls_ssl_cookie_ctx cookie_ctx;
#endif
    mbedtls_pk_context pkey;
} dtls_base_t;

typedef struct dtls_session_t_ {
    mbedtls_ssl_context ssl;
    struct udp_pcb *udp_pcb;
    struct pbuf *rx;
    ip_addr_t addr;
    u16_t port;
    bool close_queued;
    bool handshake_done;
    bool active;
    void *user_data;
    // callbacks to the user
    int (*handle_data_rx_cb)(struct dtls_session_t_*, struct pbuf*);
    int (*handle_handshake_done_cb)(struct dtls_session_t_*);
    void (*handle_session_closed_cb)(struct dtls_session_t_*);
} dtls_session_t;


int dtls_base_init(dtls_base_t *base);
void dtls_base_deinit(dtls_base_t *base);

int dtls_session_init(dtls_session_t *session, dtls_base_t *base, async_context_t *context, const char *hostname);
int dtls_set_client(dtls_session_t *session, const ip_addr_t *addr, u16_t port);
int dtls_write(dtls_session_t *session, const uint8_t *data, size_t data_len);
int dtls_processing(dtls_session_t *session, bool timeout);
int dtls_session_close(dtls_session_t *session);
void dtls_session_deinit(dtls_session_t *session);

#endif
