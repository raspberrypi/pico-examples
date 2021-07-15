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

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is 4 (pin 6)) -> SDA on MCP9808 board
   GPIO PICO_DEFAULT_I2C_SCK_PIN (On Pico this is 5 (pin 7)) -> SCL on MCP9808 board
   Vsys (pin 39) -> VDD on MCP9808 board
   GND (pin 38)  -> GND on MCP9808 board

*/
//The bus address is determined by the state of pins A0, A1 and A2 on the MCP9808 board
static uint8_t ADDRESS = 0X18;

const uint8_t WRITE_MODE = 0XFE;
const uint8_t READ_MODE = 0XFF;


//hardware registers

const uint8_t REG_POINTER = 0X00;
const uint8_t REG_CONFIG = 0X01;
const uint8_t REG_TEMP_UPPER = 0X02;
const uint8_t REG_TEMP_LOWER = 0X03;
const uint8_t REG_TEMP_CRIT = 0X04;
const uint8_t REG_TEMP_AMB = 0X05;
const uint8_t REG_MAN_ID = 0X06;
const uint8_t REG_DEV_ID = 0X07;
const uint8_t REG_RESOLUTION = 0X08;


void mcp9808_check_limits(uint8_t upper_byte) {

    // Check flags and raise alerts accordingly 
    if ((upper_byte & 0x40) == 0x40){ //TA > TUPPER
        printf("Temperature is above the upper temperature limit.\n");
    }
    if ((upper_byte & 0x20) == 0x20) { //TA < TLOWER
        printf("Temperature is below the lower temperature limit.\n");
    }
    if ((upper_byte & 0x80) == 0x80) { //TA < TCRIT
        printf("Temperature is above the critical temperature limit.\n");
    }
}

uint16_t mcp9808_convert_temp(uint8_t buf[]){

    uint16_t upper_byte;
    uint16_t lower_byte;
    uint16_t temperature;

    upper_byte = buf[0];
    lower_byte = buf[1];

    mcp9808_check_limits(upper_byte);

    //Clear flag bits
    upper_byte = upper_byte & 0x1F; 

    //Check if TA <= 0°C and convert to denary accordingly
    if ((upper_byte & 0x10) == 0x10) { 
        upper_byte = upper_byte & 0x0F; 
        temperature = 256 - ((upper_byte * 16) + (lower_byte / 16));
    }
    else {
        temperature = ((upper_byte * 16) + (lower_byte / 16));
    }
    return temperature;
}


void mcp9808_read_temp(){  

    uint8_t buf[2];
    uint16_t temperature;

    while (1) {
    // Start reading ambient temperature register for 2 bytes
    i2c_write_blocking (i2c_default, 0x18 & WRITE_MODE ,&REG_TEMP_AMB,1,true);
    i2c_read_blocking(i2c_default, 0x18 & READ_MODE, buf, 2, false);

    temperature = mcp9808_convert_temp(buf);
    printf("Ambient temperature: %d\n", temperature);

    sleep_ms(1000); 
    }   
}


void mcp9808_set_limits(){
    
    //Set an upper limit of 30°C for the temperature
    uint8_t upper_temp_msb = 0x01;
    uint8_t upper_temp_lsb = 0xE0;

    //Set an lower limit of 30°C for the temperature
    uint8_t lower_temp_msb = 0x01;
    uint8_t lower_temp_lsb = 0x40;

    //Set an critical limit of 30°C for the temperature
    uint8_t crit_temp_msb = 0x02;
    uint8_t crit_temp_lsb = 0x80;

    uint8_t buf[3];
    buf[0] = REG_TEMP_UPPER;
    buf[1] = upper_temp_msb;
    buf[2] = upper_temp_lsb; 
    i2c_write_blocking (i2c_default, 0x18, buf, 3, false);

    buf[0] = REG_TEMP_LOWER;
    buf[1] = lower_temp_msb;
    buf[2] = lower_temp_lsb; 
    i2c_write_blocking (i2c_default, 0x18, buf, 3, false);

    buf[0] = REG_TEMP_CRIT;
    buf[1] = crit_temp_msb;
    buf[2] = crit_temp_lsb; ;
    i2c_write_blocking (i2c_default, 0x18, buf, 3, false);
}

void mcp9808_init(){

    //set address (where am i sending it to?? sort this out)
    i2c_write_blocking(i2c_default, 0x32 & WRITE_MODE, &ADDRESS, 1, true);

    mcp9808_set_limits();

    mcp9808_read_temp();
}

int main() {

    stdio_init_all();

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/mpu6050_i2c example requires a board with I2C pins
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
#endif

    mcp9808_init();
}
