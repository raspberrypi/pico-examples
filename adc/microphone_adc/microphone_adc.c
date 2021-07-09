/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"

 /* Example code to plot analog values from a microphone

    Connections on Raspberry Pi Pico board, other boards may vary.

    GPIO 26/ADC0 (pin 31)-> AOUT or AUD on microphone board
    3.3v (pin 36) -> VCC on microphone board
    GND (pin 38)  -> GND on microphone board
 */

#define ADC_PIN 0
#define ADC_VREF 3.3
#define ADC_RANGE (1 << 12)
#define ADC_CONVERT ADC_VREF / ADC_RANGE

int main() {
    stdio_init_all();
    printf("Beep boop, listening...\n");

    bi_decl(bi_program_description("Analog microphone example for Raspberry Pi Pico")); // for picotool
    bi_decl(bi_1pin_with_name(ADC_PIN, "ADC input pin"));
    
    adc_init();
    adc_gpio_init(ADC_PIN + 26);
    adc_select_input(ADC_PIN);

    // wait for 255 from plotter.py
    char c;
    printf("%u", uart_getc(uart0));
    while ((c = uart_getc(uart0)) != 0xFF) {
        
    }

    uint adc_raw;
    while (0) {
        adc_raw = adc_read(); // raw voltage from ADC
        printf("%.2f\n", adc_raw * ADC_CONVERT);
        sleep_ms(1);
    }

    return 0;
}
