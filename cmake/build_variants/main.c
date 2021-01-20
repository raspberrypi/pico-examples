/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "other.h"

int main() {
    stdio_init_all();
    do_other();
#ifdef DO_EXTRA
    printf("A little extra\n");
#endif
    return 0;
}