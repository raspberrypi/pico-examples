/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
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

 // 7-bit address
#define ADDR 0x60
#define INT1_PIN _u(16)
#define INT2_PIN _u(17)

// following definitions only valid for F_MODE > 0 (ie. if FIFO enabled)
#define MPL3115A2_F_DATA _u(0x01)
#define MPL3115A2_F_STATUS _u(0x00)
#define MPL3115A2_F_SETUP _u(0x0F)
#define MPL3115A2_INT_SOURCE _u(0x12)
#define MPL3115A2_CTRLREG1 _u(0x26)
#define MPL3115A2_CTRLREG2 _u(0x27)
#define MPL3115A2_CTRLREG3 _u(0x28)
#define MPL3115A2_CTRLREG4 _u(0x29)
#define MPL3115A2_CTRLREG5 _u(0x2A)
#define MPL3115A2_OFF_P _u(0x2B)
#define MPL3115A2_OFF_T _u(0x2C)
#define MPL3115A2_OFF_H _u(0x2D)

#define MPL3115A2_FIFO_DISABLED _u(0x00)
#define MPL3115A2_FIFO_STOP_ON_OVERFLOW _u(0x80)
#define MPL3115A2_FIFO_SIZE 32
#define MPL3115A2_DATA_BATCH_SIZE 5
#define MPL3115A2_ALTITUDE_NUM_REGS 3
#define MPL3115A2_NUM_FRAC_BITS 4

#define PARAM_ASSERTIONS_ENABLE_I2C 1

volatile uint8_t fifo_data[MPL3115A2_FIFO_SIZE * MPL3115A2_DATA_BATCH_SIZE];
volatile bool has_new_data = false;

struct mpl3115a2_data_t {
    // Q8.4 fixed point
    int16_t temperature;
    // Q16.4 fixed-point
    int32_t altitude;
};

void copy_to_vbuf(uint8_t buf1[], volatile uint8_t buf2[], int buflen){
    for (size_t i = 0; i < buflen; i++) {
        buf2[i] = buf1[i];
    }
}

#ifdef i2c_default
void mpl3115a2_read_fifo(volatile uint8_t fifo_buf[]) {
    // drains the 160 byte FIFO
    uint8_t reg = MPL3115A2_F_DATA;
    uint8_t buf[MPL3115A2_FIFO_SIZE * MPL3115A2_DATA_BATCH_SIZE];
    i2c_write_blocking(i2c_default, ADDR, &reg, 1, true);
    // burst read 160 bytes from fifo
    i2c_read_blocking(i2c_default, ADDR, buf, MPL3115A2_FIFO_SIZE * MPL3115A2_DATA_BATCH_SIZE, false);
    copy_to_vbuf(buf, fifo_buf, MPL3115A2_FIFO_SIZE * MPL3115A2_DATA_BATCH_SIZE);
}

uint8_t mpl3115a2_read_reg(uint8_t reg) {
    uint8_t read;
    i2c_write_blocking(i2c_default, ADDR, &reg, 1, true); // keep control of bus
    i2c_read_blocking(i2c_default, ADDR, &read, 1, false);
    return read;
}

void mpl3115a2_init() {
    // set as altimeter with oversampling ratio of 128
    uint8_t buf[] = { MPL3115A2_CTRLREG1, 0xB8 };
    i2c_write_blocking(i2c_default, ADDR, buf, 2, false);

    // set data refresh every 2 seconds, 0 next bits as we're not using those interrupts
    buf[0] = MPL3115A2_CTRLREG2, buf[1] = 0x00;
    i2c_write_blocking(i2c_default, ADDR, buf, 2, false);

    // set both interrupts pins to active low and enable internal pullups
    buf[0] = MPL3115A2_CTRLREG3, buf[1] = 0x00;
    i2c_write_blocking(i2c_default, ADDR, buf, 2, false);

    // enable FIFO and temperature change interrupts
    buf[0] = MPL3115A2_CTRLREG4, buf[1] = 0x41;
    i2c_write_blocking(i2c_default, ADDR, buf, 2, false);

    // tie FIFO interrupt to pin INT1, temp change to pin INT2
    buf[0] = MPL3115A2_CTRLREG5, buf[1] = 0x40;
    i2c_write_blocking(i2c_default, ADDR, buf, 2, false);

    // set p, t and h offsets here if needed
    // eg. 2's complement number: 0xFF subtracts 1 meter
    //buf[0] = MPL3115A2_OFF_H, buf[1] = 0xFF;
    //i2c_write_blocking(i2c_default, ADDR, buf, 2, false);

    // do not accept more data on FIFO overflow
    buf[0] = MPL3115A2_F_SETUP, buf[1] = MPL3115A2_FIFO_STOP_ON_OVERFLOW;
    i2c_write_blocking(i2c_default, ADDR, buf, 2, false);

    // set device active
    buf[0] = MPL3115A2_CTRLREG1, buf[1] = 0xB9;
    i2c_write_blocking(i2c_default, ADDR, buf, 2, false);
}

