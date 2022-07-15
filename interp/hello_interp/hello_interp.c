/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/interp.h"

void times_table() {
    puts("9 times table:");

    // Initialise lane 0 on interp0 on this core
    interp_config cfg = interp_default_config();
    interp_set_config(interp0, 0, &cfg);

    interp0->accum[0] = 0;
    interp0->base[0] = 9;

    for (int i = 0; i < 10; ++i)
        printf("%d\n", interp0->pop[0]);
}

void moving_mask() {
    interp_config cfg = interp_default_config();
    interp0->accum[0] = 0x1234abcd;

    puts("Masking:");
    printf("ACCUM0 = %08x\n", interp0->accum[0]);
    for (int i = 0; i < 8; ++i) {
        // LSB, then MSB. These are inclusive, so 0,31 means "the entire 32 bit register"
        interp_config_set_mask(&cfg, i * 4, i * 4 + 3);
        interp_set_config(interp0, 0, &cfg);
        // Reading from ACCUMx_ADD returns the raw lane shift and mask value, without BASEx added
        printf("Nibble %d: %08x\n", i, interp0->add_raw[0]);
    }

    puts("Masking with sign extension:");
    interp_config_set_signed(&cfg, true);
    for (int i = 0; i < 8; ++i) {
        interp_config_set_mask(&cfg, i * 4, i * 4 + 3);
        interp_set_config(interp0, 0, &cfg);
        printf("Nibble %d: %08x\n", i, interp0->add_raw[0]);
    }
}

void cross_lanes() {
    interp_config cfg = interp_default_config();
    interp_config_set_cross_result(&cfg, true);
    // ACCUM0 gets lane 1 result:
    interp_set_config(interp0, 0, &cfg);
    // ACCUM1 gets lane 0 result:
    interp_set_config(interp0, 1, &cfg);

    interp0->accum[0] = 123;
    interp0->accum[1] = 456;
    interp0->base[0] = 1;
    interp0->base[1] = 0;
    puts("Lane result crossover:");
    for (int i = 0; i < 10; ++i) {
        uint32_t peek0 = interp0->peek[0];
        uint32_t pop1 = interp0->pop[1];
        printf("PEEK0, POP1: %d, %d\n", peek0, pop1);
    }
}

void simple_blend1() {
    puts("Simple blend 1:");

    interp_config cfg = interp_default_config();
    interp_config_set_blend(&cfg, true);
    interp_set_config(interp0, 0, &cfg);

    cfg = interp_default_config();
    interp_set_config(interp0, 1, &cfg);

    interp0->base[0] = 500;
    interp0->base[1] = 1000;

    for (int i = 0; i <= 6; i++) {
        // set fraction to value between 0 and 255
        interp0->accum[1] = 255 * i / 6;
        // ≈ 500 + (1000 - 500) * i / 6;
        printf("%d\n", (int) interp0->peek[1]);
    }
}

/// \tag::simple_blend2[]
void print_simple_blend2_results(bool is_signed) {
    // lane 1 signed flag controls whether base 0/1 are treated as signed or unsigned
    interp_config cfg = interp_default_config();
    interp_config_set_signed(&cfg, is_signed);
    interp_set_config(interp0, 1, &cfg);

    for (int i = 0; i <= 6; i++) {
        interp0->accum[1] = 255 * i / 6;
        if (is_signed) {
            printf("%d\n", (int) interp0->peek[1]);
        } else {
            printf("0x%08x\n", (uint) interp0->peek[1]);
        }
    }
}

void simple_blend2() {
    puts("Simple blend 2:");

    interp_config cfg = interp_default_config();
    interp_config_set_blend(&cfg, true);
    interp_set_config(interp0, 0, &cfg);

    interp0->base[0] = (uint32_t) -1000;
    interp0->base[1] = 1000;

    puts("signed:");
    print_simple_blend2_results(true);

    puts("unsigned:");
    print_simple_blend2_results(false);
}
/// \end::simple_blend2[]

void simple_blend3() {
    puts("Simple blend 3:");

    interp_config cfg = interp_default_config();
    interp_config_set_blend(&cfg, true);
    interp_set_config(interp0, 0, &cfg);

    cfg = interp_default_config();
    interp_set_config(interp0, 1, &cfg);

    interp0->accum[1] = 128;
    interp0->base01 = 0x30005000;
    printf("0x%08x\n", (int) interp0->peek[1]);
    interp0->base01 = 0xe000f000;
    printf("0x%08x\n", (int) interp0->peek[1]);

    interp_config_set_signed(&cfg, true);
    interp_set_config(interp0, 1, &cfg);

    interp0->base01 = 0xe000f000;
    printf("0x%08x\n", (int) interp0->peek[1]);
}

