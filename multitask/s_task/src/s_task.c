/* Copyright xhawk, MIT license */

#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "s_task.h"
#include "s_list.h"
#include "s_rbtree.h"

#define S_TASK_STACK_MAGIC ((int)0x5AA55AA5)
THREAD_LOCAL s_task_globals_t g_globals;

#if defined __GNUC__ && __USES_INITFINI__ && defined __ARM_ARCH
#   if __ARM_ARCH == 7
#       include "s_port_armv7m.inc.h"
#   elif __ARM_ARCH == 6
#       include "s_port_armv6m.inc.h"
#   else
#       error "no arch detected"
#   endif
#elif defined __ARMCC_VERSION
#   if defined __TARGET_CPU_CORTEX_M0
#       include "s_port_armv6m.inc.h"
#   elif defined __TARGET_CPU_CORTEX_M3
#       include "s_port_armv7m.inc.h"
#   elif defined __TARGET_CPU_CORTEX_M4
#       include "s_port_armv7m.inc.h"
#   elif defined __TARGET_CPU_CORTEX_M4_FP
#       include "s_port_armv7m.inc.h"
#   else
#       error "no arch detected"
#   endif
#elif defined __ICCSTM8__
#   if defined STM8S103
#       include "s_port_stm8s.inc.h"
#   elif defined STM8L05X_LD_VL 
#       include "s_port_stm8l15x.inc.h"
#   endif
#elif defined USE_LIBUV
#   include "s_port_libuv.inc.h"
#elif defined _WIN32
#   include "s_port_windows.inc.h"
#elif defined __AVR__
#   include "s_port_avr.inc.h"
#else
#   include "s_port_posix.inc.h"
#endif


#if defined USE_LIBUV
static uv_loop_t* s_task_uv_loop() {
    return g_globals.uv_loop;
}
static uv_timer_t* s_task_uv_timer() {
    return &g_globals.uv_timer;
}
#endif




/*******************************************************************/
/* tasks                                                           */
/*******************************************************************/


int s_task_msleep(__async__, uint32_t msec) {
    my_clock_t ticks = msec_to_ticks(msec);
    return s_task_sleep_ticks(__await__, ticks);
}

int s_task_sleep(__async__, uint32_t sec) {
    my_clock_t ticks = sec_to_ticks(sec);
    return s_task_sleep_ticks(__await__, ticks);
}

my_clock_t msec_to_ticks(uint32_t msec) {
    uint64_t ret = (uint64_t)((uint64_t)msec * (1024 * (uint64_t)MY_CLOCKS_PER_SEC / 1000) / 1024);
    if(ret >= ((my_clock_t)-1))
        return (my_clock_t)-1;
    else
        return (my_clock_t)ret;
}

my_clock_t sec_to_ticks(uint32_t sec) {
    uint64_t ret = (uint64_t)((uint64_t)sec * (uint64_t)MY_CLOCKS_PER_SEC);
    if(ret >= (my_clock_t)-1)
        return (my_clock_t)-1;
    else
        return (my_clock_t)ret;
}

#define TICKS_DEVIDER   (uint32_t)(4096*1024)
#define TICKS_PER_SEC_1 (uint32_t)(TICKS_DEVIDER / MY_CLOCKS_PER_SEC)

uint32_t ticks_to_msec(my_clock_t ticks) {
#if INT_MAX < 65536 /* it seems that stm8 uint64 is uint32 */
    uint32_t msec = 1000 * (uint32_t)ticks / MY_CLOCKS_PER_SEC;
    return msec;
#else
    uint64_t u64_msec = 1000 * (uint64_t)ticks * TICKS_PER_SEC_1 / TICKS_DEVIDER;
    uint32_t msec = (u64_msec > (uint64_t)~(uint32_t)0
        ? ~(uint32_t)0 : (uint32_t)u64_msec);
    return msec;
#endif
}

uint32_t ticks_to_sec(my_clock_t ticks) {
#if INT_MAX < 65536 /* it seems that stm8 uint64 is uint32 */
    uint32_t sec = (uint32_t)ticks / MY_CLOCKS_PER_SEC;
    return sec;
#else
    uint64_t u64_sec = (uint64_t)ticks * TICKS_PER_SEC_1 / TICKS_DEVIDER;
    uint32_t sec = (u64_sec > (uint64_t)~(uint32_t)0
        ? ~(uint32_t)0 : (uint32_t)u64_sec);
    return sec;
#endif
}

/* 
    *timeout, wait timeout
    return, true on task run
 */
