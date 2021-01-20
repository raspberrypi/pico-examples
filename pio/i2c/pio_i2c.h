/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _PIO_I2C_H
#define _PIO_I2C_H

#include "i2c.pio.h"

// ----------------------------------------------------------------------------
// Low-level functions

void pio_i2c_start(PIO pio, uint sm);
void pio_i2c_stop(PIO pio, uint sm);
void pio_i2c_repstart(PIO pio, uint sm);

bool pio_i2c_check_error(PIO pio, uint sm);
void pio_i2c_resume_after_error(PIO pio, uint sm);

// If I2C is ok, block and push data. Otherwise fall straight through.
void pio_i2c_put_or_err(PIO pio, uint sm, uint16_t data);
uint8_t pio_i2c_get(PIO pio, uint sm);

// ----------------------------------------------------------------------------
// Transaction-level functions

int pio_i2c_write_blocking(PIO pio, uint sm, uint8_t addr, uint8_t *txbuf, uint len);
int pio_i2c_read_blocking(PIO pio, uint sm, uint8_t addr, uint8_t *rxbuf, uint len);

#endif
