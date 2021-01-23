/* Copyright xhawk, MIT license */
#include <stdio.h>
#include "s_task.h"

/*
This program demonstrates three tasks:
 1) main_task -      Create two tasks, and then output main message repeatly.
 2) task_blinking -  Led blinking
 3) task_printf -    Output message repeatly
    
For full document of multitask library, please see here --
   https://github.com/xhawk18/s_task
*/

int g_stack0[2048 / sizeof(int)];
int g_stack1[2048 / sizeof(int)];

/* Task 0 */
void task_blink(__async__, void* arg) {
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    while (true) {
        gpio_put(LED_PIN, 1);
        s_task_msleep(__await__, 250);
        gpio_put(LED_PIN, 0);
        s_task_msleep(__await__, 250);
    }
}

/* Task 1 */
void task_printf(__async__, void* arg) {
    
    /* Output message */
    while (true) {
        printf("Hello, task_printf !\n");
        s_task_msleep(__await__, 300);
    }
}

/* Main task */
void main_task(__async__, void* arg) {

    /* create two sub tasks */
    s_task_create(g_stack0, sizeof(g_stack0), task_blink, NULL);
    s_task_create(g_stack1, sizeof(g_stack1), task_printf, NULL);

    /* Output message */
    while (true) {
        printf("Hello, main_task !\n");
        s_task_msleep(__await__, 400);
    }

}

int main() {

    s_task_init_system();

    stdio_init_all();
    main_task(__await__, NULL);
    
    return 0;
}
