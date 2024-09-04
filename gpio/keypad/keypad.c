/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"


#define KEYPAD_NUM_ROWS           4
#define KEYPAD_NUM_COLUMNS        4
#define NO_KEY_PRESSED         '\0'  // Just a character that unlikely be
                                     // present in keyboards, so it can by used
                                     // to determine if no key is pressed.
#define READ_MS_DELAY            10  // Delay between lectures to avoid noise


const uint8_t keypad_rows_pins[KEYPAD_NUM_ROWS] = {
    10,
    11, 
    12,
    13,
};


const uint8_t keypad_columns_pins[KEYPAD_NUM_COLUMNS] = {
    18,  // GP18
    19,  // GP19
    20,  // GP20
    21,  // GP21
};


const char keypad_keys[KEYPAD_NUM_ROWS][KEYPAD_NUM_COLUMNS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};


void pico_keypad_init(void){
    // Initialize GPIOs
    //Initialize row pins as GPIO_OUT
    for (int i=0; i < KEYPAD_NUM_ROWS; i++){
        uint8_t pin_number = keypad_rows_pins[i];
        gpio_init(pin_number);
        gpio_set_dir(pin_number, GPIO_OUT);
        gpio_put(pin_number, 0);  // Make sure GPIO_OUT is LOW
    }
    //Initialize column pins as GPIO_IN
    for (int i=0; i < KEYPAD_NUM_COLUMNS; i++){
        uint8_t pin_number = keypad_columns_pins[i];
        gpio_init(pin_number);
        gpio_set_dir(pin_number, GPIO_IN);
    }
}


char get_keypad_value(void){
    // Iterate over key and rows to identify which key is pressed.
    // When iterating rows, the GPIO_OUT associted to the row needs to be set
    // to HIGH, and then iterate the columns to determine the GPIO_IN.
    for (int row=0; row < KEYPAD_NUM_ROWS; row++){
        gpio_put(keypad_rows_pins[row], 1);
        for (int column=0; column < KEYPAD_NUM_COLUMNS; column++){
            sleep_ms(READ_MS_DELAY);
            if(gpio_get(keypad_columns_pins[column])){
                // If the pin is HIGH, this means this is a matching row and
                // column, so we put the row pin to LOW and return the pressed
                // key by using the bidimensional array keypad_keys.
                gpio_put(keypad_rows_pins[row], 0);
                return keypad_keys[row][column];
            }
        }
        gpio_put(keypad_rows_pins[row], 0);
    }
    return NO_KEY_PRESSED;
}


int main() {
    stdio_init_all();
    pico_keypad_init();
    while (true) {
        char key = get_keypad_value();
        if (key == NO_KEY_PRESSED){
            printf("No key pressed\n");
        }
        else{
            printf("Key '%c' pressed\n", key);
        }
        sleep_ms(500);
    }
}
