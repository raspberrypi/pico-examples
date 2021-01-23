#include <vregs.inc>

    module  s_port_stm8s

save_registers    macro
    push    CC
    pushw    Y
    pushw    X
    push    A
    endm

restore_registers    macro
    pop    A
    popw    X
    popw    Y
    pop    CC
    endm

save_virtual_registers   macro
    push    ?b0
    push    ?b1
    push    ?b2
    push    ?b3
    push    ?b4
    push    ?b5
    push    ?b6
    push    ?b7
    push    ?b8
    push    ?b9
    push    ?b10
    push    ?b11
    push    ?b12
    push    ?b13
    push    ?b14
    push    ?b15
    endm

restore_virtual_registers  macro
    pop    ?b15
    pop    ?b14
    pop    ?b13
    pop    ?b12
    pop    ?b11
    pop    ?b10
    pop    ?b9
    pop    ?b8
    pop    ?b7
    pop    ?b6
    pop    ?b5
    pop    ?b4
    pop    ?b3
    pop    ?b2
    pop    ?b1
    pop    ?b0
    endm

;-------------------------------------------------------------------------------
    public  swapcontext

#if __CODE_MODEL__ == __SMALL_CODE_MODEL__
    section `.near_func.text`:CODE:NOROOT(0)
#else
    section `.far_func.text`:CODE:NOROOT(0)
#endif

;-------------------------------------------------------------------------------
;extern void swapcontext(ucontext_t *oucp, const ucontext_t *ucp);

swapcontext:
    save_registers
    save_virtual_registers

    ldw     Y,SP        ;save sp to Y
    ldw     (X),Y       ;save Y to oucp
    ldw     X,(20,SP)   ;load X from ucp
    ldw     X,(X)       ;load X from *ucp
    ldw     SP, X       ;save X to sp

    restore_virtual_registers
    restore_registers
        
#if __CODE_MODEL__ == __SMALL_CODE_MODEL__
    ret
#else
    retf
#endif

;-------------------------------------------------------------------------------

    end
;*******************************************************************************

