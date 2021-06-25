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

// number of calibration registers to be read
#define NUM_CALIB_PARAMS 24

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

struct bmp280_calib_param {
  // temperature params
  uint16_t dig_t1;
  int16_t dig_t2;
  int16_t dig_t3;

  // pressure params
  uint16_t dig_p1;
  int16_t dig_p2;
  int16_t dig_p3;
  int16_t dig_p4;
  int16_t dig_p5;
  int16_t dig_p6;
  int16_t dig_p7;
  int16_t dig_p8;
  int16_t dig_p9;

  // intermediate result for pressure calc
  int32_t t_fine;
};

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

  // store the 20 bit read in a 32 bit signed integer for conversion
  *pressure = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
  *temp = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
}

void bmp280_reset() {
  // reset the device with the power-on-reset procedure
  uint8_t buf[2] = {REG_RESET, 0xB6};
  i2c_write_blocking(i2c_default, (addr & BMP280_WRITE_MODE), buf, 2, false);
}

float bmp280_convert_temp(int32_t *temp, struct bmp280_calib_param *params) {
  // use the 32-bit floating point compensation implementation given in the
  // datasheet
  float var1, var2;
  var1 = (((float)*temp) / 16384.0 - ((float)params->dig_t1) / 1024.0) *
         ((float)params->dig_t2);
  var2 = ((((float)*temp) / 131072.0 - ((float)params->dig_t1) / 8192.0) *
          (((float)*temp) / 131072.0 - ((float)params->dig_t1) / 8192.0)) *
         ((float)params->dig_t3);
  params->t_fine = (int32_t)(var1 + var2);
  return (var1 + var2) / 5120.0;
}

float bmp280_convert_pressure(int32_t *pressure,
                              struct bmp280_calib_param *params) {
  float var1, var2;
  float converted = 0.0;

  var1 = ((float)params->t_fine / 2.0) - 64000.0;
  var2 = var1 * var1 * ((float)params->dig_p6) / 32768.0;
  var2 = var2 + var1 * ((float)params->dig_p5) * 2.0;
  var2 = (var2 / 4.0) + (((float)params->dig_p4) * 65536.0);
  var1 = (((float)params->dig_p3) * var1 * var1 / 524288.0 +
          ((float)params->dig_p2) * var1) /
         524288.0;
  var1 = (1.0 + var1 / 32768.0) * ((float)params->dig_p1);

  // avoid division by 0
  if (var1 < 0 || var1 > 0) {
    converted = 1048576.0 - (float)*pressure;
    converted = (converted - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((float)params->dig_p9) * converted * converted / 2147483648.0;
    var2 = converted * ((float)params->dig_p8) / 32768.0;

    converted += (var1 + var2 + ((float)params->dig_p7)) / 16.0;
    // results in kPa
    return converted / 1000.0;
  } else {
    return 0.0;
  }
}

void bmp280_get_calib_params(struct bmp280_calib_param *params) {
  // raw temp and pressure values need to be calibrated according to
  // parameters generated during the manufacturing of the sensor
  // there are 3 temperature params, and 9 pressure params, each with a LSB
  // and MSB register, so we read from 24 registers

  uint8_t buf[NUM_CALIB_PARAMS] = {0};
  i2c_write_blocking(i2c_default, (addr & BMP280_WRITE_MODE), &REG_DIG_T1_LSB,
                     1, true);  // true to keep master control of bus
  // read in one go as register addresses auto-increment
  i2c_read_blocking(i2c_default, (addr & BMP280_READ_MODE), buf,
                    NUM_CALIB_PARAMS, false);  // false, we're done reading

  // store these in a struct for later use
  params->dig_t1 = (uint16_t)(buf[1] << 8) | buf[0];
  params->dig_t2 = (int16_t)(buf[3] << 8) | buf[2];
  params->dig_t3 = (int16_t)(buf[5] << 8) | buf[4];

  params->dig_p1 = (uint16_t)(buf[7] << 8) | buf[6];
  params->dig_p2 = (int16_t)(buf[9] << 8) | buf[8];
  params->dig_p3 = (int16_t)(buf[11] << 8) | buf[10];
  params->dig_p4 = (int16_t)(buf[13] << 8) | buf[12];
  params->dig_p5 = (int16_t)(buf[15] << 8) | buf[14];
  params->dig_p6 = (int16_t)(buf[17] << 8) | buf[16];
  params->dig_p7 = (int16_t)(buf[19] << 8) | buf[18];
  params->dig_p8 = (int16_t)(buf[21] << 8) | buf[20];
  params->dig_p9 = (int16_t)(buf[23] << 8) | buf[22];
}

#endif

int main() {
  stdio_init_all();

  // useful information for picotool
  bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN,
                             GPIO_FUNC_I2C));
  bi_decl(
      bi_program_description("BMP280 I2C example for the Raspberry Pi Pico"));

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || \
    !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c/bmp280_i2c example requires a board with I2C pins
  puts("Default I2C pins were not defined");
#else
  printf(
      "Hello, BMP280! Reading temperaure and pressure values from sensor...\n");

  // I2C is "open drain", pull ups to keep signal high when no data is being sent
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

  // configure BMP280
  bmp280_init();

  // retrieve fixed compensation params
  struct bmp280_calib_param params;
  bmp280_get_calib_params(&params);

  int32_t temp;
  int32_t pressure;

  while (1) {
    bmp280_read_raw(&temp, &pressure);
    float converted_temp = bmp280_convert_temp(&temp, &params);
    float converted_pressure = bmp280_convert_pressure(&pressure, &params);
    printf("Temperature: %.2f °C  Pressure: %.3f kPa\n", converted_temp,
           converted_pressure);
    // poll every 750ms as data refreshes every 500ms
    sleep_ms(750);
  }

#endif
  return 0;
}