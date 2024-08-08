.syntax unified
.cpu cortex-m33
.thumb

#include "hardware/dcp_instr.inc.S"
#include "hardware/dcp_canned.inc.S"

@ dot product
@ r0    pointer to array of n doubles p
@ r1    pointer to array of n doubles q
@ r2    n≥1
@ returns
@ r0:r1 dot product of p and q
.global dcp_dot
.thumb_func
dcp_dot:
    push {r4-r9,r14}
    ldrd r3,r4,[r0],#8                 @ fetch first pair of values
    ldrd r5,r6,[r1],#8
    dcp_dmul_m r7,r8, r3,r4,r5,r6, r3,r4,r5,r6,r12,r14,r9
    subs r2,#1
    beq 1f
2:
    ldrd r3,r4,[r0],#8                 @ fetch second and subsequent pairs of values
    ldrd r5,r6,[r1],#8
    dcp_dmul_m r3,r4, r3,r4,r5,r6, r3,r4,r5,r6,r12,r14,r9
    dcp_dadd_m r7,r8, r3,r4,r7,r8      @ multiply and accumulate
    subs r2,#1
    bne 2b
1:
    movs r0,r7                         @ double-precision result to r0:r1
    movs r1,r8
    pop {r4-r9,r15}

@ dot product with multiplications done exactly and accumulations done in double precision
@ r0    pointer to array of n floats p
@ r1    pointer to array of n floats q
@ r2    n≥1
@ returns
@ r0:r1 dot product of p and q
.global dcp_dotx
.thumb_func
dcp_dotx:
    push {r4,r14}
    ldr r3,[r0],#4                     @ fetch first pair of values
    ldr r4,[r1],#4
    dcp_fxprod_m r12,r14, r3,r4, r3,r4
    subs r2,#1
    beq 1f
2:
    ldr r3,[r0],#4                     @ fetch second and subsequent pairs of values
    ldr r4,[r1],#4
    dcp_fxprod_m r3,r4, r3,r4, r3,r4   @ multiply single-precision values to double-precision result (which will be exact)
    dcp_dadd_m r12,r14, r3,r4,r12,r14  @ accumulate in double precision
    subs r2,#1
    bne 2b
1:
    movs r0,r12                        @ double-precision result to r0:r1
    movs r1,r14
    pop {r4,r15}

@ IIR filter with intermediates computed in double precision
@ A filter of order n requires temporary storage of 2n words (or 8n bytes)
@ for internal delay buffers, which must be initialised to zero before the
@ first call. Coefficients are floats stored in the order b_n, a_n, b_n-1, a_n-1,
@ b_n-2, a_n-2, ..., b_0. Independent filters require separate temporary storage
@ areas but may share coefficients. "Direct form I" implementation.
@ r0 input x_t
@ r1 pointer to temporary storage
@ r2 pointer to coefficients
@ r3 order n≥1
@ returns
@ r0 output y_t
.global dcp_iirx
.thumb_func
dcp_iirx:
    push {r4-r7,r14}
    ldrd r4,r5,[r1]                    @ fetch x_t-n, y_t-n
    ldmia r2!,{r6,r7}                  @ fetch b_n, a_n
    dcp_fxprod_m r4,r6, r4,r6, r4,r6
    dcp_fxprod_m r5,r7, r5,r7, r5,r7
    dcp_dsub_m r12,r14, r4,r6,r5,r7    @ bx terms are added, ay terms are subtracted
    subs r3,#1
    beq 1f
