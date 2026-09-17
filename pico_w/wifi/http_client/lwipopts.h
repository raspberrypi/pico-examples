#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Generally you would define your own explicit list of lwIP options
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)
//
// This example uses a common include to avoid repetition
#include "lwipopts_examples_common.h"

/* TCP_WND must be strictly greater than the maximum raw TLS record size.
   A 16384-byte plaintext TLS record expands to ~16413 raw bytes (+ header/nonce/auth tag).
   Setting TCP_WND == 16384 deadlocks: the server fills the window before mbedTLS can
   complete the record, so altcp_recved is never called and rcv_wnd stays at 0.
   Use 2x to allow one record in flight while the previous one is being acked. */
#undef TCP_WND
#define TCP_WND  32768

#define LWIP_ALTCP 1

// If you don't want to use TLS (just a http request) you can avoid linking to mbedtls and remove the following
#define LWIP_ALTCP_TLS           1
#define LWIP_ALTCP_TLS_MBEDTLS   1

// Note bug in lwip with LWIP_ALTCP and LWIP_DEBUG
// https://savannah.nongnu.org/bugs/index.php?62159
//#define LWIP_DEBUG 1
#undef LWIP_DEBUG
#define ALTCP_MBEDTLS_DEBUG  LWIP_DBG_ON

#endif
