/**
 * Author:  Frank Häfele
 *
 * Date:    25.04.2024
 * 
 * Objective: Read one or multiple AM2302-Sensors
 */

#include <cstdio>
#include "pico/stdlib.h"
#include "AM2302-Sensor.hpp"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

// DHT Pin
const uint8_t SIZE = 3U;
const uint8_t PINS[SIZE] = {13U, 14U, 15U};

// create am2302 object
AM2302::AM2302_Sensor am2302[SIZE] = {
    AM2302::AM2302_Sensor{PINS[0]},
    AM2302::AM2302_Sensor{PINS[1]},
    AM2302::AM2302_Sensor{PINS[2]}
};

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    printf("\n\n === Pi Pico C++ Example - Read AM2302-Sensor === \n\n");
    // setup pin and do a sensor check
    
    for (size_t i = 0; i < SIZE; ++i) {
        printf("Sensor available : %d\n", am2302[i].begin());
    }
    printf("\n");
    sleep_ms(3000);
    while (true) {

        gpio_put(LED_PIN, 1);
        printf("\n\tStatus        :");
        for (size_t i = 0; i < SIZE; ++i) {
            printf("\t%s", am2302[i].get_sensorState(am2302[i].read()));
        }
        printf("\n\tTemperature   :");
        gpio_put(LED_PIN, 0);

        for (size_t i = 0; i < SIZE; ++i) {
            printf("\t%5.2f", am2302[i].get_Temperature());
        }
        printf("\n\tHumidity      :");
        for (size_t i = 0; i < SIZE; ++i) {
            printf("\t%5.2f", am2302[i].get_Humidity());
        }
        printf("\n\n");
        sleep_ms(10000);
    }
}