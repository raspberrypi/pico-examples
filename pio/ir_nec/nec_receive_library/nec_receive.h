/**
 * Copyright (c) 2021 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"

// public API

int nec_rx_init(PIO pio, uint pin);
bool nec_decode_frame(uint32_t sm, uint8_t *p_address, uint8_t *p_data);
