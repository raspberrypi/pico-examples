#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Generally you would define your own explicit list of lwIP options
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)
//
// This example uses a common include to avoid repetition
#include "lwipopts_examples_common.h"

/* TCP WND must be at least 16 kb to match TLS record size
   or you will get a warning "altcp_tls: TCP_WND is smaller than the RX decrypion buffer, connection RX might stall!" */
#undef TCP_WND
#define TCP_WND  16384

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
