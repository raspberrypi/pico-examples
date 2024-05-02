/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _PIO_TDM_H
#define _PIO_TDM_H

#include "hardware/pio.h"
#include "tdm.pio.h"

typedef struct pio_tdm_inst {
    PIO pio;
    uint sm;
    uint cs_pin;
} pio_tdm_inst_t;

void pio_tdm_write16_blocking(const pio_tdm_inst_t *tdm, const uint16_t *src, size_t len);

void pio_tdm_read16_blocking(const pio_tdm_inst_t *tdm, uint16_t *dst, size_t len);

void pio_tdm_write16_read16_blocking(const pio_tdm_inst_t *tdm, uint16_t *src, uint16_t *dst, size_t len);

#endif
