#include <stdio.h>
#include "pico/stdlib.h"

// Blinks an exteranl led on GPIO 10 (works for both pico & pico w)
// Sends a message over the UART serial port
// Demonstrates how to declare a debug variable with "volatile" to be "debugger firendly"

int main() {

    // Debug variables: declare with "volatile" to prevent compiler optimization.
    // Otherwise, the debugger may display the variable's value as "optimized out".
    volatile long dbgFriendly_loopCtr = 0;
    long dbgVicious_loopCtr = 0;

    // Initialize LED gpio
    const uint led_gpio = 10;
    gpio_init(led_gpio);
    gpio_set_dir(led_gpio, GPIO_OUT);

    // Initialize chosen serial port
    stdio_init_all();

    // Loop forever
    while (true) {
        // Update debugging variables
        dbgFriendly_loopCtr++; //  value in the debugger is inspectable
        dbgVicious_loopCtr++;  //  value in the debugger is "optimized out"

        // Write a message out to the serial port
        printf("Blinking Pin # %i! \r\n",led_gpio);              

        // Blink LED
        gpio_put(led_gpio, true);
        sleep_ms(1000);
        gpio_put(led_gpio, false);
        sleep_ms(1000);
    }
}
