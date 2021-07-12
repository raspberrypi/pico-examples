/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

 /* Example code to talk to an MPL3115A2 altimeter sensor via I2C

    See accompanying documentation in README.adoc or the C++ SDK booklet.

    Connections on Raspberry Pi Pico board, other boards may vary.

    GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is 4 (pin 6)) -> SDA on MPL3115A2 board
    GPIO PICO_DEFAULT_I2C_SCK_PIN (On Pico this is 5 (pin 7)) -> SCL on MPL3115A2 board
    GPIO 16 -> INT1 on MPL3115A2 board
    GPIO 17 -> INT2 on MPL3115A2 board
    3.3v (pin 36) -> VCC on MPL3115A2 board
    GND (pin 38)  -> GND on MPL3115A2 board
 */

#define ADDR _u(0x60)

// following definitions only valid for F_MODE > 0 (ie. if FIFO enabled)
#define MPL3115A2_F_DATA _u(0x01)
#define MPL3115A2_F_STATUS _u(0x00)
#define MPL3115A2_F_SETUP _u(0x0F)
#define MPL3115A2_CTRLREG1 _u(0x26)
#define MPL3115A2_CTRLREG2 _u(0x27)
#define MPL3115A2_CTRLREG3 _u(0x28)
#define MPL3115A2_CTRLREG4 _u(0x29)
#define MPL3115A2_CTRLREG5 _u(0x2A)
#define MPL3115A2_OFF_P _u(0x2B)
#define MPL3115A2_OFF_T _u(0x2C)
#define MPL3115A2_OFF_H _u(0x2D)

#define READ_BIT _u(0x01)
#define WRITE_BIT _u(0x00)

#ifdef i2c_default
void mpl3115a2_read_fifo(uint8_t buf[]) {
    // drains the 160 byte FIFO
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, (uint8_t *) MPL3115A2_F_DATA, 1, true);
    i2c_read_blocking(i2c_default, ADDR | READ_BIT, &buf, 160, false);
}

uint8_t mpl3115a2_read_reg(uint8_t reg) {
    uint8_t read;
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, &reg, 1, true);
    i2c_read_blocking(i2c_default, ADDR | READ_BIT, &read, 1, false);
}

void mpl3115a2_init(){
    // set as altimeter with oversampling ratio of 128
    uint8_t buf[] = {MPL3115A2_CTRLREG1, 0xB8};
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, buf, 2, false);

    // set data refresh every 2 seconds, 0 next bits as we're not using those interrupts
    buf[0] = MPL3115A2_CTRLREG2, buf[1] = 0x01;
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, buf, 2, false);    

    // set both interrupts pins to active low and enable internal pullps
    buf[0] = MPL3115A2_CTRLREG3, buf[1] = 0x00;
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, buf, 2, false);

    // enable FIFO and temperature change interrupts
    buf[0] = MPL3115A2_CTRLREG4, buf[1] = 0x41;
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, buf, 2, false);

    // tie FIFO interrupt to pin INT1, temp change to pin INT2
    buf[0] = MPL3115A2_CTRLREG5, buf[1] = 0x40;
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, buf, 2, false);

    // set p, t and h offsets here if needed
    // eg. 2's complement number: 0xFF subtracts 1 meter
    //buf[0] = MPL3115A2_OFF_H, buf[1] = 0xFF;
    //i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, buf, 2, false);

    // do not accept more data on FIFO overflow
    buf[0] = MPL3115A2_F_SETUP, buf[1] = 0x80;
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, buf, 2, false);

    // set device active
    buf[0] = MPL3115A2_CTRLREG3, buf[1] = 0xB9;
    i2c_write_blocking(i2c_default, ADDR | WRITE_BIT, buf, 2, false);    
}
#endif

// if we had enabled more than 2 interrupts on same pin, then we should read
// INT_SOURCE reg to find out which interrupt triggered 

// to clear fifo interrupt bits, we must read from the status register

int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c / mpl3115a2_i2c example requires a board with I2C pins
        puts("Default I2C pins were not defined");
#else
    printf("Hello, MPL3115A2! Reading altitude from sensors...\n");

    // use default I2C0 at 400kHz, I2C is active low
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // add program information for picotool
    bi_decl(bi_program_name("Example in the pico-examples library for the MPL3115A2 altimeter"));
    bi_decl(bi_1pin_with_name(16, "Interrupt pin 1"));
    bi_decl(bi_1pin_with_name(17, "Interrupt pin 2"));
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
#endif
    return 0;
}
