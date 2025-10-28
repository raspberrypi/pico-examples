/**
 * Copyright (c) 2025 mjcross
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/sntp.h"
#include "pico/util/datetime.h"
#include "pico/aon_timer.h"
#include "pico/mutex.h"

// create a mutex to avoid reading the aon_timer at the same time as lwIP/SNTP is updating it 
auto_init_mutex(aon_timer_mutex);
static bool aon_timer_is_initialised = false;

// callback for lwIP/SNTP to set the aon_timer to UTC (see lwipopts.h)
// this is called every time the application receives a valid NTP server response
void sntp_set_system_time_us(uint32_t sec, uint32_t us) {    
    static struct timespec ntp_ts;
    ntp_ts.tv_sec = sec;
    ntp_ts.tv_nsec = us * 1000;

    if (aon_timer_is_initialised) {
        // wait up to 10ms to obtain exclusive access to the aon_timer
        if (mutex_enter_timeout_ms (&aon_timer_mutex, 10)) {
            aon_timer_set_time(&ntp_ts);
            mutex_exit(&aon_timer_mutex);   // release the mutex as soon as possible
            puts("-> updated system time from NTP");
        } else {
            puts("-> skipped NTP system time update (aon_timer was busy)");
        }
    } else {
        // the aon_timer is uninitialised so we don't need exclusive access
        aon_timer_is_initialised = aon_timer_start(&ntp_ts);
        puts("-> initialised system time from NTP");
    }
}

// callback for lwIP/SNTP to read system time (UTC) from the aon_timer
// when it needs to (eg) calculate the roundtrip transmission delay
void sntp_get_system_time_us(uint32_t *sec_ptr, uint32_t * us_ptr) {
    static struct timespec sys_ts;
    // we don't need exclusive access because we are on the background thread
    aon_timer_get_time(&sys_ts);
    *sec_ptr = sys_ts.tv_sec;
    *us_ptr = sys_ts.tv_nsec / 1000;
}

// function for user code to safely read the system time (UTC) asynchronously
int get_time_utc(struct timespec *ts_ptr) {
    int retval = 1;
    if (mutex_enter_timeout_ms(&aon_timer_mutex, 10)) {
        aon_timer_get_time(ts_ptr);
        mutex_exit(&aon_timer_mutex);
        retval = 0;
    }
    return retval;
}

int main() {
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // Enable wifi station mode
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect\n");
        return 1;
    }

    // display the ip address in human readable form
    uint8_t *ip_address = (uint8_t*)&(netif_default->ip_addr.addr);
    printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);

    // initialise the lwIP/SNTP application
    sntp_setoperatingmode(SNTP_OPMODE_POLL);        // lwIP/SNTP also accepts SNTP_OPMODE_LISTENONLY
    sntp_init();


    // ----- simple demonstration of how to read and display the system time -----
    //
    struct timespec ts;
    struct tm tm;

    // OPTIONAL: set the 'TZ' env variable to the local POSIX timezone (in this case Europe/London,
    // to create your own see https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_node/libc_431.html)
    setenv("TZ", "BST0GMT,M3.5.0/1,M10.5.0/2", 1);

    // If the environment contains a valid 'TZ' definition then functions like ctime(), localtime() 
    // and their variants automatically give results converted to the local timezone instead of UTC 
    // (see below).

    while (true) {

        if(aon_timer_is_initialised) {

            // safely read the current time as UTC seconds and ms since the epoch
            get_time_utc(&ts);

            // if you just want a string representation of the current time and you're not interested
            // in the individual date/time fields, then here you can simply call:
            //      printf("%s", ctime(&(ts.tv_sec)));
            // The string produced is the same as `asctime()` (see below). Note that if you set 'TZ'
            // then the output will be in local time, otherwise UTC.

            // you can unpack the raw UTC seconds count into individual date/time fields like this.
            // Again, if you set 'TZ' then the values will be in local time, otherwise UTC. Note
            // that by default `pico_localtime_r()` just calls `localtime_r()` from the standard 'C' 
            // library, but the declaration is 'weak' so that a user can override it with their own
            // implementation if desired.
            pico_localtime_r(&(ts.tv_sec), &tm);

            // display the name of the currently active local timeszone, if defined
            if (getenv("TZ")) {
                printf("%s: ", tm.tm_isdst ? tzname[0]: tzname[1]);
                // <time.h> defines `extern char *tzname[2]` to hold the names of the POSIX timezones
            } else {
                printf("UTC: ");
            }
            
            // display individual date/time fields in the form "Mon Oct 27 22:06:08 2025\n"
            printf("%s", asctime(&tm));

        } else {
            puts("system time not yet initialised");
        }

        sleep_ms(5000); // do nothing for 5 seconds
    }
}
