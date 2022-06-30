/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps/lwiperf.h"

#include "FreeRTOS.h"
#include "task.h"

#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY				( tskIDLE_PRIORITY + 1UL )

#if CLIENT_TEST && !defined(IPERF_SERVER_IP)
#error IPERF_SERVER_IP not defined
#endif

// Report IP results and exit
static void iperf_report(void *arg, enum lwiperf_report_type report_type,
                         const ip_addr_t *local_addr, u16_t local_port, const ip_addr_t *remote_addr, u16_t remote_port,
                         u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec) {
    static uint32_t total_iperf_megabytes = 0;
    uint32_t mbytes = bytes_transferred / 1024 / 1024;
    float mbits = bandwidth_kbitpsec / 1000.0;

    total_iperf_megabytes += mbytes;

    printf("Completed iperf transfer of %d MBytes @ %.1f Mbits/sec\n", mbytes, mbits);
    printf("Total iperf megabytes since start %d Mbytes\n", total_iperf_megabytes);
}

void blink_task(__unused void *params) {
    bool on = false;
    printf("blink_task starts\n");
    while (true) {
#if 0 && configNUM_CORES > 1
        static int last_core_id;
        if (portGET_CORE_ID() != last_core_id) {
            last_core_id = portGET_CORE_ID();
            printf("blinking now from core %d\n", last_core_id);
        }
#endif
        cyw43_arch_gpio_put(0, on);
        on = !on;
        vTaskDelay(200);
    }
}

void main_task(__unused void *params) {
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }
    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        exit(1);
    } else {
        printf("Connected.\n");
    }

    xTaskCreate(blink_task, "BlinkThread", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL);

#if CLIENT_TEST
    printf("\nReady, running iperf client\n");
    ip_addr_t clientaddr;
    ip4_addr_set_u32(&clientaddr, ipaddr_addr(xstr(IPERF_SERVER_IP)));
    assert(lwiperf_start_tcp_client_default(&clientaddr, &iperf_report, NULL) != NULL);
#else
    printf("\nReady, running iperf server at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    lwiperf_start_tcp_server_default(&iperf_report, NULL);
#endif

    while(true) {
        // not much to do as LED is in another task, and we're using RAW (callback) lwIP API
        vTaskDelay(100);
    }

    cyw43_arch_deinit();
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "TestMainThread", configMINIMAL_STACK_SIZE, NULL, TEST_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
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
#if ( portSUPPORT_SMP == 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( portSUPPORT_SMP == 1 ) && ( configNUM_CORES == 2 )
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif ( RUN_FREE_RTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}
