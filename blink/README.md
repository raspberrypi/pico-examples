### Code with comments

#include "pico/stdlib.h"                                //include header files needed for the code

int main() {                                            //begin main function - here will be the code that will be run on startup
#ifndef PICO_DEFAULT_LED_PIN                            //check if PICO_DEFAULT_LED_PIN is defined (onboard led)
#warning blink example requires a board with a regular LED
#else                                                   //if there is an onboard led
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;          //set the led pin to the onboard pin
    gpio_init(LED_PIN);                                 //enable led pin
    gpio_set_dir(LED_PIN, GPIO_OUT);                    //set the pin to be an an output pin
    while (true) {                                      //run this code in a loop
        gpio_put(LED_PIN, 1);                           //turn the led on (set the pin to binary 1 - on)
        sleep_ms(250);                                  //wait 250 milliseconds
        gpio_put(LED_PIN, 0);                           //turn the led off (set the pin to binary 0 - off)
        sleep_ms(250);                                  //wait 250 milliseconds
    }                                                   //end while loop
#endif                                                  //PICO_DEFAULT_LED_PIN
}                                                       //end main function
