#ifndef INC_S_PORT_ARMV7M_H_
#define INC_S_PORT_ARMV7M_H_

#ifdef __cplusplus
extern "C" {
#endif

/* 1. define a type for clock */
typedef uint32_t my_clock_t;
typedef int32_t my_clock_diff_t;

typedef struct {
    int sp; //stack register
} ucontext_t;

/* 2. define the clock ticks count for one second */
#ifndef MY_CLOCKS_PER_SEC
#   define MY_CLOCKS_PER_SEC 1000
#endif

/* 3. Implement the initilization function for clock. Leave it blank if not required. */
void my_clock_init(void);

/* 4. Implement the function of getting current clock ticks. */
my_clock_t my_clock(void);

/* 5. Implement the idle delay function. */
void my_on_idle(uint64_t max_idle_ms);

/* 6. Define irq enable/disable functions */
#ifdef __GNUC__
__attribute__((always_inline))
#if __STDC_VERSION__ >= 199901L
inline
#endif
static void __set_PRIMASK_gcc(uint32_t primask) {
  __asm volatile ("MSR primask, %0" : : "r" (primask) : "memory");
}
#endif

__attribute__((always_inline))
#if __STDC_VERSION__ >= 199901L
inline
#endif
static void S_IRQ_DISABLE(){
#ifdef __GNUC__
    __set_PRIMASK_gcc(1);
#else
    __set_PRIMASK(1);
#endif
}

__attribute__((always_inline))
#if __STDC_VERSION__ >= 199901L
inline
#endif
static void S_IRQ_ENABLE(){
#ifdef __GNUC__
    __set_PRIMASK_gcc(0);
#else
    __set_PRIMASK(0);
#endif
}


#ifdef __cplusplus
}
#endif
#endif
