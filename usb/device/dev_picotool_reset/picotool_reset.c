/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2025 Framework Computer Inc
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    // Don't exit, wait for control transfer
    while (true) {}
}
