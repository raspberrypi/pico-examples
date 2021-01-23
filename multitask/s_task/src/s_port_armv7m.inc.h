
#ifdef ARDUINO

/* 3. Implement the initilization function for clock. Leave it blank if not required. */
void my_clock_init(){
}

/* 4. Implement the function of getting current clock ticks. */
my_clock_t my_clock() {
    return (my_clock_t)millis();
}

/* 5. Implement the idle delay function. */
void my_on_idle(uint64_t max_idle_ms) {
    delay(max_idle_ms);
}

#else


static my_clock_t g_ticks;
void SysTick_Handler(){
    ++g_ticks;
}

/* 3. Implement the initilization function for clock. Leave it blank if not required. */
void my_clock_init(){
    SysTick_Config(SystemCoreClock / MY_CLOCKS_PER_SEC);
}

/* 4. Implement the function of getting current clock ticks. */
my_clock_t my_clock() {
    return g_ticks;
}

/* 5. Implement the idle delay function. */
void my_on_idle(uint64_t max_idle_ms) {
    __WFE();
}

#endif


#if defined __ARMCC_VERSION
__asm static void swapcontext(ucontext_t *oucp, const ucontext_t *ucp) {
    PUSH    {r4-r12,lr}
    STR     sp, [r0]
    
    //LDR     r2, [r1]
    //MOV     sp, r2
    LDR     sp, [r1]
    POP     {r4-r12,lr}

    BX      lr
}

#elif defined __GNUC__

__attribute__((naked))
static void swapcontext(ucontext_t *old_tcb, const ucontext_t *new_tcb) {
    __asm__ __volatile__(
        "PUSH    {r4-r12,lr}\n"
        "STR     sp, [r0]\n"
        //"LDR     r2, [r1]\n"
        //"MOV     sp, r2\n"
        "LDR     sp, [r1]\n"
        "POP     {r4-r12,lr}\n"

        "BX      lr\n"
   );
}

#endif


static void create_context(ucontext_t *oucp, void *stack, size_t stack_size) {
    uint32_t *top_sp;
    uint32_t int_sp;

    int_sp = (int )((char *)stack + stack_size);
    int_sp = int_sp / 4 * 4;  //alignment
    top_sp = (uint32_t *)int_sp;
    
    top_sp[-1] = (int)&s_task_context_entry;
    oucp->sp = (int)&top_sp[-10];
}

