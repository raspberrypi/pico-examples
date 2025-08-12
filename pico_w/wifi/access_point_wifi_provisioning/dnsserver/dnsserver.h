/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _DNSSERVER_H_
#define _DNSSERVER_H_

#include "lwip/ip_addr.h"

typedef struct dns_server_t_ {
    struct udp_pcb *udp;
     ip_addr_t ip;
} dns_server_t;

void dns_server_init(dns_server_t *d, ip_addr_t *ip);
void dns_server_deinit(dns_server_t *d);

#endif