void linear_interpolation() {
    puts("Linear interpolation:");
    const int uv_fractional_bits = 12;

    // for lane 0
    // shift and mask XXXX XXXX XXXX XXXX XXXX FFFF FFFF FFFF (accum 0)
    // to             0000 0000 000X XXXX XXXX XXXX XXXX XXX0
    // i.e. non fractional part times 2 (for uint16_t)
    interp_config cfg = interp_default_config();
    interp_config_set_shift(&cfg, uv_fractional_bits - 1);
    interp_config_set_mask(&cfg, 1, 32 - uv_fractional_bits);
    interp_config_set_blend(&cfg, true);
    interp_set_config(interp0, 0, &cfg);

    // for lane 1
    // shift XXXX XXXX XXXX XXXX XXXX FFFF FFFF FFFF (accum 0 via cross input)
    // to    0000 XXXX XXXX XXXX XXXX FFFF FFFF FFFF

    cfg = interp_default_config();
    interp_config_set_shift(&cfg, uv_fractional_bits - 8);
    interp_config_set_signed(&cfg, true);
    interp_config_set_cross_input(&cfg, true); // signed blending
    interp_set_config(interp0, 1, &cfg);

    int16_t samples[] = {0, 10, -20, -1000, 500};

    // step is 1/4 in our fractional representation
    uint step = (1 << uv_fractional_bits) / 4;

    interp0->accum[0] = 0; // initial sample_offset;
    interp0->base[2] = (uintptr_t) samples;
    for (int i = 0; i < 16; i++) {
        // result2 = samples + (lane0 raw result)
        // i.e. ptr to the first of two samples to blend between
        int16_t *sample_pair = (int16_t *) interp0->peek[2];
        interp0->base[0] = sample_pair[0];
        interp0->base[1] = sample_pair[1];
        uint32_t peek1 = interp0->peek[1];
        uint32_t add_raw1 = interp0->add_raw[1];
        printf("%d\t(%d%% between %d and %d)\n", (int) peek1,
               100 * (add_raw1 & 0xff) / 0xff,
               sample_pair[0], sample_pair[1]);
        interp0->add_raw[0] = step;
    }
}

void clamp() {
    puts("Clamp:");
    interp_config cfg = interp_default_config();
    interp_config_set_clamp(&cfg, true);
    interp_config_set_shift(&cfg, 2);
    // set mask according to new position of sign bit..
    interp_config_set_mask(&cfg, 0, 29);
    // ...so that the shifted value is correctly sign extended
    interp_config_set_signed(&cfg, true);
    interp_set_config(interp1, 0, &cfg);

    interp1->base[0] = 0;
    interp1->base[1] = 255;

    for (int i = -1024; i <= 1024; i += 256) {
        interp1->accum[0] = i;
        printf("%d\t%d\n", i, (int) interp1->peek[0]);
    }
}

/// \tag::texture_mapping[]
void texture_mapping_setup(uint8_t *texture, uint texture_width_bits, uint texture_height_bits,
                           uint uv_fractional_bits) {
    interp_config cfg = interp_default_config();
    // set add_raw flag to use raw (un-shifted and un-masked) lane accumulator value when adding
    // it to the the lane base to make the lane result
    interp_config_set_add_raw(&cfg, true);
    interp_config_set_shift(&cfg, uv_fractional_bits);
    interp_config_set_mask(&cfg, 0, texture_width_bits - 1);
    interp_set_config(interp0, 0, &cfg);

    interp_config_set_shift(&cfg, uv_fractional_bits - texture_width_bits);
    interp_config_set_mask(&cfg, texture_width_bits, texture_width_bits + texture_height_bits - 1);
    interp_set_config(interp0, 1, &cfg);

    interp0->base[2] = (uintptr_t) texture;
}

void texture_mapped_span(uint8_t *output, uint32_t u, uint32_t v, uint32_t du, uint32_t dv, uint count) {
    // u, v are texture coordinates in fixed point with uv_fractional_bits fractional bits
    // du, dv are texture coordinate steps across the span in same fixed point.
    interp0->accum[0] = u;
    interp0->base[0] = du;
    interp0->accum[1] = v;
    interp0->base[1] = dv;
    for (uint i = 0; i < count; i++) {
        // equivalent to
        // uint32_t sm_result0 = (accum0 >> uv_fractional_bits) & (1 << (texture_width_bits - 1);
        // uint32_t sm_result1 = (accum1 >> uv_fractional_bits) & (1 << (texture_height_bits - 1);
        // uint8_t *address = texture + sm_result0 + (sm_result1 << texture_width_bits);
        // output[i] = *address;
        // accum0 = du + accum0;
        // accum1 = dv + accum1;

        // result2 is the texture address for the current pixel;
        // popping the result advances to the next iteration
        output[i] = *(uint8_t *) interp0->pop[2];
    }
}

void texture_mapping() {
    puts("Affine Texture mapping (with texture wrap):");

    uint8_t texture[] = {
            0x00, 0x01, 0x02, 0x03,
            0x10, 0x11, 0x12, 0x13,
            0x20, 0x21, 0x22, 0x23,
            0x30, 0x31, 0x32, 0x33,
    };
    // 4x4 texture
    texture_mapping_setup(texture, 2, 2, 16);
    uint8_t output[12];
    uint32_t du = 65536 / 2;  // step of 1/2
    uint32_t dv = 65536 / 3;  // step of 1/3
    texture_mapped_span(output, 0, 0, du, dv, 12);

    for (uint i = 0; i < 12; i++) {
        printf("0x%02x\n", output[i]);
    }
}
/// \end::texture_mapping[]

int main() {
    stdio_init_all();
    puts("Interpolator example");

    times_table();
    moving_mask();
    cross_lanes();
    simple_blend1();
    simple_blend2();
    simple_blend3();
    clamp();

    linear_interpolation();
    texture_mapping();

    return 0;
}