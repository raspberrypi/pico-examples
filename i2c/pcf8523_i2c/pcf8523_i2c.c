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

static void pcf820_write_current_time() {
    // buf[0] is the register to write to
    // buf[1] is the value that will be written to the register
    uint8_t buf[2];

    //Write values for the current time in the array
    //index 0 -> seconds: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 1 -> minutes: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 2 -> hours: bits 4-5 are responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 3 -> day of the month: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 4 -> day of the week: where Sunday = 0x00, Monday = 0x01, Tuesday... ...Saturday = 0x06
    //index 5 -> day of the week: bit 4 is responsible for the ten's digit and bits 0-3 for the unit's digit
    //index 6 -> years: bits 4-7 are responsible for the ten's digit and bits 0-3 for the unit's digit

    uint8_t current_val[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    for (int i = 3; i < 10; ++i){
        buf[0] = i;
        buf[1] = current_val[i - 3];
        i2c_write_blocking(i2c_default, addr, buf, 2, false);
    }
}
 

static void pcf8520_read_raw(uint8_t *buffer) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x03;
    i2c_write_blocking(i2c_default, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, addr, buffer, 7, false);
}
//#endif

void set_alarm(){
    // buf[0] is the register to write to
    // buf[1] is the value that will be written to the register
    uint8_t buf[2];

    // Default value of alarm register is 0x80
    // Set bit 8 of values to 0 to activate that particular alarm
    // Index 0 -> minutes: bits 4-5 are responsible for the ten's digit and bits 0-3 for the unit's digit
    // Index 1 -> hours: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
    // Index 2 -> day of the month: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
    // Index 3 -> day of the week: where Sunday = 0x00, Monday = 0x01, Tuesday... ...Saturday = 0x06

    uint8_t alarm_val[4] = {0x01, 0x80, 0x80, 0x80};
    // Writes alarm values to registers
    for (int i = 10; i < 14; ++i){
        buf[0] = (uint8_t) i;
        buf[1] = alarm_val[i - 10];
        i2c_write_blocking(i2c_default, addr, buf, 2, false);
    }
 
}

void check_alarm(){
    // Checks bit 3 of control register 2 for alarm flags
    uint8_t status[1];
    uint8_t val = 0x01;
    i2c_write_blocking(i2c_default, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, addr, status, 1, false);

    if ((status[0] & 0x08)==0x08){
        printf("ALARM RINGING");
    } else {
        printf("Alarm not triggered yet");
    }

}

void convert_time(int conv_time[7],uint8_t raw_time[7]){
    // Converts raw data into time
    conv_time[0]= (10*(int)((raw_time[0] & 0x70) >> 4)) + ((int)(raw_time[0] & 0x0F));
    conv_time[1]= (10*(int)((raw_time[1] & 0x70) >> 4)) + ((int)(raw_time[1] & 0x0F));
    conv_time[2]= (10*(int)((raw_time[2] & 0x30) >> 4)) + ((int)(raw_time[2] & 0x0F));
    conv_time[3]= (10*(int)((raw_time[3] & 0x30) >> 4)) + ((int)(raw_time[3] & 0x0F));
    conv_time[4]= (int)(raw_time[4] & 0x07);
    conv_time[5]= (10*(int)((raw_time[5] & 0x10) >> 4)) + ((int)(raw_time[5] & 0x0F));
    conv_time[6]= (10*(int)((raw_time[6] & 0xF0) >> 4)) + ((int)(raw_time[6] & 0x0F));
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

    pcf820_write_current_time();
    set_alarm();
    check_alarm();

    uint8_t raw_time[7];
    int real_time[7];
    char days_of_week[7][12] = {"Sunday","Monday","Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    while (1) {
        
        pcf8520_read_raw(raw_time);
        convert_time(real_time, raw_time);

        printf("Time: %02d : %02d : %02d\n",real_time[2],real_time[1],real_time[0]);
        printf("Date: %s %02d / %02d / %02d\n",days_of_week[real_time[4]], real_time[3], real_time[5], real_time[6]);
        check_alarm();
        
        
        sleep_ms(500);

        // Clear terminal 
        printf("\e[1;1H\e[2J");
    }

#endif
    return 0;
}
