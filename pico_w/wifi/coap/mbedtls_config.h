#ifndef MBEDTLS_CONFIG_TLS_CLIENT_H
#define MBEDTLS_CONFIG_TLS_CLIENT_H

#include "mbedtls_config_examples_common.h"

// Needed for dtls
#define MBEDTLS_SSL_PROTO_DTLS
#define MBEDTLS_SSL_COOKIE_C
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY
#define MBEDTLS_DEBUG_C
#define MBEDTLS_TIMING_ALT
#define MBEDTLS_VERSION_C
#define MBEDTLS_SSL_COOKIE_C

struct mbedtls_x509_crt;
static inline int mbedtls_x509_crt_parse_file(struct mbedtls_x509_crt *chain, const char *path) {
    return -4;
}

struct mbedtls_pk_context;
static inline int mbedtls_pk_parse_keyfile(struct mbedtls_pk_context *ctx, const char *path, const char *password) {
    return -4;
}

#endif