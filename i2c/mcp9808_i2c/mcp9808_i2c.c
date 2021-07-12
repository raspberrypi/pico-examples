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


const uint8_t ADDRESS = 0X31;
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

uint16_t UpperByte;
uint16_t LowerByte;
uint16_t Temperature;

nt main() {
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

    mcp9808_init()
}

void mcp9808_init(){
/*
    i2c_start(); // send START command
    i2c_write (ADDRESS & 0xFE); //WRITE Command (see Section 4.1.4 “Address Byte”)
    //also, make sure bit 0 is cleared ‘0’
    i2c_write(0x05); // Write TA Register Address
    i2c_start(); //Repeat START
    i2c_write(ADDRESS | 0x01); // READ Command (see Section 4.1.4 “Address Byte”)
    //also, make sure bit 0 is Set ‘1’
    UpperByte = i2c_read(ACK); // READ 8 bits 
    //and Send ACK bit
    LowerByte = i2c_read(NAK); // READ 8 bits 
    //and Send NAK bit
    i2c_stop(); // send STOP command
    //Convert the temperature data
    //First Check flag bits
    if ((UpperByte & 0x80) == 0x80){ //TA ³ TCRIT
    }
    if ((UpperByte & 0x40) == 0x40){ //TA > TUPPER
    }
    if ((UpperByte & 0x20) == 0x20){ //TA < TLOWER
    }
    UpperByte = UpperByte & 0x1F; //Clear flag bits
    if ((UpperByte & 0x10) == 0x10){ //TA < 0°C
    UpperByte = UpperByte & 0x0F; //Clear SIGN
    Temperature = 256 - (UpperByte x 16 + LowerByte / 16);
    }else //TA ³ 0°C
    Temperature = (UpperByte x 16 + LowerByte / 16);
    //Temperature = Ambient Temperature (°C)
    
    */

    printf("jkfd")
}


