#ifndef INC_S_TASK_INTERNAL_H_
#define INC_S_TASK_INTERNAL_H_

/* Copyright xhawk, MIT license */

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************/
/* s_task type definitions                                         */
/*******************************************************************/

#ifdef USE_JUMP_FCONTEXT
typedef void* fcontext_t;
typedef struct {
    fcontext_t  fctx;
    void* data;
} transfer_t;
#endif

typedef struct tag_s_task_t {
    s_list_t     node;
    s_event_t    join_event;
    s_task_fn_t  task_entry;
    void        *task_arg;
#if defined   USE_SWAP_CONTEXT
    ucontext_t   uc;
#   ifdef __APPLE__
    char dummy[512]; /* it seems darwin ucontext has no enough memory ? */
#   endif
#elif defined USE_JUMP_FCONTEXT
    fcontext_t   fc;
#endif
    size_t       stack_size;
    bool         waiting_cancelled;
    bool         closed;
} s_task_t;

typedef struct {
#ifndef USE_LIST_TIMER_CONTAINER
    RBTNode    rbt_node;
#else
    s_list_t   node;
#endif
    s_task_t  *task;
    my_clock_t wakeup_ticks;
} s_timer_t;

#if defined USE_JUMP_FCONTEXT
typedef struct {
    fcontext_t *from;
    fcontext_t *to;
} s_jump_t;
#endif

typedef struct {
    s_task_t    main_task;
    s_list_t    active_tasks;
    s_task_t   *current_task;

#ifndef USE_LIST_TIMER_CONTAINER
    RBTree      timers;
#else
    s_list_t    timers;
#endif

#ifdef USE_DEAD_TASK_CHECKING
    s_list_t    waiting_mutexes;
    s_list_t    waiting_events;
#endif

#if defined USE_LIBUV
    uv_loop_t  *uv_loop;
    uv_timer_t  uv_timer;
#endif

#if defined USE_IN_EMBEDDED    
    s_list_t         irq_active_tasks;
    volatile uint8_t irq_actived;
#endif
} s_task_globals_t;

#if defined _MSC_VER
#   define THREAD_LOCAL __declspec(thread)
#elif defined __clang__
#   if __clang_major__ >= 2
#       define THREAD_LOCAL __thread
#   else
#       define THREAD_LOCAL
#   endif
#elif defined __GNUC__
#   define GNUC_VERSION_ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#   if GNUC_VERSION_ >= 30301
#       define THREAD_LOCAL __thread
#   else
#       define THREAD_LOCAL
#   endif
#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L
#   define THREAD_LOCAL _Thread_local
#else
#   define THREAD_LOCAL
#endif
extern THREAD_LOCAL s_task_globals_t g_globals;


struct tag_s_task_t;
/* */
void s_task_context_entry(void);
#ifdef USE_JUMP_FCONTEXT
void s_task_fcontext_entry(transfer_t arg);
#endif

/* Run next task, but not set myself for ready to run */
void s_task_next(__async__);

#if defined USE_LIBUV
/* Try to run next tasks, and return wait time if none need to run */
void s_task_main_loop_once(__async__);
#endif

void s_timer_run(void);
uint64_t s_timer_wait_recent(void);
int s_timer_comparator(const RBTNode* a, const RBTNode* b, void* arg);

uint16_t s_chan_put_(s_chan_t *chan, const void **in_object, uint16_t *number);
uint16_t s_chan_get_(s_chan_t *chan, void **out_object, uint16_t *number);

/* Return: number of cancelled tasks */
unsigned int s_task_cancel_dead(void);
#ifdef USE_DEAD_TASK_CHECKING
unsigned int s_event_cancel_dead_waiting_tasks_(void);
unsigned int s_mutex_cancel_dead_waiting_tasks_(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
