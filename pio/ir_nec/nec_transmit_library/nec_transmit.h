/**
 * Copyright (c) 2021 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"

// public API

int nec_tx_init(PIO pio, uint pin);
uint32_t nec_encode_frame(uint8_t address, uint8_t data);
