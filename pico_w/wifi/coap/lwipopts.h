#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// needed for libcoap
#define MEMP_USE_CUSTOM_POOLS 1

#define MEM_SIZE 8000

// Generally you would define your own explicit list of lwIP options
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)
//
// This example uses a common include to avoid repetition
#include "lwipopts_examples_common.h"

#if NO_SYS
#define LOCK_TCPIP_CORE()
#define UNLOCK_TCPIP_CORE()

// This is for tinydtls
typedef int dtls_mutex_t;
#define DTLS_MUTEX_INITIALIZER 0
#define dtls_mutex_lock(a) *(a) = 1
#define dtls_mutex_trylock(a) *(a) = 1
#define dtls_mutex_unlock(a) *(a) = 0
#else
#error todo: define dtls_mutex_t
#endif

#endif
