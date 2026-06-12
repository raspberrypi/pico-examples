#include <stdio.h>

#include "pico/stdio.h"
#include "pico/multicore.h"
#include "pico/flash.h"

#include "FreeRTOS.h"
#include "task.h"

#define START_CORE_1 0

extern void run_ble_pointer(void);

#define MAIN_TASK_PRIORITY ( tskIDLE_PRIORITY + 2UL )
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

static void main_task_core0(__unused void *params) {
    run_ble_pointer();
}

static void vLaunch0( void) {
#if 1
    // todo: is this right?
    flash_safe_execute_core_init();
#endif

    TaskHandle_t task;
    static_assert(configSUPPORT_DYNAMIC_ALLOCATION, "");
    xTaskCreate(main_task_core0, "MainThread_core0", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &task);
#if configNUMBER_OF_CORES > 1
    vTaskCoreAffinitySet(task, 1<<0);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

#if START_CORE_1
static void main_task_core1(__unused void *params) {
    printf("Hello from core1");
    while(true) {
        sleep_ms(100);
    }
}

void vLaunch1( void) {

    TaskHandle_t task;
    static_assert(configSUPPORT_DYNAMIC_ALLOCATION, "");
    xTaskCreate(main_task_core1, "MainThread_core1", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &task);
    vTaskCoreAffinitySet(task, 1<<1);

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}
#endif

int main(void) {
    stdio_init_all();
    vLaunch0();
#if START_CORE_1
    multicore_launch_core1(vLaunch1);
#endif
    return 0;
}

