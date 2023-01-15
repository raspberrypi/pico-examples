#include "pico/stdlib.h"

int main() {
    const uint SEGMENT_DP = 0;
    const uint SEGMENT_A = 1;
    const uint SEGMENT_B = 2;
    const uint SEGMENT_C = 3;
    const uint SEGMENT_D = 4;
    const uint SEGMENT_E = 5;
    const uint SEGMENT_F = 6;
    const uint SEGMENT_G = 7;

    gpio_init(SEGMENT_DP);
    gpio_init(SEGMENT_A);
    gpio_init(SEGMENT_B);
    gpio_init(SEGMENT_C);
    gpio_init(SEGMENT_D);
    gpio_init(SEGMENT_E);
    gpio_init(SEGMENT_F);
    gpio_init(SEGMENT_G);

    gpio_set_dir(SEGMENT_DP, GPIO_OUT);
    gpio_set_dir(SEGMENT_A, GPIO_OUT);
    gpio_set_dir(SEGMENT_B, GPIO_OUT);
    gpio_set_dir(SEGMENT_C, GPIO_OUT);
    gpio_set_dir(SEGMENT_D, GPIO_OUT);
    gpio_set_dir(SEGMENT_E, GPIO_OUT);
    gpio_set_dir(SEGMENT_F, GPIO_OUT);
    gpio_set_dir(SEGMENT_G, GPIO_OUT);

    while (true) {
        gpio_put(SEGMENT_DP, 1);
        gpio_put(SEGMENT_A, 1);
        gpio_put(SEGMENT_B, 1);
        gpio_put(SEGMENT_C, 1);
        gpio_put(SEGMENT_D, 1);
        gpio_put(SEGMENT_E, 1);
        gpio_put(SEGMENT_F, 1);
        gpio_put(SEGMENT_G, 1);
        sleep_ms(250);
        gpio_put(SEGMENT_DP, 0);
        gpio_put(SEGMENT_A, 0);
        gpio_put(SEGMENT_B, 0);
        gpio_put(SEGMENT_C, 0);
        gpio_put(SEGMENT_D, 0);
        gpio_put(SEGMENT_E, 0);
        gpio_put(SEGMENT_F, 0);
        gpio_put(SEGMENT_G, 0);
        sleep_ms(250);
    }
}
