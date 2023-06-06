/**
 * Copyright (c) 2023 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
**/

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "onewire_library.h"    // onewire library functions
#include "ow_rom.h"             // onewire ROM command codes
#include "ds18b20.h"            // ds18b20 function codes

// Demonstrates the PIO onewire driver by taking readings from a set of
// ds18b20 1-wire temperature sensors.

int main() {
    stdio_init_all();

    PIO pio = pio0;
    uint gpio = 15;

    OW ow;
    uint offset;
    // add the program to the PIO shared address space
    if (pio_can_add_program (pio, &onewire_program)) {
        offset = pio_add_program (pio, &onewire_program);

        // claim a state machine and initialise a driver instance
        if (ow_init (&ow, pio, offset, gpio)) {

            // find and display 64-bit device addresses
            int maxdevs = 10;
            uint64_t romcode[maxdevs];
            int num_devs = ow_romsearch (&ow, romcode, maxdevs, OW_SEARCH_ROM);

            printf("Found %d devices\n", num_devs);      
            for (int i = 0; i < num_devs; i += 1) {
                printf("\t%d: 0x%llx\n", i, romcode[i]);
            }
            putchar ('\n');

            while (num_devs > 0) {
                // start temperature conversion in parallel on all devices
                // (see ds18b20 datasheet)
                ow_reset (&ow);
                ow_send (&ow, OW_SKIP_ROM);
                ow_send (&ow, DS18B20_CONVERT_T);

                // wait for the conversions to finish
                while (ow_read(&ow) == 0);

                // read the result from each device
                for (int i = 0; i < num_devs; i += 1) {
                    ow_reset (&ow);
                    ow_send (&ow, OW_MATCH_ROM);
                    for (int b = 0; b < 64; b += 8) {
                        ow_send (&ow, romcode[i] >> b);
                    }
                    ow_send (&ow, DS18B20_READ_SCRATCHPAD);
                    int16_t temp = 0;
                    temp = ow_read (&ow) | (ow_read (&ow) << 8);
                    printf ("\t%d: %f", i, temp / 16.0);
                }
                putchar ('\n');
            }
            
        } else {
            puts ("could not initialise the driver");
        }
    } else {
        puts ("could not add the program");
    }

    while(true);
}