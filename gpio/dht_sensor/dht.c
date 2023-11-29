/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#ifdef PICO_DEFAULT_LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif

// Data memory
struct DHT_Data {
    float humidity;
    float temp_celsius;
};

// forward declarations
int8_t await_state(uint8_t state);
int8_t read_sensor_data(uint8_t *buffer, uint8_t size);
int8_t read_from_dht(struct DHT_Data *result);
void print_byte_as_bit(char value);

// define constants
const int8_t SENSOR_READ_OK = 0;
const int8_t SENSOR_ERROR_CHECKSUM = -1;
const int8_t SENSOR_ERROR_TIMEOUT = -2;
const uint8_t READ_TIMEOUT = 100U;

// DHT Pin
const uint DHT_PIN = 15;

int main() {
    stdio_init_all();
    gpio_init(DHT_PIN);
#ifdef LED_PIN
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif
     printf("\n\n === Pi Pico Example - Read DHTxx-Sensor === \n\n");
    
    while (1) {

#ifdef LED_PIN
    gpio_put(LED_PIN, 1);
#endif
        struct DHT_Data dht_data;
        int8_t status = read_from_dht(&dht_data);

#ifdef LED_PIN
    gpio_put(LED_PIN, 0);
#endif

        printf("Sensor-status: %d\n", status);

        if (status == SENSOR_READ_OK) {
            float fahrenheit = (dht_data.temp_celsius * 9 / 5) + 32;
            printf("Humidity = %.1f %%\tTemperature = %.1f degC (%.1f F)\n",
                dht_data.humidity, dht_data.temp_celsius, fahrenheit);
        }
        else {
            if (status == SENSOR_ERROR_CHECKSUM) {
                printf("ERROR: Checksum not valid\n");
            }
            else if (status == SENSOR_ERROR_TIMEOUT) {
                printf("ERROR: Timeout overflow\n");
            }
        }
        printf("\n\n");
        sleep_ms(5000);
    }
}

int8_t read_from_dht(struct DHT_Data *result) {
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    // start sequence min. 1 ms low then high
    gpio_put(DHT_PIN, 0);
    sleep_us(1200U);
    gpio_put(DHT_PIN, 1);
    gpio_set_dir(DHT_PIN, GPIO_IN);

    // await for the Acknowledge sequence
    // wait for 80 µs lOW
    await_state(0);
    // wait for 80 µs HIGH
    await_state(1);

    // Now read Sensor data
    // start of time critical code
    // create buffer
    uint8_t data[5U] = {0};
    if (read_sensor_data(data, 5U) == SENSOR_ERROR_TIMEOUT) {
        return SENSOR_ERROR_TIMEOUT;
    }
    // ==> END of time critical code <==
    // check checksum
    int8_t _checksum_ok = (data[4] == ( (data[0] + data[1] + data[2] + data[3]) & 0xFF) );
    //printf("Checksum_ok: %d\n", _checksum_ok);

    /*
    // Code part to check the checksum
    // Due to older sensors have an bug an deliver wrong data
    int8_t d4 = data[4];
    int8_t cs = ( (data[0] + data[1] + data[2] + data[3]) & 0xFF);
    printf("delivered Checksum:  %d - ", d4);
    print_byte_as_bit(d4);
    printf("calculated Checksum: %d - ", cs);
    print_byte_as_bit(cs);
    */

    if (_checksum_ok) {
        uint16_t hum = (data[0] << 8) | data[1];
        int16_t temp = (data[2] << 8) | data[3];
        result->humidity  = hum * 0.1F;
        result->temp_celsius = temp * 0.1F;
        return SENSOR_READ_OK;	
    }
    else {
        return SENSOR_ERROR_CHECKSUM;
    }
}

/**
 * @brief wait for given state
 * 
 * @param state 
 * @return int8_t state to wait for
 */
int8_t await_state(uint8_t state) {
    uint8_t wait_counter = 0, state_counter = 0;
    // count wait for state time
    while ( (gpio_get(DHT_PIN) != state) ) {
        ++wait_counter;
        sleep_us(1U);
        if (wait_counter >= READ_TIMEOUT) {
            return SENSOR_ERROR_TIMEOUT;
        }
    }
    // count state time
    while ( (gpio_get(DHT_PIN) == state) ) {
        ++state_counter;
        sleep_us(1U);
        if (state_counter >= READ_TIMEOUT) {
            return SENSOR_ERROR_TIMEOUT;
        }
    }
    return (state_counter > wait_counter);
}

/**
 * @brief read sensor data
 * 
 * @param buffer data buffer of 40 bit
 * @param size of buffer => 5 Byte
 * @return int8_t 
 */
int8_t read_sensor_data(uint8_t *buffer, uint8_t size) {
    for (uint8_t i = 0; i < size; ++i) {
        for (uint8_t bit = 0; bit < 8; ++bit) {
            uint8_t wait_counter = 0, state_counter = 0;
            // count wait for state time
            while ( !gpio_get(DHT_PIN) ) {
                ++wait_counter;
                sleep_us(1U);
                if (wait_counter >= READ_TIMEOUT) {
                    return SENSOR_ERROR_TIMEOUT;
                }
            }
            // count state time
            while ( gpio_get(DHT_PIN) ) {
                ++state_counter;
                sleep_us(1U);
                if (state_counter >= READ_TIMEOUT) {
                    return SENSOR_ERROR_TIMEOUT;
                }
            }
            buffer[i] <<= 1;
            buffer[i] |= (state_counter > wait_counter);
        }
    }
    return SENSOR_READ_OK;
}

/**
 * @brief helper function to print byte as bit
 * 
 * @param value byte with 8 bits
 */
void print_byte_as_bit(char value) {
    for (int i = 7; i >= 0; --i) {
        char c = (value & (1 << i)) ? '1' : '0';
    printf("%c", c);
    }
    printf("\n");
}
