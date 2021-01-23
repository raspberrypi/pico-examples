/* Copyright xhawk, MIT license */

#include "s_task.h"

#ifndef USE_LIST_TIMER_CONTAINER

/*******************************************************************/
/* timer                                                           */
/*******************************************************************/

int s_timer_comparator(const RBTNode* a, const RBTNode* b, void* arg) {
    s_timer_t* timer_a = GET_PARENT_ADDR(a, s_timer_t, rbt_node);
    s_timer_t* timer_b = GET_PARENT_ADDR(b, s_timer_t, rbt_node);
    (void)arg;

    my_clock_diff_t diff = (my_clock_diff_t)(timer_a->wakeup_ticks - timer_b->wakeup_ticks);
    if (diff != 0) {
        return (int)diff;
    }
    else {
        if (timer_a->task < timer_b->task)
            return -1;
        else if (timer_b->task < timer_a->task)
            return 1;
        else
            return 0;
    }
}

void dump_timers(int line) {
#if 0
    RBTreeIterator itr;
    RBTNode* node;
    RBTNode* node_next;
    my_clock_t current_ticks = my_clock();
    printf("=========================== %d\n", line);
    rbt_begin_iterate(&g_globals.timers, LeftRightWalk, &itr);
    node_next = rbt_iterate(&itr);
    while ((node = node_next) != NULL) {
        s_timer_t* timer = GET_PARENT_ADDR(node, s_timer_t, rbt_node);
        printf("timer: %p %d %d\n", timer->task, (int)timer->wakeup_ticks, (int)(timer->wakeup_ticks - current_ticks));
        node_next = rbt_iterate(&itr);
    }

    s_list_t* list;
    for (list = &g_globals.active_tasks.next; list != &g_globals.active_tasks; list = list->next) {
        s_task_t *task = GET_PARENT_ADDR(list, s_task_t, node);
        printf("task: %p\n", task);
    }
#else
    (void)line;
#endif
}

void s_timer_run(void) {
    my_clock_t current_ticks = my_clock();

    RBTreeIterator itr;
    RBTNode* node;
    RBTNode* node_next;
    
    dump_timers(__LINE__);

    rbt_begin_iterate(&g_globals.timers, LeftRightWalk, &itr);
    node_next = rbt_iterate(&itr);
    while ((node = node_next) != NULL) {

        s_timer_t* timer = GET_PARENT_ADDR(node, s_timer_t, rbt_node);

        my_clock_diff_t ticks_to_wakeup = (my_clock_diff_t)(timer->wakeup_ticks - current_ticks);
        if (ticks_to_wakeup > 0) break;

        node_next = rbt_iterate(&itr);
        
        s_list_detach(&timer->task->node);
        s_list_attach(&g_globals.active_tasks, &timer->task->node);

        timer->task = NULL;
        rbt_delete(&g_globals.timers, node);
    }

    dump_timers(__LINE__);

}

uint64_t s_timer_wait_recent(void) {
    my_clock_t current_ticks = my_clock();

    RBTreeIterator itr;
    RBTNode* node;
    rbt_begin_iterate(&g_globals.timers, LeftRightWalk, &itr);

    if ((node = rbt_iterate(&itr)) != NULL) {
        s_timer_t* timer = GET_PARENT_ADDR(node, s_timer_t, rbt_node);

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
    s_timer_t timer;
    
    current_ticks = my_clock();
    
    timer.task = g_globals.current_task;
    timer.wakeup_ticks = current_ticks + ticks;

    dump_timers(__LINE__);

    if (!rbt_insert(&g_globals.timers, &timer.rbt_node)) {
#ifndef NDEBUG
        fprintf(stderr, "timer insert failed!\n");
#endif
        return -1;
    }

    dump_timers(__LINE__);

    s_list_detach(&timer.task->node);   /* no need, for safe */
    s_task_next(__await__);

    dump_timers(__LINE__);

    if (timer.task != NULL) {
        timer.task = NULL;
        rbt_delete(&g_globals.timers, &timer.rbt_node);
    }

    int ret = (g_globals.current_task->waiting_cancelled ? -1 : 0);
    g_globals.current_task->waiting_cancelled = false;
    return ret;
}


#endif
