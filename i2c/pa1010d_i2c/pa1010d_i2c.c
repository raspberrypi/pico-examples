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

/* Example code to talk to a MPU6050 MEMS accelerometer and gyroscope

   This is taking to simple approach of simply reading registers. It's perfectly
   possible to link up an interrupt line and set things up to read from the
   inbuilt FIFO to make it more useful.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefor I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is 4 (pin 6)) -> SDA on MPU6050 board
   GPIO PICO_DEFAULT_I2C_SCK_PIN (On Pico this is 5 (pin 7)) -> SCL on MPU6050 board
   3.3v (pin 36) -> VCC on MPU6050 board
   GND (pin 38)  -> GND on MPU6050 board
*/

// By default these devices  are on bus address 0x68
static int addr = 0x10;

#ifdef i2c_default


static void write_command(char command[]){
    
    int length2 = sizeof(command);
    uint8_t numcommand[length2];
    for (int i = 0; i <length2;++i){
        numcommand[i] = command[i];
    }
    int length1 = sizeof(numcommand);

    printf("%d.....%d",length1, length2);
    i2c_write_blocking(i2c_default, addr, numcommand, length2, true);

    length2 = sizeof(numcommand);
    char numcommand2[length2-1];
    for (int i = 0; i <length2;++i){
        numcommand2[i] = numcommand[i];
    }
    length1 = sizeof(numcommand2);

    //printf("%d.....%d\n",length1, length2);
    //printf("%c,,,,%c\n",numcommand[0],numcommand[1]);
    printf("%s",numcommand2);
    
}

static void mpu6050_read_raw() {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.
    uint8_t buffer[90];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    //i2c_write_blocking(i2c_default, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, addr, buffer, 90, false);

    int length2 = sizeof(buffer);
    char numcommand[length2];
    for (int i = 0; i <length2;++i){
        numcommand[i] = buffer[i];
    }
    //int length1 = sizeof(numcommand);

    //printf("%d.....%d\n",length1, length2);
    //printf("%c,,,,%c\n",numcommand[0],numcommand[1]);
    printf("%s\n",numcommand);

}
#endif


int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/mpu6050_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else
    

    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    printf("Hello, MPU6050! Reading raw data from registers...\n");
    write_command("$PMTK314,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*34\r\n");
    //write_command("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");

    while (1) {
        mpu6050_read_raw();
        sleep_ms(1000);
    }

#endif
    return 0;
}
