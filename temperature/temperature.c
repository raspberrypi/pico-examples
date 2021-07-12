/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"


const uint8_t ADDRESS = 0X00;
const uint8_t WRITE_MODE = 0XFE;
const uint8_t READ_MODE = 0XFF;


//hardware registers

const uint8_t REG_POINTER = 0X00;
const uint8_t REG_CONFIG = 0X01;
const uint8_t REG_TEMP_UPPER = 0X02;
const uint8_t REG_TEMP_LOWER = 0X04;
const uint8_t REG_TEMP_CRIT = 0X04;
const uint8_t REG_TEMP_AMB = 0X05;
const uint8_t REG_MAN_ID = 0X06;
const uint8_t REG_DEV_ID = 0X07;
const uint8_t REG_RESOLUTION = 0X08;



