/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

static volatile bool fired = false;

static void alarm_callback(void) {
    datetime_t t = {0};
    rtc_get_datetime(&t);
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("Alarm Fired At %s\n", datetime_str);
    stdio_flush();
    fired = true;
}

int main() {
    stdio_init_all();
    printf("RTC Alarm!\n");

    // Start on Wednesday 13th January 2021 11:20:00
    datetime_t t = {
        .year  = 2020,
        .month = 01,
        .day   = 13,
        .dotw  = 3, // 0 is Sunday, so 3 is Wednesday
        .hour  = 11,
        .min   = 20,
        .sec   = 00
    };

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);

    // Alarm 5 seconds later
    datetime_t alarm = {
        .year  = 2020,
        .month = 01,
        .day   = 13,
        .dotw  = 3, // 0 is Sunday, so 3 is Wednesday
        .hour  = 11,
        .min   = 20,
        .sec   = 05
    };

    rtc_set_alarm(&alarm, &alarm_callback);

    while(!fired);

    return 0;
}