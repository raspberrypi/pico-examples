/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

/* Example code to talk to a BMP280 temperature and pressure sensor

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (on Pico this is 4 (pin 6)) -> SDA on BMP280
   board GPIO PICO_DEFAULT_I2C_SCK_PIN (on Pico this is 5 (pin 7)) -> SCL on
   BMP280 board 3.3v (pin 36) -> VCC on BMP280 board GND (pin 38)  -> GND on
   BMP280 board
*/

// device has default bus address of 0x76
const uint8_t addr = 0x76;

// hardware registers
const uint8_t REG_CONFIG = 0xF5;
const uint8_t REG_CTRL_MEAS = 0xF4;
const uint8_t REG_RESET = 0xE0;

const uint8_t REG_TEMP_XLSB = 0xFC;
const uint8_t REG_TEMP_LSB = 0xFB;
const uint8_t REG_TEMP_MSB = 0xFA;

const uint8_t REG_PRESSURE_XLSB = 0xF9;
const uint8_t REG_PRESSURE_LSB = 0xF8;
const uint8_t REG_PRESSURE_MSB = 0xF7;

// calibration registers
const uint8_t REG_DIG_T1_LSB = 0x88;
const uint8_t REG_DIG_T1_MSB = 0x89;
const uint8_t REG_DIG_T2_LSB = 0x8A;
const uint8_t REG_DIG_T2_MSB = 0x8B;
const uint8_t REG_DIG_T3_LSB = 0x8C;
const uint8_t REG_DIG_T3_MSB = 0x8D;
const uint8_t REG_DIG_P1_LSB = 0x8E;
const uint8_t REG_DIG_P1_MSB = 0x8F;
const uint8_t REG_DIG_P2_LSB = 0x90;
const uint8_t REG_DIG_P2_MSB = 0x91;
const uint8_t REG_DIG_P3_LSB = 0x92;
const uint8_t REG_DIG_P3_MSB = 0x93;
const uint8_t REG_DIG_P4_LSB = 0x94;
const uint8_t REG_DIG_P4_MSB = 0x95;
const uint8_t REG_DIG_P5_LSB = 0x96;
const uint8_t REG_DIG_P5_MSB = 0x97;
const uint8_t REG_DIG_P6_LSB = 0x98;
const uint8_t REG_DIG_P6_MSB = 0x99;
const uint8_t REG_DIG_P7_LSB = 0x9A;
const uint8_t REG_DIG_P7_MSB = 0x9B;
const uint8_t REG_DIG_P8_LSB = 0x9C;
const uint8_t REG_DIG_P8_MSB = 0x9D;
const uint8_t REG_DIG_P9_LSB = 0x9E;
const uint8_t REG_DIG_P9_MSB = 0x9F;

const uint8_t BMP280_WRITE_MODE = 0xFE;
const uint8_t BMP280_READ_MODE = 0xFF;

#ifdef i2c_default
void bmp280_init() {
  // use the "handheld device dynamic" optimal setting (see datasheet)
  uint8_t buf[2];

  // 500ms sampling time, x16 filter, not set
  uint8_t reg_config_val = ((0x04 << 5) | (0x05 << 2)) & 0xFC;

  // LSB of slave address sets read or write mode
  // send register number followed by its corresponding value
  buf[0] = REG_CONFIG;
  buf[1] = reg_config_val;
  i2c_write_blocking(i2c_default, (addr & BMP280_WRITE_MODE), buf, 2, false);

  // osrs_t x1, osrs_p x4, normal mode operation
  uint8_t reg_ctrl_meas_val = (0x01 << 5) | (0x03 << 2) | (0x03);
  buf[0] = REG_CTRL_MEAS;
  buf[1] = reg_ctrl_meas_val;
  i2c_write_blocking(i2c_default, (addr & BMP280_WRITE_MODE), buf, 2, false);
}

void bmp280_read_raw(int32_t *temp, int32_t *pressure) {
  // BMP280 data registers are auto-incrementing and we have 3 temperature and
  // pressure registers, each so we start at 0xF7 and read 6 bytes to 0xFC note:
  // normal mode does not require further ctrl_meas and config register writes

  uint8_t buf[6];
  i2c_write_blocking(i2c_default, (addr & BMP280_WRITE_MODE), &REG_PRESSURE_MSB,
                     1, true);  // true to keep master control of bus
  i2c_read_blocking(i2c_default, (addr & BMP280_READ_MODE), buf, 6,
                    false);  // false - finished with bus

  *pressure = (buf[0] << 16) | (buf[1] << 8) | (buf[2] >> 4);
  *temp = (buf[3] << 16) | (buf[4] << 8) | (buf[5] >> 4);
}

void bmp280_reset() {
  // reset the device with the power-on-reset procedure
}

#endif

int main() {
  stdio_init_all();
  // useful information for picotool
  bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN,
                             GPIO_FUNC_I2C));

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || \
    !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c/bmp280_i2c example requires a board with I2C pins
  puts("Default I2C pins were not defined");
#else
  printf(
      "Hello, BMP280! Reading temperaure and pressure values from sensor...\n");

  // I2C is "open drain", pull ups to keep signal high when no communication
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

  // configure BMP280
  bmp280_init();

  int32_t temp;
  int32_t pressure;

  while (1) {
    bmp280_read_raw(&temp, &pressure);
    printf("temperature: %d, pressure: %d\n", temp, pressure);
    sleep_ms(750);
  }

#endif
  return 0;
}