/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RPI_CONNECT_OTA_DEMO_H
#define RPI_CONNECT_OTA_DEMO_H

#include <stdio.h>
#include <stdint.h>

#include "pico/time.h"

/* Demo application logging ----------------------------------------------------
 * The app has its own logging namespace, separate from the rpi_connect*
 * libraries, so its flow of control stays readable. Levels:
 *   ERROR - failures, always on.
 *   INFO  - flow of control / status milestones, always on. This is what you
 *           normally watch when running the example.
 *   DEBUG - per-chunk / low-level detail, opt-in (off by default) to avoid
 *           log spam.
 * For deeper debugging of the libraries themselves, enable the library
 * RPI_CONNECT*_DEBUG_ENABLE flags in CMakeLists.txt (see the logging block). */
#ifndef RPI_CONNECT_OTA_DEMO_DEBUG_ENABLE
#define RPI_CONNECT_OTA_DEMO_DEBUG_ENABLE 0
#endif

#define RPI_CONNECT_OTA_DEMO_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define RPI_CONNECT_OTA_DEMO_INFO(...)  printf(__VA_ARGS__)

#if RPI_CONNECT_OTA_DEMO_DEBUG_ENABLE
#define RPI_CONNECT_OTA_DEMO_DEBUG(...) printf(__VA_ARGS__)
#else
#define RPI_CONNECT_OTA_DEMO_DEBUG(...) do {} while (0)
#endif

int rpi_connect_ota_demo_init(const char *client_id, const char *serial_number, const char *hostname);
int rpi_connect_ota_demo_main(const char *token);

#endif /* RPI_CONNECT_OTA_DEMO_H */
