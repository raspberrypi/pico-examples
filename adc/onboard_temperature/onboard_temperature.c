#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, page 68. 
 * pico-examples/adc/adc_console/adc_console.c */
const float read_onboard_temperature(const char unit) {
    
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    adc_select_input(4);
    float adc = adc_read() * conversionFactor;

    float tempC = 27.0 - (adc - 0.706) / 0.001721;

    if (unit == 'C') {
        return tempC;
    } else if (unit == 'F') {
        return tempC * 9 / 5 + 32;
    }

    return -1.0;
}

int main() {
    const uint LED_PIN = 25;

    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    adc_init();
    adc_set_temp_sensor_enabled(true);

    while (true) {
        printf("Onboard temperature = %.02f C\n", read_onboard_temperature('C'));

        gpio_put(LED_PIN, 1);
        sleep_ms(10);

        gpio_put(LED_PIN, 0);
        sleep_ms(990);
    }

    return 0;
}
