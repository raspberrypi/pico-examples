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
const uint SEGMENT_COUNT = 8;
const uint END = 9;
const uint CHARACTERS[][8] = {
    { SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_E, SEGMENT_F, END, END }, // 0
    { SEGMENT_B, SEGMENT_C, END, END, END, END, END, END }, // 1
    { SEGMENT_A, SEGMENT_B, SEGMENT_D, SEGMENT_E, SEGMENT_G, END, END, END }, // 2
    { SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_G, END, END, END }, // 3
    { SEGMENT_B, SEGMENT_C, SEGMENT_F, SEGMENT_G, END, END, END, END }, // 4
    { SEGMENT_A, SEGMENT_C, SEGMENT_D, SEGMENT_F, SEGMENT_G, END, END, END }, // 5
    { SEGMENT_A, SEGMENT_C, SEGMENT_D, SEGMENT_E, SEGMENT_F, SEGMENT_G, END, END }, // 6
    { SEGMENT_A, SEGMENT_B, SEGMENT_C, END, END, END, END, END }, // 7
    { SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_E, SEGMENT_F, SEGMENT_G, END }, // 8
    { SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_F, SEGMENT_G, END, END }, // 9
    { SEGMENT_DP, END, END, END, END, END, END, END }, // .
    { SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_E, SEGMENT_F, SEGMENT_G, SEGMENT_DP } // LAMP TEST
};
const uint CHAR_0 = 0;
const uint CHAR_1 = 1;
const uint CHAR_2 = 2;
const uint CHAR_3 = 3;
const uint CHAR_4 = 4;
const uint CHAR_5 = 5;
const uint CHAR_6 = 6;
const uint CHAR_7 = 7;
const uint CHAR_8 = 8;
const uint CHAR_9 = 9;
const uint CHAR_DP = 10;
const uint CHAR_LAMP_TEST = 11;

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

/// @brief call the specified function on a variable-length constant-terminated array of segments
/// @param segments array of segments on which to call the function
/// @param something pointer to a function which takes a uint (the segment) and does something to it
/// @note Max array size is SEGMENT_COUNT. Terminate the array with the constant END.
void variable_segments_do(const uint segments[], void (*something)(uint)) {
    for(int i = 0; i < SEGMENT_COUNT && segments[i] != END; i++) {
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
        for(int i = 0; i < 3; i++) {
            all_segments_do(set);
            sleep_ms(250);
            all_segments_do(reset);
            sleep_ms(250);
        }

        sleep_ms(2000);

        variable_segments_do(CHARACTERS[1], set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);
        variable_segments_do(CHARACTERS[0], set);
        sleep_ms(250);
        all_segments_do(reset);
        
        sleep_ms(2000);

        for(int c = 0; c < 12; c++) {
            variable_segments_do(CHARACTERS[c], set);
            sleep_ms(250);
            all_segments_do(reset);
            sleep_ms(250);
        }

        sleep_ms(2000);

        variable_segments_do(CHARACTERS[CHAR_8], set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);
        variable_segments_do(CHARACTERS[CHAR_6], set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);
        variable_segments_do(CHARACTERS[CHAR_7], set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);
        variable_segments_do(CHARACTERS[CHAR_5], set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);
        variable_segments_do(CHARACTERS[CHAR_3], set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);
        variable_segments_do(CHARACTERS[CHAR_0], set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);
        variable_segments_do(CHARACTERS[CHAR_9], set);
        sleep_ms(250);
        all_segments_do(reset);
        sleep_ms(250);

        sleep_ms(2000);

    }
}
