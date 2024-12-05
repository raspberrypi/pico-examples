/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
// Include sys/types.h before inttypes.h to work around issue with
// certain versions of GCC and newlib which causes omission of PRIx64
#include <sys/types.h>
#include <inttypes.h>

#include "pico/stdlib.h"
#include "pico/rand.h"

// How many random numbers to generate
#define RANDOM_COUNT 500000

// Percentage that the min and max count must be from the mean
#define RANDOM_PERCENT_MIN 98
#define RANDOM_PERCENT_MAX 101

int main()
{
    stdio_init_all();

    // Generate some random numbers. It will seed itself
    uint32_t r32 = get_rand_32();
    uint64_t r64 = get_rand_64();
    rng_128_t r128;
    get_rand_128(&r128);

    // Display the random numbers
    printf("Random 32bits %08"PRIx32"\n", r32);
    printf("Random 64bits %016"PRIx64"\n", r64);
    printf("Random 128bits %016"PRIx64"%016"PRIx64"\n", r128.r[0], r128.r[1]);

    // Lets check they appear to be a bit random
    printf("Generating %d random numbers\n", RANDOM_COUNT);
    uint32_t count[32] = {0};
    for (int run = 0; run < RANDOM_COUNT; run++) {
        count[get_rand_32() % count_of(count)]++;
    }

    // Display count
    uint32_t biggest = 0;
    uint32_t smallest = 0xFFFFFFFF;
    for (uint num = 0; num < count_of(count); num++) {
        printf("count of %d: %u\n", num, count[num]);
        if (count[num] > biggest) biggest = count[num];
        if (count[num] < smallest) smallest = count[num];

        // Check the distribution
        int check = ((RANDOM_COUNT / count_of(count)) * 100) / count[num];
        hard_assert(check >= RANDOM_PERCENT_MIN && check <= RANDOM_PERCENT_MAX);
    }
    printf("Biggest count: %u\n", biggest);
    printf("Smallest count: %u\n", smallest);

    printf("All done\n");
    return 0;
}