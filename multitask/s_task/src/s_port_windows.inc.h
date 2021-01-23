/* Copyright xhawk, MIT license */

/* Timer functions need to be implemented on a new porting. */

void my_clock_init(){
}

my_clock_t my_clock() {
    return GetTickCount();
}

void my_on_idle(uint64_t max_idle_ms) {
    Sleep((DWORD)max_idle_ms);
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

extern
transfer_t BOOST_CONTEXT_CALLDECL jump_fcontext( fcontext_t const to, void * vp);
extern
fcontext_t BOOST_CONTEXT_CALLDECL make_fcontext( void * sp, size_t size, void (* fn)( transfer_t) );


void create_fcontext(fcontext_t *fc, void * sp, size_t size, void (* fn)( transfer_t)) {
    sp = (void *)((char *)sp + size);
    *fc = make_fcontext(sp, size, fn);
}

