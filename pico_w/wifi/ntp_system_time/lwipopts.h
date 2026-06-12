#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Extra options for the lwIP/SNTP application
// (see https://www.nongnu.org/lwip/2_1_x/group__sntp__opts.html)
//
// This example uses a common include to avoid repetition
#include "lwipopts_examples_common.h"

// If we use SNTP we should increase the number of LWIP system timeouts by one
#define MEMP_NUM_SYS_TIMEOUT        (LWIP_NUM_SYS_TIMEOUT_INTERNAL+1)
#define SNTP_MAX_SERVERS            LWIP_DHCP_MAX_NTP_SERVERS
#define SNTP_GET_SERVERS_FROM_DHCP  LWIP_DHCP_GET_NTP_SRV
#define SNTP_SERVER_DNS             1
#define SNTP_SERVER_ADDRESS         "pool.ntp.org"
// show debug information from the lwIP/SNTP application
#define SNTP_DEBUG                  LWIP_DBG_ON
#define SNTP_PORT                   LWIP_IANA_PORT_SNTP
// verify IP addresses and port numbers of received packets
#define SNTP_CHECK_RESPONSE         2

// compensate for packet transmission delay
// do NOT enable this on RP2040 if you are using aon_timer to keep the system
// time, because it only has a 1 second resolution (see CMakeLists.txt)
#ifdef DISABLE_SNTP_COMP_ROUNDTRIP
#define   SNTP_COMP_ROUNDTRIP       0
#else
#define   SNTP_COMP_ROUNDTRIP       1
#endif

#define SNTP_STARTUP_DELAY          1
#define SNTP_STARTUP_DELAY_FUNC     (LWIP_RAND() % 5000)
#define SNTP_RECV_TIMEOUT           15000
// how often to query the NTP servers, in ms (60000 is the minimum permitted by RFC4330) 
#define SNTP_UPDATE_DELAY           3600000
#define SNTP_RETRY_TIMEOUT           SNTP_RECV_TIMEOUT
#define SNTP_RETRY_TIMEOUT_MAX       (SNTP_RETRY_TIMEOUT * 10)
#define	SNTP_RETRY_TIMEOUT_EXP       1
#define SNTP_MONITOR_SERVER_REACHABILITY    1

//* configure SNTP to use our callback functions for reading and setting the system time
#define SNTP_GET_SYSTEM_TIME(sec, us)  sntp_get_system_time_us(&(sec), &(us))
#define SNTP_SET_SYSTEM_TIME_US(sec, us)   sntp_set_system_time_us(sec, us)

//* declare our callback functions (the implementations are in ntp_system_time.c)
#include "stdint.h"
void sntp_set_system_time_us(uint32_t sec, uint32_t us);
void sntp_get_system_time_us(uint32_t *sec_ptr, uint32_t *us_ptr);


#endif /* __LWIPOPTS_H__ */