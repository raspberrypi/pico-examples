/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
// Include sys/types.h before inttypes.h to work around issue with
// certain versions of GCC and newlib which causes omission of PRIu64
#include <sys/types.h>
#include <inttypes.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "mbedtls/sha256.h"

#define BUFFER_SIZE 10000

int main() {
    stdio_init_all();
    // nist 3
    uint8_t *buffer = malloc(BUFFER_SIZE);
    memset(buffer, 0x61, BUFFER_SIZE);
    const uint8_t nist_3_expected[] = { \
        0xcd, 0xc7, 0x6e, 0x5c, 0x99, 0x14, 0xfb, 0x92, 0x81, 0xa1, \
        0xc7, 0xe2, 0x84, 0xd7, 0x3e, 0x67, 0xf1, 0x80, 0x9a, 0x48, \
        0xa4, 0x97, 0x20, 0x0e, 0x04, 0x6d, 0x39, 0xcc, 0xc7, 0x11, \
        0x2c, 0xd0 };

    // check mbedtls hw accelerated speed
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    uint64_t start = time_us_64();
    int rc = mbedtls_sha256_starts_ret(&ctx, 0);
    hard_assert(rc == 0);
    for(int i = 0; i < 1000000; i += BUFFER_SIZE) {
        rc = mbedtls_sha256_update_ret(&ctx, buffer, BUFFER_SIZE);
        hard_assert(rc == 0);
    }
    unsigned char mbed_result[32];
    rc = mbedtls_sha256_finish_ret(&ctx, mbed_result);
    hard_assert(rc == 0);
    uint64_t mbed_time = (time_us_64() - start) / 1000;
    printf("Mbedtls time for sha256 of 1M bytes %"PRIu64"ms\n", mbed_time);
    hard_assert(memcmp(nist_3_expected, mbed_result, 32) == 0);
    mbedtls_sha256_free(&ctx);
    hard_assert(mbed_time < 50); // less than 50ms
    free(buffer);
    printf("Test passed\n");
}
