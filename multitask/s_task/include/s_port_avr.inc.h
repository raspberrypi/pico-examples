/* Copyright xhawk, MIT license */
#ifdef ARDUINO
#   include <Arduino.h>
#else
#   include <avr/io.h>
#endif

/* Timer functions need to be implemented on a new porting. */
#ifdef ARDUINO

void my_clock_init() {
}

my_clock_t my_clock() {
    return (my_clock_t)millis();
}

void my_on_idle(uint64_t max_idle_ms) {
    delay(max_idle_ms);
}

#else

static volatile my_clock_t g_sys_ticks;

void my_clock_init(){
#ifndef F_CPU
/* Atmega328p CPU frequency is 16MHZ */
#   define F_CPU 16000000UL
#endif
    /* Set prescaler 256 */
    TCCR1B = _BV(CS12) | _BV(WGM12);
    /* For Arduino Uno CPU 16000000 HZ, so the OCR1A should count from 0 to 624
     x * 1/16M * 256 = 10 ms = 0.01 s
     x = 16 M / 100 / 256 = 625
    */
    OCR1A = (F_CPU / 256 / MY_CLOCKS_PER_SEC) - 1;
    /* enable compare match 1A interrupt */
    TIMSK1 = _BV(OCIE1A);
    
    sei();
}

my_clock_t my_clock() {
    return g_sys_ticks;
}

void my_on_idle(uint64_t max_idle_ms) {
}

/* interrupt every SYS_TICK to re-schedule tasks */
ISR(TIMER1_COMPA_vect) {
    ++g_sys_ticks;
}

#endif

static void create_context(ucontext_t *uc, void *stack, size_t stack_size) {
    uint8_t *stack_top = (uint8_t *)stack + stack_size;
    /* pc, the function pointer is uint16_t in avr */
    stack_top[-1] = (uint8_t)((uint16_t)s_task_context_entry & 0xFF);         /* the LSB */
    stack_top[-2] = (uint8_t)(((uint16_t)s_task_context_entry >> 8) & 0xFF);  /* THE MSB */
    uc->sp = (int)(stack_top - 21);
}

static void swapcontext(ucontext_t *old_tcb, const ucontext_t *new_tcb) __attribute__ ((naked)); 
static void swapcontext(ucontext_t *old_tcb, const ucontext_t *new_tcb) {
  /* The assembly code is in intel style, source is always on the right */
  /* Y-reg is R28 and R29 */
  __asm__ __volatile__(
      "push r2\n\t"
      "push r3\n\t"
      "push r4\n\t"
      "push r5\n\t"
      "push r6\n\t"
      "push r7\n\t"
      "push r8\n\t"
      "push r9\n\t"
      "push r10\n\t"
      "push r11\n\t"
      "push r12\n\t"
      "push r13\n\t"
      "push r14\n\t"
      "push r15\n\t"
      "push r16\n\t"
      "push r17\n\t"
      "push r28\n\t"
      "push r29\n\t"

      /* r16, r17, r28 and r29 is saved, we're safe to use them */
      "mov r28, %A[_old_tcb_]\n\t" /* move old tcb(LSB) to Y-regs */
      "mov r29, %B[_old_tcb_]\n\t" /* MSB */
      "in r16, %[_SPL_]\n\t"       /* get stack pointer to r17:r16 */
      "in r17, %[_SPH_]\n\t"
      "st Y, r16\n\t"              /* set old_tcb->sp to stack pointer */
      "std Y+1, r17\n\t"           /* because sp is the first member of the TCB struct */

      "mov r28, %A[_new_tcb_]\n\t"
      "mov r29, %B[_new_tcb_]\n\t"
      "ld r16, Y\n\t"              /* load new_tcb->sp to r17:r16 */
      "ldd r17, Y+1\n\t"
      "out %[_SPL_], r16\n\t"      /* change the stack pointer to new_tcb->sp */
      "out %[_SPH_], r17\n\t"

      "pop r29\n\t"                /* restore new_tcb's context */
      "pop r28\n\t"
      "pop r17\n\t"
      "pop r16\n\t"
      "pop r15\n\t"
      "pop r14\n\t"
      "pop r13\n\t"
      "pop r12\n\t"
      "pop r11\n\t"
      "pop r10\n\t"
      "pop r9\n\t"
      "pop r8\n\t"
      "pop r7\n\t"
      "pop r6\n\t"
      "pop r5\n\t"
      "pop r4\n\t"
      "pop r3\n\t"
      "pop r2\n\t"
      
      "ret\n\t"
      "" ::
      [_SPL_] "i" _SFR_IO_ADDR(SPL),
      [_SPH_] "i" _SFR_IO_ADDR(SPH),
      [_old_tcb_] "r"(old_tcb),
      [_new_tcb_] "r"(new_tcb)
  );
}
