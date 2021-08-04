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

/* Example code to talk to a PCF8520 MEMS accelerometer and gyroscope

   This is taking to simple approach of simply reading registers. It's perfectly
   possible to link up an interrupt line and set things up to read from the
   inbuilt FIFO to make it more useful.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefor I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is 4 (pin 6)) -> SDA on PCF8520 board
   GPIO PICO_DEFAULT_I2C_SCK_PIN (On Pico this is 5 (pin 7)) -> SCL on PCF8520 board
   3.3v (pin 36) -> VCC on PCF8520 board
   GND (pin 38)  -> GND on PCF8520 board
*/

// By default these devices  are on bus address 0x68
static int addr = 0x68;

//#ifdef i2c_default
static void pcf8520_reset() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x00, 0x58};
    i2c_write_blocking(i2c_default, addr, buf, 2, false);
}

static uint8_t pcf8520_read_raw() {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x03;
    i2c_write_blocking(i2c_default, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, addr, buffer, 6, false);
    printf("%d\n",buffer[0]);
    printf("%d\n",buffer[1]);
    printf("%d\n",buffer[2]);
    printf("%d\n",buffer[3]);

    return buffer;
}
//#endif

int convert_time(uint8_t raw_time[]){
    int conv_time[6];
    conv_time[0]= (10*((int)(raw_time[0]>>4)));
    printf("infucnt");
    conv_time[1]= (10*((int)(raw_time[0]>>4)));
    conv_time[2]= (10*((int)(raw_time[0]>>4)));
    conv_time[3]= (10*((int)(raw_time[0]>>4)));
    conv_time[4]= (10*((int)(raw_time[0]>>4)));
    conv_time[5]= (10*((int)(raw_time[0]>>4)));

    return conv_time;
}

int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/pcf8520_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else
    printf("Hello, PCF8520! Reading raw data from registers...\n");

    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    pcf8520_reset();
    uint8_t raw_time[];
    int real_time[];

    while (1) {
        
        raw_time = pcf8520_read_raw();
        real_time = convert_time(raw_time);
        sleep_ms(1000);
    }

#endif
    return 0;
}
