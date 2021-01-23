#include "s_task.h"

/*******************************************************************/
/* event                                                           */
/*******************************************************************/

/* Initialize a wait event */
void s_event_init(s_event_t *event) {
    s_list_init(&event->wait_list);
#ifdef USE_DEAD_TASK_CHECKING
    s_list_init(&event->self);
#endif
}

/* Add the event to global waiting event list */
static void s_event_add_to_waiting_list(s_event_t *event) {
#ifdef USE_DEAD_TASK_CHECKING
    if(s_list_is_empty(&event->wait_list)) {
        s_list_detach(&event->self);
        s_list_attach(&g_globals.waiting_events, &event->self);
    }
#endif
}

/* Remove the event from global waiting event list */
static void s_event_remove_from_waiting_list(s_event_t *event) {
#ifdef USE_DEAD_TASK_CHECKING
    if(s_list_is_empty(&event->wait_list)) {
        s_list_detach(&event->self);
    }
#endif
}

#ifdef USE_DEAD_TASK_CHECKING
/* Cancel dead waiting tasks */
unsigned int s_event_cancel_dead_waiting_tasks_() {
    s_list_t *next_event;
    s_list_t *this_event;
    unsigned int ret = 0;
    /* Check all events */
    for(this_event = s_list_get_next(&g_globals.waiting_events);
        this_event != &g_globals.waiting_events;
        this_event = next_event) {
        s_list_t *next_task;
        s_list_t *this_task;
        s_event_t *event;
        
        next_event = s_list_get_next(this_event);
        s_list_detach(this_event);
        event = GET_PARENT_ADDR(this_event, s_event_t, self);
        
        /* Check all tasks blocked on this event */
        for(this_task = s_list_get_next(&event->wait_list);
            this_task != &event->wait_list;
            this_task = next_task) {
            next_task = s_list_get_next(this_task);
                    
            s_task_t *task = GET_PARENT_ADDR(this_task, s_task_t, node);
            s_task_cancel_wait(task);
            ++ret;
        }
    }

#ifndef NDEBUG
    if (ret > 0) {
        fprintf(stderr, "error: cancel dead tasks waiting on event!\n");
    }
#endif

    return ret;
}
#endif

/* Wait event
 *  return 0 on event set
 *  return -1 on event waiting cancelled
 */
int s_event_wait(__async__, s_event_t *event) {
    int ret;
    /* Put current task to the event's waiting list */
    s_event_add_to_waiting_list(event);
    s_list_detach(&g_globals.current_task->node);   /* no need, for safe */
    s_list_attach(&event->wait_list, &g_globals.current_task->node);
    s_task_next(__await__);
    ret = (g_globals.current_task->waiting_cancelled ? -1 : 0);
    g_globals.current_task->waiting_cancelled = false;
    return ret;
}

/* Set event */
void s_event_set(s_event_t *event) {
    s_list_attach(&g_globals.active_tasks, &event->wait_list);
    s_list_detach(&event->wait_list);
    s_event_remove_from_waiting_list(event);
}

/* Wait event */
#ifndef USE_LIST_TIMER_CONTAINER
static int s_event_wait_ticks(__async__, s_event_t *event, my_clock_t ticks) {
    my_clock_t current_ticks;
    s_timer_t timer;
    int ret;
    
    current_ticks = my_clock();
    timer.task = g_globals.current_task;
    timer.wakeup_ticks = current_ticks + ticks;

    if (!rbt_insert(&g_globals.timers, &timer.rbt_node)) {
#ifndef NDEBUG
        fprintf(stderr, "timer insert failed!\n");
#endif
        return -1;
    }

    s_event_add_to_waiting_list(event);
    s_list_detach(&g_globals.current_task->node);   /* no need, for safe */
    /* Put current task to the event's waiting list */
    s_list_attach(&event->wait_list, &g_globals.current_task->node);
    s_task_next(__await__);

    if (timer.task != NULL) {
        timer.task = NULL;
        rbt_delete(&g_globals.timers, &timer.rbt_node);
    }

    ret = (g_globals.current_task->waiting_cancelled ? -1 : 0);
    g_globals.current_task->waiting_cancelled = false;
    return ret;
}
#else
static int s_event_wait_ticks(__async__, s_event_t *event, my_clock_t ticks) {
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

    s_event_add_to_waiting_list(event);
    s_list_detach(&timer.task->node);   /* no need, for safe */
    /* Put current task to the event's waiting list */
    s_list_attach(&event->wait_list, &g_globals.current_task->node);
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

/* Wait event */
int s_event_wait_msec(__async__, s_event_t *event, uint32_t msec) {
    my_clock_t ticks = msec_to_ticks(msec);
    return s_event_wait_ticks(__await__, event, ticks);
}

/* Wait event */
int s_event_wait_sec(__async__, s_event_t *event, uint32_t sec) {
    my_clock_t ticks = sec_to_ticks(sec);
    return s_event_wait_ticks(__await__, event, ticks);
}
