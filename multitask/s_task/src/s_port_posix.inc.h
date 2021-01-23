/* Copyright xhawk, MIT license */

/* Timer functions need to be implemented on a new porting. */

void my_clock_init(){
}

my_clock_t my_clock() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (my_clock_t)(tp.tv_sec * 1000 + tp.tv_nsec / 1000000);
}

void my_on_idle(uint64_t max_idle_ms) {
    usleep(max_idle_ms * 1000);
}


#if (defined(i386) || defined(__i386__) || defined(__i386) \
     || defined(__i486__) || defined(__i586__) || defined(__i686__) \
     || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) \
     || defined(__I86__) || defined(__INTEL__) || defined(__IA32__) \
     || defined(_M_IX86) || defined(_I86_)) && defined(_WIN32)
# define BOOST_CONTEXT_CALLDECL __cdecl
#else
# define BOOST_CONTEXT_CALLDECL
#endif


#ifdef USE_SWAP_CONTEXT
void create_context(ucontext_t *uc, void *stack, size_t stack_size) {
    getcontext(uc);
    uc->uc_stack.ss_sp = stack;
    uc->uc_stack.ss_size = stack_size;
    uc->uc_link = 0;
    makecontext(uc, (void (*)(void))&s_task_context_entry, 0);
}
#endif


#ifdef USE_JUMP_FCONTEXT
extern
transfer_t BOOST_CONTEXT_CALLDECL jump_fcontext( fcontext_t const to, void * vp);
extern
fcontext_t BOOST_CONTEXT_CALLDECL make_fcontext( void * sp, size_t size, void (* fn)( transfer_t) );

void create_fcontext(fcontext_t *fc, void *stack, size_t stack_size, void (* fn)( transfer_t)) {
    stack = (void *)((char *)stack + stack_size);
    *fc = make_fcontext(stack, stack_size, fn);
}
#endif

