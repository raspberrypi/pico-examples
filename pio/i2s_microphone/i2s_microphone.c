#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "i2s_microphone.pio.h"

#define PIO_INPUT_PIN_BASE 12
#define PIO_OUTPUT_PIN_BASE 10

int main() {
    stdio_init_all();

    // Load the i2s_microphone program, and configure a free state machine
    // to run the program.
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &i2s_microphone_program);
    uint sm = pio_claim_unused_sm(pio, true);
    i2s_microphone_2_program_init(pio, sm, offset, PIO_OUTPUT_PIN_BASE, PIO_INPUT_PIN_BASE);

    while (1);
    
}
