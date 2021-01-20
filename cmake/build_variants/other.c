/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "other.h"

void do_other() {
    printf("The common thing is %d\n",
           A_DEFINE_THAT_IS_SHARED);
    printf("The binary local thing is %d\n",
           A_DEFINE_THAT_IS_NOT_SHARED);
}