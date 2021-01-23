#ifndef INC_S_TASK_H_
#define INC_S_TASK_H_

/* Copyright xhawk, MIT license */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "s_list.h"
#include "s_rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    int dummy;
} s_awaiter_t;

#   define __await__      __awaiter_dummy__
#   define __async__      s_awaiter_t *__awaiter_dummy__

/* Function type for task entrance */
typedef void(*s_task_fn_t)(__async__, void *arg);


/* #define USE_SWAP_CONTEXT                                            */
/* #define USE_JUMP_FCONTEXT                                           */
/* #define USE_LIST_TIMER_CONTAINER //for very small memory footprint  */
/* #define USE_IN_EMBEDDED                                             */
/* #define USE_STACK_DEBUG                                             */
/* #define USE_DEAD_TASK_CHECKING                                      */


#if defined __ARMCC_VERSION || (defined __GNUC__ && __USES_INITFINI__ && defined __ARM_ARCH)
#   define USE_IN_EMBEDDED
#   define USE_SWAP_CONTEXT
#   define USE_LIST_TIMER_CONTAINER
#   if defined ARDUINO
#       include "s_port_atsam3x8e.h"
#   elif defined STM32F10X_MD
#       include "s_port_stm32f10x.h"
#   elif defined STM32F302x8
#       include "s_port_stm32f30x.h"
#   elif defined STM32L1XX_MD
#       include "s_port_stm32l1xx.h"
#   elif defined PICO_BUILD
#       include "s_port_raspberry_pico.h"
#   else
#       include "s_port_m051.h"
#   endif
#elif defined __ICCSTM8__
#   define USE_IN_EMBEDDED
#   define USE_SWAP_CONTEXT
#   define USE_LIST_TIMER_CONTAINER
#   if defined STM8S103
#       include "s_port_stm8s.h"
#   elif defined STM8L05X_LD_VL 
#       include "s_port_stm8l15x.h"
#   endif
#elif defined USE_LIBUV
#   ifdef __CYGWIN__
#       define USE_SWAP_CONTEXT
#   else
#       define USE_JUMP_FCONTEXT
#   endif
#   define USE_DEAD_TASK_CHECKING
#   include "s_port_libuv.h"
#elif defined _WIN32
#   define USE_JUMP_FCONTEXT
#   define USE_DEAD_TASK_CHECKING
#   include "s_port_windows.h"
#elif defined __AVR__
#   define USE_IN_EMBEDDED
#   define USE_SWAP_CONTEXT
#   define USE_LIST_TIMER_CONTAINER
#   include "s_port_avr.h"
#else
#   ifdef __CYGWIN__
#       define USE_SWAP_CONTEXT
#   else
#       define USE_JUMP_FCONTEXT
#   endif
#   define USE_DEAD_TASK_CHECKING
#   include "s_port_posix.h"
#endif

#if defined USE_LIBUV && defined USE_IN_EMBEDDED
#   error "libuv can not used in embedded system"
#endif

typedef struct {
    s_list_t wait_list;
#ifdef USE_DEAD_TASK_CHECKING
    s_list_t self;
#endif
    bool locked;
} s_mutex_t;

typedef struct {
    s_list_t wait_list;
#ifdef USE_DEAD_TASK_CHECKING
    s_list_t self;
#endif
} s_event_t;

typedef struct {
    uint16_t max_count;
    uint16_t begin;
    uint16_t available_count;
    uint16_t element_size;
    s_event_t event;
} s_chan_t;


#include "s_task_internal.h"



/* Initialize the task system. */
#if defined USE_LIBUV
void s_task_init_system_(uv_loop_t* uv_loop);
#define s_task_init_system(uv_loop) __async__ = 0; (void)__awaiter_dummy__; s_task_init_system_(uv_loop)
#else
void s_task_init_system_(void);
#define s_task_init_system() __async__ = 0; (void)__awaiter_dummy__; s_task_init_system_()
#endif



/* Create a new task */
void s_task_create(void *stack, size_t stack_size, s_task_fn_t entry, void *arg);

/* Wait a task to exit */
int s_task_join(__async__, void *stack);

/* Kill a task */
void s_task_kill(void *stack);

/* Sleep in ticks */
int s_task_sleep_ticks(__async__, my_clock_t ticks);

/* Sleep in milliseconds */
int s_task_msleep(__async__, uint32_t msec);

/* Sleep in seconds */
int s_task_sleep(__async__, uint32_t sec);

/* Yield current task */
void s_task_yield(__async__);

/* Cancel task waiting and make it running */
void s_task_cancel_wait(void* stack);

/* Get free stack size (for debug) */
size_t s_task_get_stack_free_size(void);

/* Dump task information */
/* void dump_tasks(__async__); */

/* Initialize a mutex */
void s_mutex_init(s_mutex_t *mutex);

/* Lock the mutex */
int s_mutex_lock(__async__, s_mutex_t *mutex);

/* Unlock the mutex */
void s_mutex_unlock(s_mutex_t *mutex);

/* Initialize a wait event */
void s_event_init(s_event_t *event);

