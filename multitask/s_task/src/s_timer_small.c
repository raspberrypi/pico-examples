/* Copyright xhawk, MIT license */

#include "s_task.h"

#ifdef USE_LIST_TIMER_CONTAINER

/*******************************************************************/
/* timer                                                           */
/*******************************************************************/

void s_timer_run() {
    my_clock_t current_ticks = my_clock();

    s_list_t *node = s_list_get_next(&g_globals.timers);
    while (node != &g_globals.timers) {
        s_list_t *node_next;
        s_timer_t *timer = GET_PARENT_ADDR(node, s_timer_t, node);

        my_clock_diff_t ticks_to_wakeup = (my_clock_diff_t)(timer->wakeup_ticks - current_ticks);
        if (ticks_to_wakeup > 0) break;

        node_next = s_list_get_next(node);

        s_list_detach(&timer->task->node);
        s_list_attach(&g_globals.active_tasks, &timer->task->node);

        timer->task = NULL;
        s_list_detach(node);
        node = node_next;
    }
}

uint64_t s_timer_wait_recent() {
    my_clock_t current_ticks = my_clock();
    s_list_t *node = s_list_get_next(&g_globals.timers);
    if (node != &g_globals.timers) {
        s_timer_t *timer = GET_PARENT_ADDR(node, s_timer_t, node);
        
        my_clock_diff_t ticks_to_wakeup = (my_clock_diff_t)(timer->wakeup_ticks - current_ticks);
        /* printf("ticks_to_wakeup = %d %d %d \n", ticks_to_wakeup, (int)current_ticks, (int)timer->wakeup_ticks); */
        if (ticks_to_wakeup > 0) {
            uint64_t wait = ((uint64_t)ticks_to_wakeup * 1000 / MY_CLOCKS_PER_SEC);
            return wait;
        }
        else
            return 0;
    }
    return (uint64_t)-1;    /* max value */
}

int s_task_sleep_ticks(__async__, my_clock_t ticks) {
    my_clock_t current_ticks;
    s_list_t *node;
    s_timer_t timer;
    int ret;

    current_ticks = my_clock();

    s_list_init(&timer.node);
    timer.task = g_globals.current_task;
    timer.wakeup_ticks = current_ticks + ticks;

    for(node = s_list_get_next(&g_globals.timers);
        node != &g_globals.timers;
        node = s_list_get_next(node)) {
        s_timer_t *timer = GET_PARENT_ADDR(node, s_timer_t, node);

        my_clock_diff_t ticks_to_wakeup = (my_clock_diff_t)(timer->wakeup_ticks - current_ticks);
        if (ticks_to_wakeup >= 0 && (my_clock_t)ticks_to_wakeup > ticks)
            break;
    }
    s_list_attach(node, &timer.node);

    s_list_detach(&timer.task->node);   /* no need, for safe */
    s_task_next(__await__);

    if (timer.task != NULL) {
        timer.task = NULL;
        s_list_detach(&timer.node);
    }

    ret = (g_globals.current_task->waiting_cancelled ? -1 : 0);
    g_globals.current_task->waiting_cancelled = false;
    return ret;
}


#endif
