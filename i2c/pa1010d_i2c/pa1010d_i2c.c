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
#include "string.h"

/* Example code to talk to a PA1010D Mini GPS module.

   This example reads the Recommended Minimum Specific GNSS Sentence, which includes basic location and time data, each second, formats and displays it.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is 4 (physical pin 6)) -> SDA on PA1010D board
   GPIO PICO_DEFAULT_I2C_SCK_PIN (On Pico this is 5 (physical pin 7)) -> SCL on PA1010D board
   3.3v (physical pin 36) -> VCC on PA1010D board
   GND (physical pin 38)  -> GND on PA1010D board
*/

const int addr = 0x10;
#define MAX_READ 250

#ifdef i2c_default

void pa1010d_write_command(const char command[], int com_length) {
    // Convert character array to bytes for writing
    uint8_t int_command[com_length];

    for (int i = 0; i < com_length; ++i) {
        int_command[i] = command[i];
        i2c_write_blocking(i2c_default, addr, &int_command[i], 1, true);
    }
}

void pa1010d_parse_string(char output[], char protocol[]) {
    // Finds location of protocol message in output
    char *com_index = strstr(output, protocol);
    int p = com_index - output;

    // Splits components of output sentence into array
#define NO_OF_FIELDS 14
#define MAX_LEN 15

    int n = 0;
    int m = 0;

    char gps_data[NO_OF_FIELDS][MAX_LEN];
    memset(gps_data, 0, sizeof(gps_data));

    bool complete = false;
    while (output[p] != '$' && n < MAX_LEN && complete == false) {
        if (output[p] == ',' || output[p] == '*') {
            n += 1;
            m = 0;
        } else {
            gps_data[n][m] = output[p];
            // Checks if sentence is complete
            if (m < NO_OF_FIELDS) {
                m++;
            } else {
                complete = true;
            }
        }
        p++;
    }

    // Displays GNRMC data
    // Similarly, additional if statements can be used to add more protocols 
    if (strcmp(protocol, "GNRMC") == 0) {
        printf("Protcol:%s\n", gps_data[0]);
        printf("UTC Time: %s\n", gps_data[1]);
        printf("Status: %s\n", gps_data[2][0] == 'V' ? "Data invalid. GPS fix not found." : "Data Valid");
        printf("Latitude: %s\n", gps_data[3]);
        printf("N/S indicator: %s\n", gps_data[4]);
        printf("Longitude: %s\n", gps_data[5]);
        printf("E/W indicator: %s\n", gps_data[6]);
        printf("Speed over ground: %s\n", gps_data[7]);
        printf("Course over ground: %s\n", gps_data[8]);
        printf("Date: %c%c/%c%c/%c%c\n", gps_data[9][0], gps_data[9][1], gps_data[9][2], gps_data[9][3], gps_data[9][4],
               gps_data[9][5]);
        printf("Magnetic Variation: %s\n", gps_data[10]);
        printf("E/W degree indicator: %s\n", gps_data[11]);
        printf("Mode: %s\n", gps_data[12]);
        printf("Checksum: %c%c\n", gps_data[13][0], gps_data[13][1]);
    }
}

void pa1010d_read_raw(char numcommand[]) {
    uint8_t buffer[MAX_READ];

    int i = 0;
    bool complete = false;

    i2c_read_blocking(i2c_default, addr, buffer, MAX_READ, false);

    // Convert bytes to characters
    while (i < MAX_READ && complete == false) {
        numcommand[i] = buffer[i];
        // Stop converting at end of message 
        if (buffer[i] == 10 && buffer[i + 1] == 10) {
            complete = true;
        }
        i++;
    }
}

#endif

int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c/mpu6050_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else

    char numcommand[MAX_READ];

    // Decide which protocols you would like to retrieve data from
    char init_command[] = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";

    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    printf("Hello, PA1010D! Reading raw data from module...\n");

    pa1010d_write_command(init_command, sizeof(init_command));

    while (1) {
        // Clear array
        memset(numcommand, 0, MAX_READ);
        // Read and re-format
        pa1010d_read_raw(numcommand);
        pa1010d_parse_string(numcommand, "GNRMC");

        // Wait for data to refresh
        sleep_ms(1000);

        // Clear terminal 
        printf("\033[1;1H\033[2J");
    }
#endif
}
