#include <stdio.h>
#include "pico/stdlib.h"


bool state;
const uint LED_PIN = 25;

// Debounce control
unsigned long time = to_ms_since_boot(get_absolute_time());
const int delayTime = 50; // Change according to the push-buttons


void inter_test(uint gpio, uint32_t events) {
    if ((to_ms_since_boot(get_absolute_time())-time)>delayTime) {
        time = to_ms_since_boot(get_absolute_time());

        printf("interrupt worked\n");
        state = !state;
        gpio_put(LED_PIN, state);
    }
}




int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    state = true;
    gpio_put(LED_PIN, state);


    gpio_init(2);
    gpio_pull_up(2);

    gpio_set_irq_enabled_with_callback(2, GPIO_IRQ_EDGE_FALL , true, &inter_test);


    while(1) {}
}