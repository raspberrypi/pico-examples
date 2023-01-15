#include "pico/stdlib.h"

/*
 *  LED digital display segments and their corresponding pins
 */
const uint SEGMENT_DP = 0;
const uint SEGMENT_A = 1;
const uint SEGMENT_B = 2;
const uint SEGMENT_C = 3;
const uint SEGMENT_D = 4;
const uint SEGMENT_E = 5;
const uint SEGMENT_F = 6;
const uint SEGMENT_G = 7;
const uint SEGMENTS[] = { SEGMENT_DP, SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_E, SEGMENT_F, SEGMENT_G };

/// @brief sets a segment's pin to output
/// @param segment the segment to set to output
void set_dir_out(uint segment) {
    gpio_set_dir(segment, GPIO_OUT);
}

/// @brief initializes a segment's pin
/// @param segment the segment to initialize
void init(uint segment) {
    gpio_init(segment);
}

/// @brief set or turn on a segment
/// @param segment the segment to set
void set(uint segment) {
    gpio_put(segment, 1);
}

/// @brief reset or turn off a segment
/// @param segment the segment to reset
void reset(uint segment) {
    gpio_put(segment, 0);
}

/// @brief call the specified function on an array of segments
/// @param segments array of segments on which to call the function
/// @param segment_count size of the incoming array
/// @param something pointer to a function which takes a uint (the segment) and does something to it
void segments_do(uint segments[], size_t segment_count, void (*something)(uint)) {
    for(int i = 0; i < segment_count; i++) {
        something(segments[i]);
    }
}

/// @brief call the specified function on all segments
/// @param something pointer to a function which takes a uint (the segment) and does something to it
void all_segments_do(void (*something)(uint)) {
    segments_do((uint *)SEGMENTS, sizeof(SEGMENTS)/sizeof(SEGMENTS[0]), something);
}

int main() {
    all_segments_do(init);
    all_segments_do(set_dir_out);

    while (true) {
        all_segments_do(set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);
    }
}