static void s_task_call_next(__async__) {
    /* get next task and run it */
    s_list_t* next;
    s_task_t* old_task;

    (void)__awaiter_dummy__;

    /* Check active tasks */
    if (s_list_is_empty(&g_globals.active_tasks)) {
#ifndef NDEBUG
        fprintf(stderr, "error: must has one task to run\n");
#endif
        return;
    }

    old_task = g_globals.current_task;
    next = s_list_get_next(&g_globals.active_tasks);
    
    /* printf("next = %p %p\n", g_globals.current_task, next); */

    g_globals.current_task = GET_PARENT_ADDR(next, s_task_t, node);
    s_list_detach(next);

    if (old_task != g_globals.current_task) {
#if defined   USE_SWAP_CONTEXT
        swapcontext(&old_task->uc, &g_globals.current_task->uc);
#elif defined USE_JUMP_FCONTEXT
        s_jump_t jump;
        jump.from = &old_task->fc;
        jump.to = &g_globals.current_task->fc;
        transfer_t t = jump_fcontext(g_globals.current_task->fc, (void*)&jump);
        s_jump_t* ret = (s_jump_t*)t.data;
        *ret->from = t.fctx;
#endif
    }

#ifdef USE_STACK_DEBUG
    if(g_globals.current_task->stack_size > 0) {
        s_task_t* task = g_globals.current_task;
        void *real_stack = (void *)&task[1];
        size_t real_stack_size = task->stack_size - sizeof(task[0]);
        size_t int_stack_size = real_stack_size / sizeof(int);
        if(((int *)real_stack)[0] != S_TASK_STACK_MAGIC) {
#ifndef NDEBUG
            fprintf(stderr, "stack overflow in lower bits");
#endif
            while(1);   /* dead */
        }
        if(((int *)real_stack)[int_stack_size - 1] != S_TASK_STACK_MAGIC) {
#ifndef NDEBUG
            fprintf(stderr, "stack overflow in higher bits");
#endif
            while(1);   /* dead */
        }
    }
#endif    
}

#if defined USE_LIBUV
void s_task_next(__async__) {
    g_globals.current_task->waiting_cancelled = false;
    s_timer_run();
    s_task_call_next(__await__);
}

void s_task_main_loop_once(__async__) {
    s_timer_run();
    while (!s_list_is_empty(&g_globals.active_tasks)) {
        /* Put current task to the waiting list */
        s_list_attach(&g_globals.active_tasks, &g_globals.current_task->node);
        s_task_call_next(__await__);
        s_timer_run();
    }

    /* Check timers */
    if (!rbt_is_empty(&g_globals.timers)) {
        /* Wait for the recent timer */
        uint64_t timeout = s_timer_wait_recent();
        my_on_idle(timeout);
    }
}

#else

