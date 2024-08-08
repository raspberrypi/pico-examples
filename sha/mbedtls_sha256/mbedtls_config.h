#ifndef _MBEDTLS_CONFIG_H
#define _MBEDTLS_CONFIG_H

#if LIB_PICO_SHA256
// Enable hardware acceleration
#define MBEDTLS_SHA256_ALT
#else
#define MBEDTLS_SHA256_C
#endif

#endif