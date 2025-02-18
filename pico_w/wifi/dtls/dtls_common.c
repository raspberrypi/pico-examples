#include <string.h>
#include "dtls_common.h"

#ifndef DTLS_CERT_INC
#error Need to define DTLS_CERT_INC
#endif
#include DTLS_CERT_INC

static const uint8_t dtls_root_cert[] = DTLS_ROOT_CERT;
static const uint8_t dtls_key[] = DTLS_KEY;
static const uint8_t dtls_cert[] = DTLS_CERT;

#include "mbedtls/net_sockets.h"

// If the timeout is zero the dtls connection will not timeout
// If the timeout is set then the dtls connection will be dropped if no data is received within this time
// Having a timeout is useful as there's no way for the client/server to know if the other end disappears
// Having a timeout implies you need some sort of keep alive from the client to the server?
#ifndef DTLS_READ_TIMEOUT_MS
#define DTLS_READ_TIMEOUT_MS 30000
#endif

// 0 No debug
// 1 Error
// 2 State change
// 3 Informational
// 4 Verbose
#ifndef MBEDTLS_DEBUG_LEVEL
#define MBEDTLS_DEBUG_LEVEL 1
#endif

static void dtls_timer_callback(__unused async_context_t *context, async_at_time_worker_t *worker) {
    DTLS_DEBUG("pico_mbedtls_timing_worker_callback\n");
    dtls_processing((dtls_session_t*)worker->user_data, true);
}

static void dtls_timer_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
    pico_mbedtls_timer_context_t *ctx = (pico_mbedtls_timer_context_t*)data;
    if (fin_ms == 0) {
        async_context_remove_at_time_worker(ctx->async_context, &ctx->worker);
        ctx->fin_time = nil_time;
        DTLS_DEBUGV("dtls_timer_set_delay cancelled\n");
        return;
    }
    // The async worker will get called
    DTLS_DEBUGV("dtls_timer_set_delay fin_ms=%u\n", fin_ms);
    async_context_add_at_time_worker_in_ms(ctx->async_context, &ctx->worker, fin_ms);
    ctx->fin_time = make_timeout_time_ms(fin_ms);
    ctx->int_time = make_timeout_time_ms(int_ms);
}

static int dtls_timer_get_delay(void *data)
{
    pico_mbedtls_timer_context_t *ctx = (pico_mbedtls_timer_context_t*)data;
    if (is_nil_time(ctx->fin_time)) {
        return -1 ;
    }
    if (time_reached(ctx->fin_time)) {
        DTLS_DEBUG("dtls_timer_get_delay FIN\n");
        return 2;
    }
    if (time_reached(ctx->int_time)) {
        DTLS_DEBUG("dtls_timer_get_delay INT\n");
        return 1;
    }
    return 0;
}

static void init_dtls_timer_context(pico_mbedtls_timer_context_t *timer_ctx, async_context_t *async_context, dtls_session_t *session) {
    timer_ctx->async_context = async_context;
    timer_ctx->worker.do_work = dtls_timer_callback;
    timer_ctx->worker.user_data = session;
    timer_ctx->fin_time = nil_time;
    mbedtls_ssl_set_timer_cb(&session->ssl, timer_ctx, dtls_timer_set_delay, dtls_timer_get_delay);
}

static void ssl_debug(void *ctx, int level, const char *file, int line, const char *str)
{
    (void)ctx;
    (void)level;
    DTLS_DEBUG("%s:%04d: %s", file, line, str);
}

static int bio_send(void *arg, const unsigned char *dataptr, size_t size)
{
    DTLS_DEBUG("bio_send %d\n", size);
    dtls_session_t *session = (dtls_session_t*)arg;
    assert(session && session->udp_pcb);
    assert(session->port != 0 && !ip_addr_isany_val(session->addr));
    assert(session->active);

    int written = 0;
    size_t size_left = size;

    while (size_left) {
        u16_t write_len = (u16_t)LWIP_MIN(size_left, 0xFFFF);
        struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, write_len, PBUF_RAM);
        if (!p) {
            DTLS_ERROR("pbuf alloc failed\n");
            break;
        }
        memcpy(p->payload, dataptr, write_len);
        int err = udp_sendto(session->udp_pcb, p, &session->addr, session->port);
        DTLS_DEBUG("udp_sendto %d\n", err);
        pbuf_free(p);
        assert(err == ERR_OK);
        if (err == ERR_OK) {
            written += write_len;
            size_left -= write_len;
            dataptr += write_len;
        } else if (err == ERR_MEM) {
            break;
        } else {
            return MBEDTLS_ERR_NET_SEND_FAILED;
        }
    }
    return written;
}