void s_task_next(__async__) {
    g_globals.current_task->waiting_cancelled = false;
    while (true) {
#ifdef USE_IN_EMBEDDED
        if(g_globals.irq_actived){
            S_IRQ_DISABLE();
            g_globals.irq_actived = 0;
            s_list_attach(&g_globals.active_tasks, &g_globals.irq_active_tasks);
            s_list_detach(&g_globals.irq_active_tasks);
            S_IRQ_ENABLE();
        }
#endif

        s_timer_run();
        if (!s_list_is_empty(&g_globals.active_tasks)) {
            s_task_call_next(__await__);
            return;
        }

        /* Check timers */
#ifndef USE_LIST_TIMER_CONTAINER
        if (!rbt_is_empty(&g_globals.timers)) {
#else
        if (!s_list_is_empty(&g_globals.timers)) {
#endif
            /* Wait for the recent timer */
            uint64_t timeout = s_timer_wait_recent();
            my_on_idle(timeout);
        }
        else if(s_task_cancel_dead() == 0) {

#ifndef NDEBUG
            fprintf(stderr, "error: must not wait so long!\n");
#endif
            my_on_idle((uint64_t)-1);
        }
    }
}

#endif

void s_task_yield(__async__) {
    /* Put current task to the waiting list */
    s_list_attach(&g_globals.active_tasks, &g_globals.current_task->node);
    s_task_next(__await__);
    g_globals.current_task->waiting_cancelled = false;
}

#if defined USE_LIBUV
void s_task_init_system_(uv_loop_t* uv_loop)
#else
void s_task_init_system_()
#endif
{
#if defined USE_LIBUV
    g_globals.uv_loop = uv_loop;
#endif
    
#if defined USE_IN_EMBEDDED    
    s_list_init(&g_globals.irq_active_tasks);
    g_globals.irq_actived = 0;
#endif

    s_list_init(&g_globals.active_tasks);
#ifdef USE_DEAD_TASK_CHECKING	
    s_list_init(&g_globals.waiting_mutexes);
    s_list_init(&g_globals.waiting_events);
#endif

#ifndef USE_LIST_TIMER_CONTAINER
    rbt_create(&g_globals.timers,
        s_timer_comparator,
        NULL
    );
#else
    s_list_init(&g_globals.timers);
#endif

    my_clock_init();

    s_list_init(&g_globals.main_task.node);
    s_event_init(&g_globals.main_task.join_event);
    g_globals.main_task.stack_size = 0;
    g_globals.main_task.closed = false;
    g_globals.main_task.waiting_cancelled = false;
    g_globals.current_task = &g_globals.main_task;
}

void s_task_create(void *stack, size_t stack_size, s_task_fn_t task_entry, void *task_arg) {
    void *real_stack;
    size_t real_stack_size;

    s_task_t *task = (s_task_t *)stack;
    s_list_init(&task->node);
    s_event_init(&task->join_event);
    task->task_entry = task_entry;
    task->task_arg   = task_arg;
    task->stack_size = stack_size;
    task->closed = false;
    task->waiting_cancelled = false;
    s_list_attach(&g_globals.active_tasks, &task->node);

    real_stack = (void *)&task[1];
    real_stack_size = stack_size - sizeof(task[0]);
#ifdef USE_STACK_DEBUG
    {
        /* Fill magic number so as to check stack size */
        size_t int_stack_size = real_stack_size / sizeof(int);
        if(int_stack_size <= 2) {
#ifndef NDEBUG
            fprintf(stderr, "stack size too small");
            return;
#endif
        }
        ((int *)real_stack)[0] = S_TASK_STACK_MAGIC;
        ((int *)real_stack)[int_stack_size - 1] = S_TASK_STACK_MAGIC;
        real_stack = (char *)real_stack + sizeof(int);
        real_stack_size = (int_stack_size - 2) * sizeof(int);
    }
#endif

#if defined   USE_SWAP_CONTEXT
    create_context(&task->uc, real_stack, real_stack_size);
#elif defined USE_JUMP_FCONTEXT
    create_fcontext(&task->fc, real_stack, real_stack_size, s_task_fcontext_entry);
#endif
}

int s_task_join(__async__, void *stack) {
    s_task_t *task = (s_task_t *)stack;
    while (!task->closed) {
        int ret = s_event_wait(__await__, &task->join_event);
        if (ret != 0)
            return ret;
    }
    return 0;
}

/* timer conflict with this function!!!
   Do NOT call s_task_kill, and let the task exit by itself! */
void s_task_kill__remove(void *stack) {
    s_task_t *task = (s_task_t *)stack;
    s_list_detach(&task->node);
}

void s_task_cancel_wait(void* stack) {
    s_task_t* task = (s_task_t*)stack;

    task->waiting_cancelled = true;
    s_list_detach(&task->node);
    s_list_attach(&g_globals.active_tasks, &task->node);
}

unsigned int s_task_cancel_dead() {
#ifdef USE_DEAD_TASK_CHECKING
    return s_event_cancel_dead_waiting_tasks_()
         + s_mutex_cancel_dead_waiting_tasks_();
#else
    return 0;
#endif
}

static size_t s_task_get_stack_free_size_ex_by_stack(void *stack) {
    uint32_t *check;
    for(check = (uint32_t *)stack; ; ++check){
        if(*check != 0xFFFFFFFF)
            return (char *)check - (char *)stack;
    }
}

static size_t s_task_get_stack_free_size_by_task(s_task_t *task) {
    return s_task_get_stack_free_size_ex_by_stack(&task[1]);
}

size_t s_task_get_stack_free_size() {
    return s_task_get_stack_free_size_by_task(g_globals.current_task);
}

void s_task_context_entry() {
    struct tag_s_task_t *task = g_globals.current_task;
    s_task_fn_t task_entry = task->task_entry;
    void *task_arg         = task->task_arg;

    __async__ = 0;
    (*task_entry)(__await__, task_arg);

    task->closed = true;
    s_event_set(&task->join_event);
    s_task_next(__await__);
}


#ifdef USE_JUMP_FCONTEXT
void s_task_fcontext_entry(transfer_t arg) {
    /* printf("=== s_task_helper_entry = %p\n", arg.fctx); */

    s_jump_t* jump = (s_jump_t*)arg.data;
    *jump->from = arg.fctx;
    /* printf("%p %p %p\n", jump, jump->to, g_globals.current_task); */

    s_task_context_entry();
}
#endif