void gpio_callback(uint gpio, uint32_t events) {
    // if we had enabled more than 2 interrupts on same pin, then we should read
    // INT_SOURCE reg to find out which interrupt triggered

    if (gpio == INT1_PIN) {
        // FIFO overflow interrupt
        // watermark bits set to 0 in F_SETUP reg, so only possible event is an overflow
        // otherwise, we would read F_STATUS to confirm it was an overflow

        // drain the fifo
        mpl3115a2_read_fifo(fifo_data);
        printf("%d %d %d", fifo_data[0], fifo_data[1], fifo_data[2] >> 4);
        // read status register to clear interrupt bit
        mpl3115a2_read_reg(MPL3115A2_F_STATUS);
        has_new_data = true;
    } else if (gpio == INT2_PIN) {
        // temp change interrupt
        //printf("the temperature changed!\n");
    }
}
#endif

void mpl3115a2_convert_fifo_batch(uint8_t start, volatile uint8_t buf[], struct mpl3115a2_data_t* data) {
    // convert a batch of fifo data into temperature and altitude data

    // 3 altitude registers: MSB (8 bits), CSB (8 bits) and LSB (4 bits, starting from MSB)
    // first two are integer bits (2's complement) and LSB is fractional bits -> makes 20 bit signed integer
    int32_t h = 0;
    if ((buf[start] & 0x80) == 0x80) {
        // signed bit is 1, so invert and add 1
        h = ~(buf[start] << 8 | buf[start + 1]) + 1;
        h = h << 4;
        h = h * (1 << MPL3115A2_NUM_FRAC_BITS) | buf[start + 2] >> 4;
        h *= -1;
    } else {
        h = buf[start] << 8 | buf[start + 1];
        h = h << 4;
        h = h * (1 << MPL3115A2_NUM_FRAC_BITS) | buf[start + 2] >> 4;
    }
    data->altitude = h;

    int16_t t = 0;
    if ((buf[start + 3] & 0x80) == 0x80) {
        // signed bit is 1, so invert and add 1
        t = ~buf[start + 3] + 1;
        t = t << 8;
        t = t * (1 << MPL3115A2_NUM_FRAC_BITS) | buf[start + 4] >> 4;
        t *= -1;
    } else {
        t = (buf[start + 3] << 8) * (1 << MPL3115A2_NUM_FRAC_BITS) | buf[start + 4] >> 4;
    }
    data->temperature = t;
}

// to clear fifo interrupt bits, we must read from the status register
int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c / mpl3115a2_i2c example requires a board with I2C pins
        puts("Default I2C pins were not defined");
#else
    printf("Hello, MPL3115A2. Waiting for something to interrupt me!...\n");

    // use default I2C0 at 400kHz, I2C is active low
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    gpio_init(INT1_PIN);
    gpio_init(INT2_PIN);

    // add program information for picotool
    bi_decl(bi_program_name("Example in the pico-examples library for the MPL3115A2 altimeter"));
    bi_decl(bi_1pin_with_name(16, "Interrupt pin 1"));
    bi_decl(bi_1pin_with_name(17, "Interrupt pin 2"));
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    mpl3115a2_init();

    gpio_set_irq_enabled_with_callback(INT1_PIN, GPIO_IRQ_LEVEL_LOW, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(INT2_PIN, GPIO_IRQ_LEVEL_LOW, true, &gpio_callback);

    while (1) {
        // as interrupt data comes in, let's print the 32 sample average
        if (has_new_data) {
            struct mpl3115a2_data_t data;
            for (int i = 0; i < MPL3115A2_FIFO_SIZE; i++) {
                mpl3115a2_convert_fifo_batch(i * MPL3115A2_DATA_BATCH_SIZE, fifo_data, &data);
                printf("t: %f C, h: %f m\n", data.temperature / (1 << MPL3115A2_NUM_FRAC_BITS), data.altitude / (1 << MPL3115A2_NUM_FRAC_BITS));
            }

            has_new_data = false;
        }
        sleep_ms(10);
    };

#endif
    return 0;
}
