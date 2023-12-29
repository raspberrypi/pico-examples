/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdio>
#include "pico/stdlib.h"
#include "AM2302-Sensor.hpp"

const uint LED_PIN = 13U;

// DHT Pin
const uint DHT_PIN = 15;

// create am2302 object
AM2302::AM2302_Sensor am2302{DHT_PIN};

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    printf("\n\n === Pi Pico C++ Example - Read AM2302-Sensor === \n\n");
    // setup pin and do a sensor check
    if (!am2302.begin()) {
        while(true) {
           printf("ERROR: during sensor check! Pleae check sensor connectivity!\n");
           sleep_ms(10000);
        }
    }
    sleep_ms(3000);
    while (true) {

        gpio_put(LED_PIN, 1);
        int8_t status = am2302.read();

    gpio_put(LED_PIN, 0);

        printf("Sensor-status: %d\n", status);

        if (status == AM2302::AM2302_READ_OK) {
            printf("Humidity = %.1f %%\tTemperature = %.1f degC\n",
                am2302.get_Humidity(), am2302.get_Temperature());
        }
        else {
            if (status == AM2302::AM2302_ERROR_CHECKSUM) {
                printf("ERROR: Checksum not valid\n");
            }
            else if (status == AM2302::AM2302_ERROR_TIMEOUT) {
                printf("ERROR: Timeout overflow\n");
            }
            else if (status == AM2302::AM2302_ERROR_READ_FREQ) {
                printf("ERROR: Read frequency too high!\n");
            }
        }
        printf("\n\n");
        sleep_ms(2000);
    }
}