2:
    ldrd r4,r5,[r1,#8]                 @ x_t-(n-i), y_t-(n-i)
    strd r4,r5,[r1],#8                 @ save delayed samples
    ldmia r2!,{r6,r7}                  @ b_n-i, a_n-i
    dcp_fxprod_m r4,r6, r4,r6, r4,r6
    dcp_dadd_m r12,r14, r12,r14,r4,r6  @ add bx term
    dcp_fxprod_m r5,r7, r5,r7, r5,r7
    dcp_dsub_m r12,r14, r12,r14,r5,r7  @ subtract ay term
    subs r3,#1
    bne 2b
1:
    ldr r6,[r2]                        @ fetch b_0
    dcp_fxprod_m r4,r6, r0,r6, r4,r6   @ final b_0 term
    dcp_dadd_m r12,r14, r12,r14,r4,r6
    dcp_double2float_m r4, r12,r14     @ convert accumulated result to float
    strd r0,r4,[r1]                    @ save x and y in delay buffer
    mov r0,r4                          @ return y
    pop {r4-r7,r15}

@ Macros to help with FFT steps

@ Given px pointing to a complex number x (two doubles, real part then imaginary part) and
@ py pointing to a complex number y, replace x with x+y and y with x-y
@ Write xr for the real part of x, xi for the imaginary part
@ Uses four temporary registers, t0..t3
.macro xpmy_m px,py,t0,t1,t2,t3
    ldrd \t2,\t3,[\px,#0]              @ fetch xr
    ldrd \t0,\t1,[\py,#0]              @ fetch yr
    dcp_dadd_m \t0,\t1, \t0,\t1,\t2,\t3
    strd \t0,\t1,[\px,#0]              @ write xr=xr+yr
    ldrd \t0,\t1,[\py,#0]              @ fetch yr again
    dcp_dsub_m \t0,\t1, \t2,\t3,\t0,\t1
    strd \t0,\t1,[\py,#0]              @ write yr=xr-yr
    ldrd \t2,\t3,[\px,#8]              @ fetch xi
    ldrd \t0,\t1,[\py,#8]              @ fetch yi
    dcp_dadd_m \t0,\t1, \t0,\t1,\t2,\t3
    strd \t0,\t1,[\px,#8]              @ write xi=xi+yi
    ldrd \t0,\t1,[\py,#8]              @ fetch yi again
    dcp_dsub_m \t0,\t1, \t2,\t3,\t0,\t1
    strd \t0,\t1,[\py,#8]              @ write yi=xi-yi
.endm

@ Given px pointing to a complex number x (two doubles, real part then imaginary part) and
@ py pointing to a complex number y, replace x with x-jy and y with x+jy
@ Write xr for the real part of x, xi for the imaginary part
@ Uses four temporary registers, t0..t3
.macro xmpjy_m px,py,t0,t1,t2,t3
    ldrd \t2,\t3,[\px,#0]              @ fetch xr
    ldrd \t0,\t1,[\py,#8]              @ fetch yi
    dcp_dadd_m \t0,\t1, \t0,\t1,\t2,\t3
    strd \t0,\t1,[\px,#0]              @ write xr=xr+yi
    ldrd \t0,\t1,[\py,#8]              @ fetch yi again
    dcp_dsub_m \t0,\t1, \t2,\t3,\t0,\t1
    ldrd \t2,\t3,[\py,#0]              @ fetch yr
    strd \t0,\t1,[\py,#0]              @ write yr=xr-yi
    ldrd \t0,\t1,[\px,#8]              @ fetch xi
    dcp_dadd_m \t0,\t1, \t0,\t1,\t2,\t3
    strd \t0,\t1,[\py,#8]              @ write yi=xi+yr
    ldrd \t0,\t1,[\px,#8]              @ fetch xi again
    dcp_dsub_m \t0,\t1, \t0,\t1,\t2,\t3
    strd \t0,\t1,[\px,#8]              @ write xi=xi-yr
.endm

@ r0 pointer to complex number x
@ r1 pointer to complex number y
@ replaces x with x+y, y with x-y
.global dcp_butterfly_radix2
.thumb_func
dcp_butterfly_radix2:
    push {r14}
    xpmy_m r0,r1,r12,r14,r2,r3
    pop {r15}

@ r0 pointer to complex number x
@ r1 pointer to complex number y
@ r2 pointer to complex number ω
@ replaces x with x+y, y with ω(x-y)
@ suitable for radix-2 decimation-in-frequency (DIF) FFT implementation
.global dcp_butterfly_radix2_twiddle_dif
.thumb_func
dcp_butterfly_radix2_twiddle_dif:
    push {r4-r10,r14}
    ldrd r3,r4,[r0,#0]                 @ fetch xr
    ldrd r5,r6,[r1,#0]                 @ fetch yr
    dcp_dadd_m r7,r8, r3,r4,r5,r6
    strd r7,r8,[r0,#0]                 @ write xr=xr+yr
    dcp_dsub_m r7,r8, r3,r4,r5,r6      @ now xr-yr is in r7:r8
    ldrd r3,r4,[r2,#0]                 @ fetch ωr
    dcp_dmul_m r3,r4, r3,r4,r7,r8, r3,r4,r5,r6,r9,r12,r14
    strd r3,r4,[r1,#0]                 @ write ωr(xr-yr) to yr for now
    ldrd r3,r4,[r2,#8]                 @ fetch ωi
    dcp_dmul_m r12,r14, r3,r4,r7,r8, r3,r4,r5,r6,r9,r12,r14
@ now ωi(xr-yr) is in r12:r14
    ldrd r3,r4,[r0,#8]                 @ fetch xi
    ldrd r5,r6,[r1,#8]                 @ fetch yi
    dcp_dadd_m r7,r8, r3,r4,r5,r6
    strd r7,r8,[r0,#8]                 @ write xi=xi+yi
@ we have now finished with r0
    dcp_dsub_m r7,r8, r3,r4,r5,r6      @ now xi-yi is in r7:r8
    ldrd r3,r4,[r2,#0]                 @ fetch ωr
    dcp_dmul_m r3,r4, r3,r4,r7,r8, r3,r4,r5,r6,r9,r10,r0
@ now ωr(xi-yi) is in r3:r4
    dcp_dadd_m r3,r4, r3,r4,r12,r14
    strd r3,r4,[r1,#8]                 @ write yi=ωr(xi-yi) + ωi(xr-yr)
    ldrd r3,r4,[r2,#8]                 @ ωi
    dcp_dmul_m r3,r4, r3,r4,r7,r8, r3,r4,r5,r6,r9,r12,r14
@ now ωi(xi-yi) is in r3:r4
    ldrd r5,r6,[r1,#0]                 @ fetch ωr(xr-yr)
    dcp_dsub_m r3,r4, r5,r6,r3,r4
    strd r3,r4,[r1,#0]                 @ write yr=ωr(xr-yr)-ωi(xi-yi)
    pop {r4-r10,r15}

@ r0 pointer to complex number x
@ r1 pointer to complex number y
@ r2 pointer to complex number ω
@ replaces x with x+ωy, y with x-ωy
@ suitable for radix-2 decimation-in-time (DIT) FFT implementation
.global dcp_butterfly_radix2_twiddle_dit
.thumb_func
dcp_butterfly_radix2_twiddle_dit:
    push {r4-r9,r14}
    ldrd r3,r4,[r2,#0]                 @ fetch ωr
    ldrd r5,r6,[r1,#0]                 @ fetch yr
    dcp_dmul_m r8,r9, r3,r4,r5,r6, r3,r4,r5,r6,r12,r14,r7
@ now ωryr is in r8:r9
    ldrd r3,r4,[r2,#8]                 @ fetch ωi
    ldrd r5,r6,[r1,#8]                 @ fetch yi
    dcp_dmul_m r3,r4, r3,r4,r5,r6, r3,r4,r5,r6,r12,r14,r7
@ now ωiyi is in r3:r4
    dcp_dsub_m r8,r9, r8,r9,r3,r4      @ ωryr-ωiyi=Re(ωy) in r8:r9
    ldrd r3,r4,[r0,#0]                 @ fetch xr
    dcp_dadd_m r5,r6, r3,r4,r8,r9      @ xr+Re(ωy)
    strd r5,r6,[r0,#0]                 @ write xr=xr+Re(ωy)
    dcp_dsub_m r3,r4, r3,r4,r8,r9      @ xr-Re(ωy)
    ldrd r5,r6,[r1,#0]                 @ fetch yr
    strd r3,r4,[r1,#0]                 @ write yr=xr-Re(ωy)
    ldrd r3,r4,[r2,#8]                 @ fetch ωi
    dcp_dmul_m r8,r9, r3,r4,r5,r6, r3,r4,r5,r6,r12,r14,r7
@ now ωiyr is in r8:r9
    ldrd r3,r4,[r2,#0]                 @ fetch ωr
    ldrd r5,r6,[r1,#8]                 @ fetch yi
    dcp_dmul_m r3,r4, r3,r4,r5,r6, r3,r4,r5,r6,r12,r14,r7
@ now ωryi is in r3:r4
    dcp_dadd_m r8,r9, r8,r9,r3,r4      @ ωiyr+ωryi=Im(ωy)
    ldrd r3,r4,[r0,#8]                 @ fetch xi
    dcp_dadd_m r5,r6, r3,r4,r8,r9      @ xi+Im(ωy)
    strd r5,r6,[r0,#8]                 @ write xi=xi+Im(ωy)
    dcp_dsub_m r3,r4, r3,r4,r8,r9      @ xi-Im(ωy)
    strd r3,r4,[r1,#8]                 @ write yi=xi-Im(ωy)
    pop {r4-r9,r15}

@ r0 pointer to complex number w
@ r1 pointer to complex number x
@ r2 pointer to complex number y
@ r3 pointer to complex number z
@ replaces
@ w with w +  x + y +  z = (w+y) +  (x+z)
@ x with w -  x + y -  z = (w+y) -  (x+z)
@ y with w - jx - y + jz = (w-y) - j(x-z)
@ z with w + jx - y - jz = (w-y) + j(x-z)
@ note that this introduces a bit reversal of indices à la radix 2
.global dcp_butterfly_radix4
.thumb_func
dcp_butterfly_radix4:                  @ @r0       @r1       @r2       @r3
    push {r4,r5,r14}                   @  w         x         y         z
    xpmy_m  r0,r2,r12,r14,r4,r5        @  w+y       x         w-y       z
    xpmy_m  r1,r3,r12,r14,r4,r5        @  w+y       x+z       w-y       x-z
    xpmy_m  r0,r1,r12,r14,r4,r5        @  w+x+y+z   w-x+y-z   w-y       x-z
    xmpjy_m r2,r3,r12,r14,r4,r5        @  w+x+y+z   w-x+y-z   w-jx-y+jz w+jx-y-jz
    pop {r4,r5,r15}
