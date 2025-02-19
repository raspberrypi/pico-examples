#ifndef MBEDTLS_CONFIG_TLS_CLIENT_H
#define MBEDTLS_CONFIG_TLS_CLIENT_H

#include "mbedtls_config_examples_common.h"

// Needed for dtls
#define MBEDTLS_SSL_PROTO_DTLS
#define MBEDTLS_SSL_COOKIE_C
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY

#endif