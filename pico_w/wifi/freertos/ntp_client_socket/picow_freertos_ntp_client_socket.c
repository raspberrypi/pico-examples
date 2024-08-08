/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "FreeRTOS.h"
#include "task.h"

#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define TEST_TASK_PRIORITY                ( tskIDLE_PRIORITY + 1UL )

#define NTP_SERVER ("pool.ntp.org")
#define NTP_MSG_LEN (48)
#define NTP_PORT (123)
// seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_DELTA (2208988800)
#define ntpEST_TIME (30 * 1000)
#define NTP_FAIL_TIME (10)

void main_task(__unused void *params) {
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        vTaskDelete(NULL);
    }
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        vTaskDelete(NULL);
    } else {
        printf("Connected.\n");
    }

    while(true) {
        const struct addrinfo ntp_info = {
            .ai_flags = 0,
            .ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = 0,
        };

        struct addrinfo *dns_result = NULL;
        printf("Trying DNS...\n");
        int err = getaddrinfo(NTP_SERVER, "123", &ntp_info, &dns_result);
        if (err != 0)
        {
            printf("DNS lookup failed with error: %d\n", err);
            continue;
        }
#ifndef INET6_ADDRSTRLEN
        char address[INET_ADDRSTRLEN] = {0};
#else
        char address[INET6_ADDRSTRLEN] = {0};
#endif
        ip_addr_t ip_addr;
#if LWIP_IPV4
        if (dns_result->ai_addr->sa_family == AF_INET) {
            const struct sockaddr_in *sock_addr = (struct sockaddr_in*)dns_result->ai_addr;
            inet_addr_to_ip4addr(ip_2_ip4(&ip_addr), &sock_addr->sin_addr);
            IP_SET_TYPE(&ip_addr, IPADDR_TYPE_V4);
        }
#endif
#if LWIP_IPV6
        if (dns_result->ai_addr->sa_family == AF_INET6) {
            const struct sockaddr_in6 *sock_addr = (struct sockaddr_in6*)dns_result->ai_addr;
            inet6_addr_to_ip6addr(ip_2_ip6(&ip_addr), &sock_addr->sin6_addr);
            IP_SET_TYPE(&ip_addr, IPADDR_TYPE_V6);
        }
#endif
        inet_ntop(dns_result->ai_family, &ip_addr, address, sizeof(address));
        printf("Got DNS response: %s\n", address);

        int sock = socket(dns_result->ai_family, dns_result->ai_socktype, dns_result->ai_protocol);
        if (sock == -1)
        {
            printf("Failed to allocate socket");
            freeaddrinfo(dns_result);
            continue;
        }

        struct timeval timeout = {
            .tv_sec = NTP_FAIL_TIME,
        };
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        err = connect(sock, dns_result->ai_addr, dns_result->ai_addrlen);
        freeaddrinfo(dns_result);
        if (err == -1)
        {
            printf("Failed to connect to NTP server");
            close(sock);
            continue;
        }

        unsigned char request[NTP_MSG_LEN] = {0x1b, 0};
        int amount = send(sock, request, NTP_MSG_LEN, 0);
        if (amount != NTP_MSG_LEN)
        {
            printf("We were unable to send the complete NTP request");
            close(sock);
            continue;
        }

        amount = recv(sock, request, NTP_MSG_LEN, 0);
        close(sock);
        if (amount != NTP_MSG_LEN)
        {
            printf("We did not receive a complete NTP response");
            continue;
        }

        uint8_t mode = request[0] & 0x7;
        uint8_t stratum = request[1];

        // Check the result
        if (amount == NTP_MSG_LEN &&
            mode == 0x4 &&
            stratum != 0)
        {
            uint8_t seconds_buf[4] = {};
            memcpy(seconds_buf, request + 40, 4);
            uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
            uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
            time_t epoch = seconds_since_1970;
            struct tm *utc = gmtime(&epoch);
            printf("got ntp response: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
                   utc->tm_hour, utc->tm_min, utc->tm_sec);
        }

        // This assumes one tick is one ms, which is true for the default configuration
        vTaskDelay(ntpEST_TIME);
    }

    cyw43_arch_deinit();
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "TestMainThread", 4096, NULL, TEST_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

int main( void )
{
    stdio_init_all();

    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
#if ( configNUMBER_OF_CORES > 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( configNUMBER_OF_CORES == 2 )
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif ( RUN_FREERTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}