static int bio_recv(void *arg, unsigned char *buf, size_t len)
{
    dtls_session_t *session = (dtls_session_t*)arg;
    assert(session && session->udp_pcb);

    struct pbuf *p = session->rx;
    if (!p || (p->len == 0 && !p->next)) {
        if (p) {
          pbuf_free(p);
        }
        session->rx = NULL;
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    uint16_t copy_len = (u16_t)LWIP_MIN(len, p->tot_len);
    DTLS_DEBUG("bio_recv %d/%d\n", copy_len, len);
    uint16_t ret = pbuf_copy_partial(p, buf, copy_len, 0);
    p = pbuf_free_header(p, ret);
    if (p && p->len == 0) {
        session->rx = p->next;
        p->next = NULL;
        pbuf_free(p);
    }
    session->rx = p;
    return ret;
}

int dtls_base_init(dtls_base_t *base)
{
    int ret = ERR_OK;
#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(MBEDTLS_DEBUG_LEVEL);
#endif
    mbedtls_ssl_config_init(&base->conf);
    mbedtls_ctr_drbg_init(&base->ctr_drbg);
    mbedtls_ssl_conf_rng(&base->conf, mbedtls_ctr_drbg_random, &base->ctr_drbg);
    mbedtls_ssl_conf_dbg(&base->conf, ssl_debug, NULL);
    mbedtls_ssl_conf_read_timeout(&base->conf, DTLS_READ_TIMEOUT_MS);
    mbedtls_entropy_init(&base->entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&base->ctr_drbg, mbedtls_entropy_func, &base->entropy, (const unsigned char *)CUSTOM_MBEDTLS_ENTROPY_PTR, CUSTOM_MBEDTLS_ENTROPY_LEN)) != 0) {
        DTLS_ERROR("Failed to seed rnd %d", ret);
        return ret;
    }
    mbedtls_x509_crt_init(&base->cert);
    ret = mbedtls_x509_crt_parse(&base->cert, dtls_cert, sizeof(dtls_cert));
    if (ret != 0) {
        DTLS_ERROR("Failed to parse client cert %d", ret);
        return ret;
    }
    ret = mbedtls_x509_crt_parse(&base->cert, dtls_root_cert, sizeof(dtls_root_cert));
    if (ret != 0) {
        DTLS_ERROR("Failed to parse ca cert %d", ret);
        return ret;
    }
    mbedtls_ssl_conf_ca_chain(&base->conf, base->cert.next, NULL);
    mbedtls_pk_init(&base->pkey);
    ret = mbedtls_pk_parse_key(&base->pkey, dtls_key, sizeof(dtls_key), NULL, 0);
    if (ret != 0) {
        DTLS_ERROR("Failed to parse key");
        return ret;
    }
    if ((ret = mbedtls_ssl_conf_own_cert(&base->conf, &base->cert, &base->pkey)) != 0) {
        DTLS_ERROR("failed to load own cert %d", ret);
        return ret;
    }
    mbedtls_ssl_conf_authmode(&base->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
#ifdef DTLS_SERVER
    if ((ret = mbedtls_ssl_config_defaults(&base->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        DTLS_ERROR("ssl client config failed %d", ret);
        return ret;
    }
#else
    if ((ret = mbedtls_ssl_config_defaults(&base->conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        DTLS_ERROR("ssl server config failed %d", ret);
        return ret;
    }
    mbedtls_ssl_cookie_init(&base->cookie_ctx);
    if ((ret = mbedtls_ssl_cookie_setup(&base->cookie_ctx, mbedtls_ctr_drbg_random, &base->ctr_drbg)) != 0) {
        DTLS_ERROR("cookie setup failed %d", ret);
        return ret;
    }
    mbedtls_ssl_conf_dtls_cookies(&base->conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &base->cookie_ctx);
#endif
    return ret;
}

void dtls_base_deinit(dtls_base_t *base)
{
    mbedtls_x509_crt_free(&base->cert);
    mbedtls_pk_free(&base->pkey);
#ifndef DTLS_SERVER
    mbedtls_ssl_cookie_free(&base->cookie_ctx);
#endif
    mbedtls_ssl_config_free(&base->conf);
    mbedtls_ctr_drbg_free(&base->ctr_drbg);
    mbedtls_entropy_free(&base->entropy);
}

int dtls_session_init(dtls_session_t *session, dtls_base_t *base, async_context_t *context, const char *hostname)
{
    int ret;
    mbedtls_ssl_init(&session->ssl);
    if ((ret = mbedtls_ssl_setup(&session->ssl, &base->conf)) != 0) {
        DTLS_ERROR("ssl setup failed %d", ret);
        return ret;
    }
    mbedtls_ssl_set_bio(&session->ssl, session, bio_send, bio_recv, NULL);
    init_dtls_timer_context(&base->timer_ctx, context, session);
    if (hostname) {
        if ((ret = mbedtls_ssl_set_hostname(&session->ssl, hostname)) != 0) { // server hostname
            DTLS_ERROR("Failed to set host name %d", ret);
            return ret;
        }
    }
    return ret;
}

void dtls_session_deinit(dtls_session_t *session)
{
    mbedtls_ssl_free(&session->ssl);
}

// receive tls data
static int recv_data(dtls_session_t *session)
{
    DTLS_DEBUG("recv_data\n");
    int ret;
    do {
        struct pbuf *buf = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
        if (buf == NULL) {
            DTLS_ERROR("pbuf_alloc failed\n");
            assert(0); // todo
            return ERR_OK;
        }
        ret = mbedtls_ssl_read(&session->ssl, (unsigned char *)buf->payload, PBUF_POOL_BUFSIZE);
        if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            pbuf_free(buf);
            return ERR_OK;
        }
        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) { // the other end is closing the connection
            DTLS_INFO("Peer closed the connection\n");
            pbuf_free(buf);
            return dtls_session_close(session);
        }
        if (ret < 0) {
            DTLS_ERROR("mbedtls_ssl_read error -0x%X\n", -ret);
            pbuf_free(buf);
            return dtls_session_close(session);
        }
        if (ret > 0) {
            assert(ret <= PBUF_POOL_BUFSIZE);
            pbuf_realloc(buf, (u16_t)ret);
            if (session->handle_data_rx_cb) {
                int err = session->handle_data_rx_cb(session, buf);
                if (err != ERR_OK) {
                    if (err == ERR_ABRT) {
                      return ERR_ABRT;
                    }
                    return ERR_OK;
                }
            } else {
                pbuf_free(buf);
            }
        } else {
            pbuf_free(buf);
        }
    } while (ret > 0 && session->active);
    return ERR_OK;
}

int dtls_processing(dtls_session_t *session, bool timeout)
{
    DTLS_DEBUG("dtls_processing timeout=%d\n", timeout);
    int ret;
    if (session->close_queued) {
        dtls_session_close(session);
    }
    if (!session->handshake_done) {
        ret = mbedtls_ssl_handshake(&session->ssl);
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            // handshake not done, wait for more recv calls
            return ERR_OK;
        }
        if (ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
            // Handshake ok, client should reconnect with correct cookie
            return dtls_session_close(session);
        }
        if (ret != 0) {
            DTLS_ERROR("mbedtls_ssl_handshake failed: -0x%X\n", -ret);
            dtls_session_close(session);
            return ret;
        }
        if (session->handle_handshake_done_cb) {
            session->handle_handshake_done_cb(session); // todo: handle error?
        }
        session->handshake_done = true;
    }
    if (session->rx == NULL && !timeout) {
        DTLS_DEBUG("No data\n");
        return ERR_OK;
    }
    return recv_data(session);
}

int dtls_session_close(dtls_session_t *session) {
    // send notification
    int ret = mbedtls_ssl_close_notify(&session->ssl);
    DTLS_DEBUG("close_notify %d\n", ret);
    if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        session->close_queued = true;
        return ret;
    }
    if (session->handle_session_closed_cb) {
        session->handle_session_closed_cb(session);
    }
    mbedtls_ssl_session_reset(&session->ssl);
    session->close_queued = false;
    session->handshake_done = false;
    if (session->rx) {
        pbuf_free(session->rx);
        session->rx = NULL;
    }
    // clear client details
    session->port = 0;
    ip_addr_set_zero(&session->addr);
    return ERR_OK;
}

int dtls_write(dtls_session_t *session, const uint8_t *data, size_t data_len) {
    int ret = mbedtls_ssl_write(&session->ssl, data, data_len);
    assert(ret > 0);
    if (ret < 0) {
        DTLS_ERROR("mbedtls_ssl_write failed %d", ret);
    }
    return ret;
}

int dtls_set_client(dtls_session_t *session, const ip_addr_t *addr, u16_t port) {
    ip_addr_copy(session->addr, *addr);
    session->port = port;

    char client_ip[48];
    size_t client_ip_len;
    ipaddr_ntoa_r(addr, client_ip, sizeof(client_ip));
    client_ip_len = strlen(client_ip);
    client_ip_len += snprintf(client_ip + client_ip_len, sizeof(client_ip) - client_ip_len, ":%u", port);

    // For HelloVerifyRequest cookies
    int ret = mbedtls_ssl_set_client_transport_id(&session->ssl, (unsigned char *)client_ip, client_ip_len);
    if (ret == ERR_OK) {
        DTLS_DEBUG("New client %s\n", client_ip);
    }
    return ret;
}