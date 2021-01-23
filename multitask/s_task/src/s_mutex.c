/* Copyright xhawk, MIT license */

#include "s_task.h"

/*******************************************************************/
/* mutex                                                           */
/*******************************************************************/


/* Initialize a mutex */
void s_mutex_init(s_mutex_t *mutex) {
    s_list_init(&mutex->wait_list);
#ifdef USE_DEAD_TASK_CHECKING
    s_list_init(&mutex->self);
#endif
    mutex->locked = false;
}

/* Add the mutex to global waiting mutex list */
static void s_mutex_add_to_waiting_list(s_mutex_t *mutex) {
#ifdef USE_DEAD_TASK_CHECKING
    if(s_list_is_empty(&mutex->wait_list)) {
        s_list_detach(&mutex->self);
        s_list_attach(&g_globals.waiting_mutexes, &mutex->self);
    }
#endif
}

/* Remove the mutex from global waiting mutex list */
static void s_mutex_remove_from_waiting_list(s_mutex_t *mutex) {
#ifdef USE_DEAD_TASK_CHECKING
    if(s_list_is_empty(&mutex->wait_list)) {
        s_list_detach(&mutex->self);
    }
#endif
}

#ifdef USE_DEAD_TASK_CHECKING
/* Cancel dead waiting tasks */
unsigned int s_mutex_cancel_dead_waiting_tasks_() {
    s_list_t *next_mutex;
    s_list_t *this_mutex;
    unsigned int ret = 0;
    /* Check all mutexs */
    for(this_mutex = s_list_get_next(&g_globals.waiting_mutexes);
        this_mutex != &g_globals.waiting_mutexes;
        this_mutex = next_mutex) {
        s_list_t *next_task;
        s_list_t *this_task;
        s_mutex_t *mutex;
        
        next_mutex = s_list_get_next(this_mutex);
        s_list_detach(this_mutex);
        mutex = GET_PARENT_ADDR(this_mutex, s_mutex_t, self);
        
        /* Check all tasks blocked on this mutex */
        for(this_task = s_list_get_next(&mutex->wait_list);
            this_task != &mutex->wait_list;
            this_task = next_task) {
            next_task = s_list_get_next(this_task);
                    
            s_task_t *task = GET_PARENT_ADDR(this_task, s_task_t, node);
            s_task_cancel_wait(task);
            ++ret;
        }
    }

#ifndef NDEBUG
    if (ret > 0) {
        fprintf(stderr, "error: cancel dead tasks waiting on mutex!\n");
    }
#endif

    return ret;
}
#endif

/* Lock the mutex */
int s_mutex_lock(__async__, s_mutex_t *mutex) {
    if(mutex->locked) {
        int ret;
        /* Put current task to the mutex's waiting list */
        s_mutex_add_to_waiting_list(mutex);
        s_list_detach(&g_globals.current_task->node);   /* no need, for safe */
        s_list_attach(&mutex->wait_list, &g_globals.current_task->node);
        s_task_next(__await__);

        ret = (g_globals.current_task->waiting_cancelled ? -1 : 0);
        g_globals.current_task->waiting_cancelled = false;
        return ret;
    }
    else {
        mutex->locked = true;
        return 0;
    }
}

/* Unlock the mutex */
void s_mutex_unlock(s_mutex_t *mutex) {
    if(s_list_is_empty(&mutex->wait_list))
        mutex->locked = false;
    else {
        s_list_t *next = s_list_get_next(&mutex->wait_list);
        s_list_detach(next);
        s_list_attach(&g_globals.active_tasks, next);
        s_mutex_remove_from_waiting_list(mutex);        
    }
}


