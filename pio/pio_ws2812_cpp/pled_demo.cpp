// Adapted from https://github.com/raspberrypi/pico-examples/blob/master/pio/ws2812/ws2812_parallel.c
// SPDX-License-Identifier: BSD-3-Clause

// #define DEBUG
// #define TEST_CRGB
// #define TEST_PLED

#ifdef DEBUG
#include <stdio.h>
#include <tusb.h>
#endif
#include <stdlib.h>
#include <string.h>

#define LEDS_PER_STRIP 64
#define NUM_STRIPS 4
#define PIN_BASE 2
#define TOTAL_PIXELS (NUM_STRIPS * LEDS_PER_STRIP)
#define INITIAL_BRIGHT 16

#include "pled.hpp"

void pattern_snakes(int tick) {
    for (uint i = 0; i < TOTAL_PIXELS; i++) {
        uint x = (i + (tick >> 1)) % 64;
        if (x < 10)
            PLED::led[i] = CRGB(0xff, 0, 0);
        else if (x >= 15 && x < 25)
            PLED::led[i] = CRGB(0, 0xff, 0);
        else if (x >= 30 && x < 40)
            PLED::led[i] = CRGB(0, 0, 0xff);
        else
            PLED::led[i] = CRGB(0);
    }
}

void pattern_random(int tick) {
    if (tick % 8)
        return;
    for (uint i = 0; i < TOTAL_PIXELS; ++i)
        PLED::led[i] = CRGB(rand());
}

void pattern_sparkle(int tick) {
    if (tick % 8)
        return;
    for (uint i = 0; i < TOTAL_PIXELS; ++i)
        PLED::led[i] = CRGB(rand() % 16 ? 0 : 0xffffffff);
}

void pattern_greys(int tick) {
    uint max = 100;  // let's not draw too much current!
    tick %= max;
    for (uint i = 0; i < TOTAL_PIXELS; ++i) {
        PLED::led[i] = CRGB(tick * 0x10101);
        if (++tick >= max)
            tick = 0;
    }
}

void pattern_point(int tick) {
    for (uint i = 0; i < TOTAL_PIXELS; ++i) {
        PLED::led[i] = CRGB(0);
    }
    PLED::led[(uint)tick % TOTAL_PIXELS] = CRGB(0xFFFFFF);
}

typedef void (*pattern)(int tick);
const struct
{
    pattern pat;
    const char *name;
} pattern_table[] = {
    {pattern_snakes, "Snakes!"},
    {pattern_random, "Random data"},
    {pattern_sparkle, "Sparkles"},
    {pattern_greys, "Greys"},
    {pattern_point, "Point"}};

void prompt(const char *p) {
#ifdef DEBUG
    printf("%s (Anykey):", p);
    char resp = getchar();
    printf("\nOK\n");
#endif
}

int main() {
#ifdef DEBUG
    stdio_init_all();

    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }
    sleep_ms(500);
    prompt("WS2812 parallel demo\n");
#endif
    PLED::init();
#ifdef DEBUG
    printf("Planes@%X data@%X diff %d\n", &PLED::planes[0][0], &PLED::strip[0][0]);
#ifdef TEST_CRGB
    printf("CRGB tests\n");
    CRGB t, t1(0x010203), t2(0x10, 0x20, 0x30);
    printf("Default CRGB 00000000=%08X\n", t.toRGB());
    printf("Word CRGB 00010203=%08X\n", t1.toRGB());
    printf("Byte CRGB 00102030=%08X\n", t2.toRGB());
    t.r = 0xFF;
    printf("Set r 00FF0000=%08X\n", t.toRGB());
    t.g = 0x80;
    printf("Set g 00FF8000=%08X\n", t.toRGB());
    t.b = 0x40;
    printf("set b 00FF8040=%08X\n", t.toRGB());
    t2[1] = 0xff;
    printf("Set [1] 0010FF30=%08X\n", t2.toRGB());
    printf("Read g 00000002=%08X\n", t1.g);
    printf("Read [0] 00000001=%08X\n", t1[0]);
    printf("Half CRGB 00804020=%08X\n", (t / 2.0).toRGB());
    printf("Scale CRGB 00804020=%08X\n", (t * (uint8_t)127).toRGB());
    printf("Add 00C06030=%08X\n", (t / 4.0 + t / 2.0).toRGB());
    t = 0x804020;
    printf("Set word 00804020=%08X\n", t.toRGB());
    printf("Get grb 00408020=%08X\n", t.toGRB());
    printf("Done\n");
#endif
#ifdef TEST_PLED
    printf("PLED tests\n");
    PLED::strip[0][0] = CRGB(0x102030);
    PLED::strip[1][0] = CRGB(0x010203);
    printf("Read led 00102030=%08X\n", PLED::led[0].toRGB());
    printf("Read led 00010203=%08X\n", PLED::led[LEDS_PER_STRIP].toRGB());
    PLED::led[2 * LEDS_PER_STRIP] = CRGB(0x804020);
    printf("Write led 00804020=%08X\n", PLED::strip[2][0].toRGB());
    PLED::transpose_bits();
    printf("Check 00804020=%08X\n", PLED::strip[2][0].toRGB());
    printf("Read planes\n");
    for (int b = 0; b < 8; b++) {
        printf("%d:%08X\n", b, PLED::planes[0][b]);
    }
    printf("Done\n");
#endif
#endif
    while (1) {
        int tick = 0;
        int pat = rand() % count_of(pattern_table);
        int dir = (rand() >> 30) & 1 ? 1 : -1;
        if (rand() & 1)
            dir = 0;
#ifdef DEBUG
        printf(pattern_table[pat].name);
        prompt(dir == 1 ? "(forward)" : dir ? "(backward)"
                                            : "(still)");
#endif
        for (int i = 0; i < 1000; ++i) {
            pattern_table[pat].pat(tick);
#ifdef DEBUG
            printf("First pixel %06X\r", PLED::led[0].toRGB());
#endif
            tick += dir;
            PLED::show();
        }
#ifdef DEBUG
        printf("First pixel %06X\n", PLED::led[0].toRGB());
#endif
    }
}
