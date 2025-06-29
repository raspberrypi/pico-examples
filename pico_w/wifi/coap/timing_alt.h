#ifndef _TIMING_ALT_
#define _TIMING_ALT_

#include "pico/async_context.h"
#include "pico/time.h"

struct mbedtls_timing_hr_time {
    absolute_time_t t;
};

typedef struct mbedtls_timing_delay_context {
    absolute_time_t fin_time;
    absolute_time_t int_time;
} mbedtls_timing_delay_context;

#endif
