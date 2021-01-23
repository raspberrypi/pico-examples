
#ifdef PICO_BUILD

/* 3. Implement the initilization function for clock. Leave it blank if not required. */
void my_clock_init(){
}

/* 4. Implement the function of getting current clock ticks. */
my_clock_t my_clock() {
    return to_ms_since_boot(get_absolute_time());
}

/* 5. Implement the idle delay function. */
void my_on_idle(uint64_t max_idle_ms) {
    sleep_ms(max_idle_ms);
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
    PUSH    {r4-r7}
    MOV     r2, r8
    MOV     r3, r9
    MOV     r4, r10
    MOV     r5, r11
    MOV     r6, r12
    MOV     r7, lr
    PUSH    {r2-r7}
    MOV     r2, sp
    STM     r0, {r2}

    LDM     r1, {r2}
    MOV     sp, r2
    POP     {r2-r7}
    MOV     r8, r2
    MOV     r9, r3
    MOV     r10, r4
    MOV     r11, r5
    MOV     r12, r6
    MOV     lr, r7
    POP     {r4-r7}

    BX      lr
}

#elif defined __GNUC__

__attribute__((naked))
static void swapcontext(ucontext_t *old_tcb, const ucontext_t *new_tcb) {
    __asm__ __volatile__(
        "PUSH    {r4-r7}\n"
        "MOV     r2, r8\n"
        "MOV     r3, r9\n"
        "MOV     r4, r10\n"
        "MOV     r5, r11\n"
        "MOV     r6, r12\n"
        "MOV     r7, lr\n"
        "PUSH    {r2-r7}\n"
        "MOV     r2, sp\n"
        "STM     r0!, {r2}\n"

        "LDM     r1!, {r2}\n"
        "MOV     sp, r2\n"
        "POP     {r2-r7}\n"
        "MOV     r8, r2\n"
        "MOV     r9, r3\n"
        "MOV     r10, r4\n"
        "MOV     r11, r5\n"
        "MOV     r12, r6\n"
        "MOV     lr, r7\n"
        "POP     {r4-r7}\n"
   );
}

#endif


static void create_context(ucontext_t *oucp, void *stack, size_t stack_size) {
    uint32_t *top_sp;
    uint32_t int_sp;

    int_sp = (int)((char *)stack + stack_size);
    int_sp = int_sp / 4 * 4;  //alignment
    top_sp = (uint32_t *)int_sp;
    
    top_sp[-5] = (int)&s_task_context_entry;
    oucp->sp = (int)&top_sp[-10];
}

