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
                                     // present in keyboards, so it can be used
                                     // to determine if no key is pressed.
#define READ_MS_DELAY            10  // Delay between read to avoid noise

// Enable enhanced mode to allow reading multiple keys simultaneously.
#define ENHANCED_KEYPAD


#ifdef ENHANCED_KEYPAD
/*
 * When using the Enhanced Keypad mode, the get_keypad_value function
 * returns a KeypadResult structure instead of a single character.
 *
 * KeypadResult includes:
 * - count: The number of keys pressed (up to MAX_MULTI_KEYS).
 * - keys: An array containing the pressed keys (up to MAX_MULTI_KEYS).
 */
#define MAX_MULTI_KEYS 6  // Maximum number of keys that can be pressed simultaneously.

typedef struct {
    int count;                  // Number of keys pressed.
    char keys[MAX_MULTI_KEYS];  // Array of pressed keys.
} KeypadResult;
#endif  // ENHANCED_KEYPAD


const uint8_t keypad_rows_pins[KEYPAD_NUM_ROWS] = {
    9,      // GP9
    8,      // GP8
    7,      // GP7
    6,      // GP6
};


const uint8_t keypad_columns_pins[KEYPAD_NUM_COLUMNS] = {
    5,      // GP5
    4,      // GP4
    3,      // GP2
    2,      // GP1
};


const char keypad_keys[KEYPAD_NUM_ROWS][KEYPAD_NUM_COLUMNS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};


void pico_keypad_init(void) {
    // Initialize GPIOs
    //Initialize row pins as GPIO_OUT
    for (int i=0; i < KEYPAD_NUM_ROWS; i++) {
        uint8_t pin_number = keypad_rows_pins[i];
        gpio_init(pin_number);
        gpio_set_dir(pin_number, GPIO_OUT);
        gpio_put(pin_number, 0);  // Make sure GPIO_OUT is LOW
    }
    //Initialize column pins as GPIO_IN
    for (int i=0; i < KEYPAD_NUM_COLUMNS; i++) {
        uint8_t pin_number = keypad_columns_pins[i];
        gpio_init(pin_number);
        gpio_set_dir(pin_number, GPIO_IN);
    }
}


#ifdef ENHANCED_KEYPAD
KeypadResult get_keypad_result(void) {
    KeypadResult result;        // Define the result structure.
    result.count = 0;           // Initialize count to 0.

    // Iterate over key and rows to identify which key(s) are pressed.
    for (int row=0; row < KEYPAD_NUM_ROWS; row++) {
        gpio_put(keypad_rows_pins[row], 1);
        for (int column=0; column < KEYPAD_NUM_COLUMNS; column++) {
            sleep_ms(READ_MS_DELAY);
            if(gpio_get(keypad_columns_pins[column])) {
                // If the column pin is HIGH, a key is pressed.
                // Save the key in the KeypadResult structure and increase
                // count.
                result.keys[result.count] = keypad_keys[row][column];
                result.count++;
            }
        }
        gpio_put(keypad_rows_pins[row], 0);
    }

    // Return the structure containing all pressed keys.
    return result;
}
#else
char get_keypad_result(void) {
    // Iterate over key and rows to identify which key is pressed.
    // When iterating rows, the GPIO_OUT associated to the row needs to be set
    // to HIGH, and then iterate the columns to determine the GPIO_IN.
    for (int row=0; row < KEYPAD_NUM_ROWS; row++) {
        gpio_put(keypad_rows_pins[row], 1);
        for (int column=0; column < KEYPAD_NUM_COLUMNS; column++) {
            sleep_ms(READ_MS_DELAY);
            if(gpio_get(keypad_columns_pins[column])) {
                // If the column pin is HIGH, a key is pressed.
                // Put the row pin to low and return the pressed key.
                gpio_put(keypad_rows_pins[row], 0);
                return keypad_keys[row][column];
            }
        }
        gpio_put(keypad_rows_pins[row], 0);
    }

    // Return a constant indicating no key was pressed
    return NO_KEY_PRESSED;
}
#endif  //ENHANCED_KEYPAD


int main() {
    stdio_init_all();
    pico_keypad_init();
    while (true) {
        #ifdef ENHANCED_KEYPAD
        // Call the enhanced function to get the result structure
        // containing the number of keys pressed and the keys themselves.
        KeypadResult result = get_keypad_result();

        // Check if no keys are pressed.
        if (result.count == 0) {
            // Inform the user that no keys are pressed.
            printf("No key pressed\n");
        }
        else{
            // If one or more keys are pressed, print the number of pressed keys.
            printf("Bang!!! '%d' key(s) are pressed. Keys: ", result.count);

            // Iterate through the pressed keys and print them.
            for (int i = 0; i < result.count; i++) {
                printf("%c", result.keys[i]);

                //  If it's not the last key, print a comma separator.
                if (i != (result.count - 1)) {
                    printf(", ");
                } else {
                    // If it's the last key, move to the next line.
                    printf("\n");
                }
            }
        }
        #else
        // Call the simple function to get a single pressed key.
        char key = get_keypad_result();

        // Check if no key is pressed.
        if (key == NO_KEY_PRESSED) {
            // Inform the user that no keys are pressed.
            printf("No key pressed\n");
        }
        else{
            // If a key is pressed, print the key.
            printf("Key '%c' pressed\n", key);
        }
        #endif
        sleep_ms(500);
    }
}
