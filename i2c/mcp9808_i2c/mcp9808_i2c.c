/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

/* Example code to talk to a MCP9808 ±0.5°C Digital temperature Sensor
   
   This reads and writes to registers on the board. 

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is GP4 (physical pin 6)) -> SDA on MCP9808 board
   GPIO PICO_DEFAULT_I2C_SCK_PIN (On Pico this is GP5 (physcial pin 7)) -> SCL on MCP9808 board
   Vsys (physical pin 39) -> VDD on MCP9808 board
   GND (physical pin 38)  -> GND on MCP9808 board

*/
//The bus address is determined by the state of pins A0, A1 and A2 on the MCP9808 board
static uint8_t ADDRESS = 0x18;

//hardware registers

const uint8_t REG_POINTER = 0x00;
const uint8_t REG_CONFIG = 0x01;
const uint8_t REG_TEMP_UPPER = 0x02;
const uint8_t REG_TEMP_LOWER = 0x03;
const uint8_t REG_TEMP_CRIT = 0x04;
const uint8_t REG_TEMP_AMB = 0x05;
const uint8_t REG_RESOLUTION = 0x08;


void mcp9808_check_limits(uint8_t upper_byte) {

    // Check flags and raise alerts accordingly 
    if ((upper_byte & 0x40) == 0x40) { //TA > TUPPER
        printf("Temperature is above the upper temperature limit.\n");
    }
    if ((upper_byte & 0x20) == 0x20) { //TA < TLOWER
        printf("Temperature is below the lower temperature limit.\n");
    }
    if ((upper_byte & 0x80) == 0x80) { //TA > TCRIT
        printf("Temperature is above the critical temperature limit.\n");
    }
}

float mcp9808_convert_temp(uint8_t upper_byte, uint8_t lower_byte) {

    float temperature;


    //Check if TA <= 0°C and convert to denary accordingly
    if ((upper_byte & 0x10) == 0x10) {
        upper_byte = upper_byte & 0x0F;
        temperature = 256 - (((float) upper_byte * 16) + ((float) lower_byte / 16));
    } else {
        temperature = (((float) upper_byte * 16) + ((float) lower_byte / 16));

    }
    return temperature;
}

#ifdef i2c_default
void mcp9808_set_limits() {

    //Set an upper limit of 30°C for the temperature
    uint8_t upper_temp_msb = 0x01;
    uint8_t upper_temp_lsb = 0xE0;

    //Set a lower limit of 20°C for the temperature
    uint8_t lower_temp_msb = 0x01;
    uint8_t lower_temp_lsb = 0x40;

    //Set a critical limit of 40°C for the temperature
    uint8_t crit_temp_msb = 0x02;
    uint8_t crit_temp_lsb = 0x80;

    uint8_t buf[3];
    buf[0] = REG_TEMP_UPPER;
    buf[1] = upper_temp_msb;
    buf[2] = upper_temp_lsb;
    i2c_write_blocking(i2c_default, ADDRESS, buf, 3, false);

    buf[0] = REG_TEMP_LOWER;
    buf[1] = lower_temp_msb;
    buf[2] = lower_temp_lsb;
    i2c_write_blocking(i2c_default, ADDRESS, buf, 3, false);

    buf[0] = REG_TEMP_CRIT;
    buf[1] = crit_temp_msb;
    buf[2] = crit_temp_lsb;;
    i2c_write_blocking(i2c_default, ADDRESS, buf, 3, false);
}
#endif

int main() {

    stdio_init_all();

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c/mcp9808_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else
    printf("Hello, MCP9808! Reading raw data from registers...\n");

    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    mcp9808_set_limits();

    uint8_t buf[2];
    uint16_t upper_byte;
    uint16_t lower_byte;

    float temperature;

    while (1) {
        // Start reading ambient temperature register for 2 bytes
        i2c_write_blocking(i2c_default, ADDRESS, &REG_TEMP_AMB, 1, true);
        i2c_read_blocking(i2c_default, ADDRESS, buf, 2, false);

        upper_byte = buf[0];
        lower_byte = buf[1];

        //isolates limit flags in upper byte
        mcp9808_check_limits(upper_byte & 0xE0);

        //clears flag bits in upper byte
        temperature = mcp9808_convert_temp(upper_byte & 0x1F, lower_byte);
        printf("Ambient temperature: %.4f°C\n", temperature);

        sleep_ms(1000);
    }
#endif
}