/* Set event */
void s_event_set(s_event_t *event);

/* Wait event
 *  return 0 on event set
 *  return -1 on event waiting cancelled
 */
int s_event_wait(__async__, s_event_t *event);

/* Wait event with timeout */
int s_event_wait_msec(__async__, s_event_t *event, uint32_t msec);

/* Wait event with timeout */
int s_event_wait_sec(__async__, s_event_t *event, uint32_t msec);


/* macro: Declare the chan variable
 *    name: name of the chan
 *    TYPE: type of element in the chan
 *    count: max count of element buffer in the chan
 */
#define s_chan_declare(name,TYPE,count)                                                         \
    s_chan_t name[1 + ((count)*sizeof(TYPE) + sizeof(s_chan_t) - 1) / sizeof(s_chan_t)]

/* macro: Initialize the chan (parameters same as what's in s_declare_chan).
 * To make a chan, we need to use "s_chan_declare" and then call "s_chan_init".
 */
#define s_chan_init(name,TYPE,count)    do {                                                    \
    (&name[0])->max_count       = (count);                                                      \
    (&name[0])->begin           = 0;                                                            \
    (&name[0])->available_count = 0;                                                            \
    (&name[0])->element_size    = sizeof(TYPE);                                                 \
    s_event_init(&(&name[0])->event);                                                           \
} while(0)

/* Put element into chan
 *  return 0 on chan put successfully
 *  return -1 on chan cancelled
 */
int s_chan_put(__async__, s_chan_t *chan, const void *in_object);

/* Put number of elements into chan
 *  return 0 on chan put successfully
 *  return -1 on chan cancelled
 */
int s_chan_put_n(__async__, s_chan_t *chan, const void *in_object, uint16_t number);

/* Get element from chan
 *  return 0 on chan get successfully
 *  return -1 on chan cancelled
 */
int s_chan_get(__async__, s_chan_t *chan, void *out_object);

/* Get number of elements from chan
 *  return 0 on chan get successfully
 *  return -1 on chan cancelled
 */
int s_chan_get_n(__async__, s_chan_t *chan, void *out_object, uint16_t number);

/* milliseconds to ticks */
my_clock_t msec_to_ticks(uint32_t msec);
/* seconds to ticks */
my_clock_t sec_to_ticks(uint32_t sec);
/* ticks to milliseconds */
uint32_t ticks_to_msec(my_clock_t ticks);
/* ticks to seconds */
uint32_t ticks_to_sec(my_clock_t ticks);


#ifdef USE_IN_EMBEDDED

/* Set event in interrupt */
void s_event_set__in_irq(s_event_t * event);

/*
 * Task waits event from irq. Disable irq before call this function!
 *   S_IRQ_DISABLE()
 *   ...
 *   s_event_wait__from_irq(...)
 *   ...
 *   S_IRQ_ENABLE()
 */
int s_event_wait__from_irq(__async__, s_event_t * event);

/*
 * Task waits event from irq. Disable irq before call this function!
 *   S_IRQ_DISABLE()
 *   ...
 *   s_event_wait_msec__from_irq(...)
 *   ...
 *   S_IRQ_ENABLE()
 */
int s_event_wait_msec__from_irq(__async__, s_event_t * event, uint32_t msec);

/*
 * Task waits event from irq. Disable irq before call this function!
 *   S_IRQ_DISABLE()
 *   ...
 *   s_event_wait_sec__from_irq(...)
 *   ...
 *   S_IRQ_ENABLE()
 */
int s_event_wait_sec__from_irq(__async__, s_event_t * event, uint32_t sec);


/* Task puts element into chan and waits interrupt to read the chan */
void s_chan_put__to_irq(__async__, s_chan_t *chan, const void *in_object);

/* Task puts number of elements into chan and waits interrupt to read the chan */
void s_chan_put_n__to_irq(__async__, s_chan_t *chan, const void *in_object, uint16_t number);

/* Task waits interrupt to write the chan and then gets element from chan */
void s_chan_get__from_irq(__async__, s_chan_t *chan, void *out_object);

/* Task waits interrupt to write the chan and then gets number of elements from chan */
void s_chan_get_n__from_irq(__async__, s_chan_t *chan, void *out_object, uint16_t number);


/*
 * Interrupt writes element into the chan,
 * return number of element was written into chan
 */
uint16_t s_chan_put__in_irq(s_chan_t *chan, const void *in_object);

/*
 * Interrupt writes number of elements into the chan,
 * return number of element was written into chan
 */
uint16_t s_chan_put_n__in_irq(s_chan_t *chan, const void *in_object, uint16_t number);

/*
 * Interrupt reads element from chan,
 * return number of element was read from chan
 */
uint16_t s_chan_get__in_irq(s_chan_t *chan, void *out_object);

/*
 * Interrupt reads number of elements from chan,
 * return number of element was read from chan
 */
uint16_t s_chan_get_n__in_irq(s_chan_t *chan, void *out_object, uint16_t number);


#endif

#ifdef __cplusplus
}
#endif

#include "s_uv.h"

#endif
