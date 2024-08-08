.syntax unified
.cpu cortex-m33
.thumb

#include "hardware/platform_defs.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sha256.h"
#include "hardware/rcp.h"

#include "config.h"

.global delay
.global aes_start
.global aes_end
.global flush_reg
.global isr_systick
.extern systick_data

.global gen_lut_inverse
.global gen_lut_sbox
.if NEED_INV_ROUNDS
.global gen_lut_inv_sbox
.endif

.if INCLUDE_ENCRYPT_CBC
.global cbc_encrypt_s
.endif
.if INCLUDE_DECRYPT_CBC
.global cbc_decrypt_s
.endif
.if INCLUDE_CRYPT_CTR
.global ctr_crypt_s
.endif

.global remap
.global gen_rand
.global init_key

.global rkey_s
.global lut_a,lut_a_map
.global lut_b,lut_b_map
.global rstate

@ RCP macros

#define CTAG0  0x2a
#define CTAG1  0x2b
#define CTAG2  0x2c
#define CTAG3  0x2d
#define CTAG4  0x2e
#define CTAG5  0x30
#define CTAG6  0x31
#define CTAG7  0x32
#define CTAG8  0x33
#define CTAG9  0x34
#define CTAG10 0x35
#define CTAG11 0x36
#define CTAG12 0x37
#define CTAG13 0x38
#define CTAG14 0x39
#define CTAG15 0x3a
#define CTAG16 0x3b
#define CTAG17 0x3c

.macro SET_COUNT n
.if RC_COUNT
.if RC_JITTER
 rcp_count_set \n
.else
 rcp_count_set_nodelay \n
.endif
.endif
.endm

.macro CHK_COUNT n
.if RC_COUNT
.if RC_JITTER
 rcp_count_check \n
.else
 rcp_count_check_nodelay \n
.endif
.endif
.endm

.macro GET_CANARY rx,tag
.if RC_CANARY
.if RC_JITTER
 rcp_canary_get \rx,\tag
.else
 rcp_canary_get_nodelay \rx,\tag
.endif
.endif
.endm

.macro CHK_CANARY rx,tag
.if RC_CANARY
.if RC_JITTER
 rcp_canary_check \rx,\tag
.else
 rcp_canary_check_nodelay \rx,\tag
.endif
.endif
.endm

.macro GET_CANARY_NJ rx,tag  @ with no jitter even if you ask for it (otherwise slows down gen_rand a lot)
.if RC_CANARY
 rcp_canary_get_nodelay \rx,\tag
.endif
.endm

.macro CHK_CANARY_NJ rx,tag  @ with no jitter even if you ask for it
.if RC_CANARY
 rcp_canary_check_nodelay \rx,\tag
.endif
.endm

.section .stack.aes
@ Regardless of configuration the code uses a single 256-entry LUT. If both
@ encryption and decryption are enabled then this is a table of inverses
@ of GF(2⁸) field elements, from which both the S-box and inverse S-box
@ functions can be derived; otherwise it can be a simple inverse S-box
@ table.
@ In either case the LUT is represented as two shares, lut_a and lut_b,
@ whose values must be EORed. Furthermore, the contents of each share are
@ scambled according to a 4-byte "map". The map comprises two bytes that
@ are EORed into the addressing of the share, and two bytes that are
@ EORed into the data read back from the share. Performing a lookup
@ of a value x involves computing
@ lut_a[x ^ a₀ ^ a₁] ^ c₀ ^ c₁ ^ lut_b[x ^ b₀ ^ b₁] ^ d₀ ^ d₁
@ where a₀, a₁, c₀ and c₁ are the "map" of the lut_a share and
@ b₀, b₁, d₀ and d₁ are the "map" of the lut_b share.
@ In practice the result of a lookup is itself represented in two
@ shares, namely
@ lut_a[x ^ a₀ ^ a₁] ^ c₀ ^ d₀  and
@ lut_b[x ^ b₀ ^ b₁] ^ c₁ ^ d₁
lut_a:                       @ LUT share A
.space 256
lut_a_map:                   @ the current scrambling of lut_a; not particularly secret since it can be deduced from the contents of lut_a and lut_b
.space 4
.space 4                     @ align to multiple of 8
lut_b:                       @ LUT share B
.space 256
lut_b_map:
.space 4
.space 4                     @ align to multiple of 8
rkey_s:                      @ round key shares
.if RK_ROR
.space 600
.else
.space 480
.endif
.if CT_BPERM
ctr_scratch:                 @ scratch area for CTR code to use when "decrypting" out-of-range blocks
.space 16
.endif
rstate:                      @ SHA random state, to be initialised to TRNG bytes; zeroth byte must be initialised to zero
.space 16

.section .text.aes,"ax",%progbits

.thumb_func
aes_start:
 nop

.if GEN_RAND_SHA
.balign 4
.thumb_func
@ random numbers using SHA256 hardware
@ preserves r1-r3
gen_rand:
 GET_CANARY_NJ r0,CTAG1
 push {r0-r3,r14}
 ldr r0,=#SHA256_BASE
4:
 ldr r2,=#rstate
 ldrb r1,[r2]                @ get word counter from bottom byte of rstate[] (offset into SUM registers)
 subs r3,r1,#4               @ decrement it to previous SUM register
 ble 1f                      @ if the offset was 4 or less we have run out of SUM register values
.if SHA256_SUM0_OFFSET!=8
.err
.endif
2:
 ldr r0,[r0,r1]              @ read value from SUM register: note that this relies on SHA256_SUM0_OFFSET==8
 strb r3,[r2]                @ save updated SUM register offset in bottom byte of rstate[]
 pop {r1}
 CHK_CANARY_NJ r1,CTAG1
 pop {r1-r3,r15}

1:
 movs r3,#SHA256_SUM6_OFFSET+1
 strb r3,[r2]                @ reset word counter: the +1 is compensated for later
 movw r1,#(1<<SHA256_CSR_BSWAP_LSB)+(1<<SHA256_CSR_START_LSB)
 str r1,[r0,#SHA256_CSR_OFFSET]        @ start SHA256 hardware
 movs r3,#3                  @ take four words from rstate, incrementing as we go
 ldr r1,[r2]
 adds r1,r1,#255             @ overall this adds 256 to the value in rstate and resets the bottom byte to SHA256_SUM6_OFFSET
1:
 str r1,[r2],#4
 str r1,[r0,#SHA256_WDATA_OFFSET]
 cbz r3,3f
 ldr r1,[r2]
 adcs r1,r1,#0
 sub r3,r3,#1                @ preserve the carry
 b 1b
3:
 ldr r1,=#1223352428         @ 12 more words with a fixed value
 movs r3,#12
1:
 str r1,[r0,#SHA256_WDATA_OFFSET]
 subs r3,r3,#1
 bne 1b
1:
 ldr r3,[r0,#SHA256_CSR_OFFSET]
 lsrs r3,r3,#SHA256_CSR_SUM_VLD_LSB+1
 bcc 1b                      @ wait for hardware to finish
 ldr r0,[r0,#SHA256_SUM7_OFFSET]
 pop {r1}
 CHK_CANARY_NJ r1,CTAG1
 pop {r1-r3,r15}

.else

@ preserves r1-r3
.balign 4
.thumb_func
gen_rand:
 GET_CANARY_NJ r0,CTAG1
 push {r0,r1,r14}
 ldr r14,=rstate
 ldr r0,[r14]
 ldr r1,=0x1d872b41         @ constant for a maximum-length sequence
 and r1,r1,r0,asr#31         @ will we be shifting out a 1? keep the constant, otherwise 0
 eor r0,r1,r0,lsl#1
 str r0,[r14]
 pop {r1}
 CHK_CANARY_NJ r1,CTAG1
 pop {r1,r15}

.endif
.ltorg

.balign 4
.thumb_func
gen_lut_inverse:
@ set lut_a to be a table of GF(2⁸) inverses, using lut_b as temporary storage
@ return r0=lut_a, r1=lut_b
 ldr r0,=lut_a
 ldr r1,=lut_b
@ first set lut_a to be a table of antilogarithms, lut_b a table of logarithms
 mov r2,#0
 strb r2,[r0]                @ (*)
 mov r3,#1                   @ we maintain invariant that r2=log(r3)
1:
 strb r2,[r0,r3]             @ log table
 strb r3,[r1,r2]             @ antilog table
 lsls r12,r3,#25
 it cs
 eorcs r12,r12,#0x1b000000   @ multiply by x
 eor r3,r3,r12,lsr#24        @ multiply by x+1 ("3"), which is a primitive element
 add r2,r2,#1
 cmp r2,#255
 bls 1b
 movs r2,#255
1:
 ldrb r3,[r0,r2]             @ for each i≠0, find log,...
 eor r3,r3,#255              @ ... negate...
 ldrb r3,[r1,r3]             @ ... and antilog to get inverse
 strb r3,[r0,r2]
 subs r2,r2,#1
 bne 1b                      @ note that inverse(0)=0 by (*) above
 bx r14

.balign 4
.thumb_func
remap:
@ do a random remap of the LUTs
@ preserves r0-r11
 push {r14}
 GET_CANARY r14,CTAG2
 push {r0-r11,r14}
 bl gen_rand
 ldr r1,=lut_a
 bl remap_1
 bl gen_rand
 ldr r1,=lut_b
 bl remap_1
 pop {r0-r11,r14}
 CHK_CANARY r14,CTAG2
 pop {r15}

remap_1:
@ r0: B0:xa B1:xb B2:ya B3:yb
@ r1: array of 256 bytes, followed by a 4-byte map
@ shuffle LUT share array such that new[i]=old[i^xa^xb]^ya^yb, update map according to r0
 GET_CANARY r6,CTAG3
 push {r6,r14}
 mov r14,0x01010101
 ubfx r6,r0,#16,#8
 ubfx r7,r0,#24,#8
 mul r6,r6,r14               @ data remaps ya and yb, byte replicated
 mul r7,r7,r14
 movw r10,#0x1010
 and r10,r10,r0,lsl#3        @ 0/16 in each byte of r10 from b1 and b9 of r0, ready for rotates by 0 or 16
 mov r3,#0x7f7f7f7f
 ubfx r2,r0,#0,#1
 lsl r11,r3,r2               @ 0x7f or 0xfe in each byte of r11, ready for sel of rev16
 ubfx r2,r0,#8,#1
 lsl r12,r3,r2
 ldr r2,[r1,#0x100]          @ old map
 eors r2,r2,r0
 str r2,[r1,#0x100]          @ updated map
 mov r2,#252                 @ loop over entries
1:
 ldr r4,[r1,r2]
 eor r3,r2,r0
 eor r3,r3,r0,ror#8
 and r3,r3,#0xfc             @ r3=remapped address r2
 ldr r5,[r1,r3]
 eors r5,r5,r6               @ remap data; ensure case x==0 works by doing both remaps on same side
 eors r5,r5,r7
 lsr r8,r10,#8
 ror r5,r5,r8                @ ROR#16 is the same as eor of address with 2
 ror r5,r5,r10
 rev16 r8,r5                 @ REV16 is the same as eor of address with 1
 uadd8 r9,r11,r11
 sel r5,r8,r5
 rev16 r8,r5
 uadd8 r9,r12,r12
 sel r5,r8,r5
 mul r8,r14,r2
 mul r9,r14,r3
 usub8 r8,r8,r9              @ bytewise comparison of original address and remapped address, both byte replicated
 sel r8,r4,r5                @ swap r4 and r5 as necessary in constant time
 str r8,[r1,r2]              @ write possibly swapped values back
 sel r8,r5,r4
 str r8,[r1,r3]
 subs r2,r2,#4
 bpl 1b
 pop {r6,r14}
 CHK_CANARY r6,CTAG3
 bx r14

.if NEED_HPERM
.balign 4
.thumb_func
hperm:
@ rotate state within registers
@ r0: B0: rotate amount for r4,r8; B1: rotate amount for r5,r9; B2: rotate amount for r6,r10; B3: rotate amount for r7,r11
@ return r0 value required to undo
 movs r1,#0x18               @ constant for subsequent ANDs
 and r2,r1,r0,lsl#3          @ extract amount
 rors r4,r4,r2               @ rotate share A
 rors r8,r8,r2               @ rotate share B
 and r2,r1,r0,lsr#5          @ etc.
 rors r5,r5,r2
 rors r9,r9,r2
 and r2,r1,r0,lsr#13
 rors r6,r6,r2
 rors r10,r10,r2
 and r2,r1,r0,lsr#21
 rors r7,r7,r2
 rors r11,r11,r2
@ movs r1,#0                 @ not needed as 0x18 has zeros in all the required places to do a two-bit-wise negate
 usub8 r0,r1,r0
 bx r14
.endif

.if NEED_VPERM
.balign 4
.thumb_func
vperm:
@ rotate state registers r4->r5-r6->r7->r4 etc. in constant time
@ r0: b0..1: rotate amount
@ returns r0 value required to undo
@ preserves r2
 and r1,r0,#2
 rsbs r1,r1,#0               @ 0 or fffffffe depending on b1 of r0
 uadd8 r1,r1,r1              @ set/clear all GE flags according to b1 of r0: set if rotate of two places is required
 mov r1,r4
 sel r4,r6,r4
 sel r6,r1,r6
 mov r1,r5
 sel r5,r7,r5
 sel r7,r1,r7
 mov r1,r8
 sel r8,r10,r8
 sel r10,r1,r10
 mov r1,r9
 sel r9,r11,r9
 sel r11,r1,r11
 and r1,r0,#1
 rsbs r1,r1,#0               @ 0 or ffffffff depending on b0 of r0
 uadd8 r1,r1,r1              @ set/clear all GE flags according to b0 of r0: set if rotate of one place is required
 mov r1,r4
 sel r4,r5,r4
 sel r5,r6,r5
 sel r6,r7,r6
 sel r7,r1,r7
 mov r1,r8
 sel r8, r9  ,r8
 sel r9, r10 ,r9
 sel r10,r11,r10
 sel r11,r1 ,r11
 rsbs r0,r0,#0               @ generate control value for inverse operation
 bx r14
.endif

.if IK_SHUFREAD
@ randomly shuffle an array n bytes long, n≤65536 a power of 2, by performing k random exchanges, k>0
@ r0: array pointer p
@ r1: n
@ r2: k
@ does not need to be a subroutine!!!
array_shuf:
 push {r4-r6,r14}
 mov r4,r0
 subs r5,r1,#1     @ mask for random number generation
 mov r6,r2
1:
 bl gen_rand
 and r1,r5,r0,lsr#16
 and r0,r5,r0      @ r0,r1 are two random numbers 0..n-1
 ldrb r2,[r4,r0]
 ldrb r3,[r4,r1]
 strb r3,[r4,r0]
 strb r2,[r4,r1]
 subs r6,r6,#1
 bne 1b
 pop {r4-r6,r15}
.endif

@ "refresh" shares of rkeys by random eor into both shares of each word
.if RK_ROR
@ and randomly change rotate amount on each word of each share
.endif
@ preserves r0-r11
.balign 4
ref_round_keys_s:
 push {r14}
 GET_CANARY r14,CTAG4
 push {r0-r11,r14}
 ldr r0,=rkey_s
 mov r1,#15                  @ there are 15 expanded keys
1:
.if RK_ROR
 ldmia r0,{r2-r11}
 push {r0-r1}

 bl gen_rand                 @ xra=random extra rotates for share A
 usub8 r6,r6,r0              @ ra-=xra bytewise
 rors r2,r2,r0               @ a=ror(a,xra)
 rev16 r0,r0                 @ byte order 2301, i.e. B1 at the bottom
 rors r3,r3,r0               @ a=ror(a,xra)
 rev r0,r0                   @ byte order 1032, i.e. B2 at the bottom
 rors r4,r4,r0               @ a=ror(a,xra)
 rev16 r0,r0                 @ byte order 0123, i.e. B3 at the bottom
 rors r5,r5,r0               @ a=ror(a,xra)

 bl gen_rand                 @ xrb=random extra rotates for share B
 usub8 r11,r11,r0            @ rb-=xrb bytewise
 rors r7,r7,r0               @ b=ror(b,xrb)
 rev16 r0,r0
 rors r8,r8,r0               @ b=ror(b,xrb)
 rev r0,r0
 rors r9,r9,r0               @ b=ror(b,xrb)
 rev16 r0,r0
 rors r10,r10,r0             @ b=ror(b,xrb)
 usub8 r1,r6,r11             @ ra-rb bytewise

 bl gen_rand                 @ xab=extra exclusive OR into shares
 eors r2,r2,r0               @ a^=xab
 rors r0,r0,r1               @ ror(xab,ra-rb)
 eors r7,r7,r0               @ b^=ror(xab,ra-rb)
 rev16 r1,r1

 bl gen_rand                 @ xab
 eors r3,r3,r0               @ a^=xab
 rors r0,r0,r1               @ ror(xab,ra-rb)
 eors r8,r8,r0               @ b^=ror(xab,ra-rb)
 rev r1,r1

 bl gen_rand                 @ xab
 eors r4,r4,r0               @ a^=xab
 rors r0,r0,r1               @ ror(xab,ra-rb)
 eors r9,r9,r0               @ b^=ror(xab,ra-rb)
 rev16 r1,r1

 bl gen_rand                 @ xab
 eors r5,r5,r0               @ a^=xab
 rors r0,r0,r1               @ ror(xab,ra-rb)
 eors r10,r10,r0             @ b^=ror(xab,ra-rb)

 pop {r0-r1}
 stmia r0!,{r2-r11}
.else
 ldmia r0,{r4-r11}           @ EOR random data into the shares
 push {r0-r1}
 bl gen_rand
 eor r4,r4,r0
 eor r8,r8,r0
 bl gen_rand
 eor r5,r5,r0
 eor r9,r9,r0
 bl gen_rand
 eor r6,r6,r0
 eor r10,r10,r0
 bl gen_rand
 eor r7,r7,r0
 eor r11,r11,r0
 pop {r0-r1}
 stmia r0!,{r4-r11}
.endif
 subs r1,r1,#1
 bne 1b
 pop {r0-r11,r14}
 CHK_CANARY r14,CTAG4
 pop {r15}

@ switch from non-shared to shared state
.balign 4
ns_to_s:
 push {r14}
 GET_CANARY r14,CTAG5
 push {r0-r3,r14}
 bl gen_rand
 mov r8,r0
 bl gen_rand
 mov r9,r0
 bl gen_rand
 mov r10,r0
 bl gen_rand
 mov r11,r0
 eors r4,r4,r8
 eors r5,r5,r9
 eors r6,r6,r10
 eors r7,r7,r11
 pop {r0-r3,r14}
 CHK_CANARY r14,CTAG5
 pop {r15}

.if NEED_ROUNDS
.balign 4
.thumb_func
shift_rows_s:
@ first "rotate" the two most-significant bytes of the state by two registers
@ slightly faster (but not shorter?) with ubfx/bfi
 eors r0,r4,r6               @ ta=state[0]^state[2]; ta&=0xffff0000; state[0]^=ta; state[2]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r4,r4,r0
 eors r6,r6,r0
 eors r0,r5,r7               @ ta=state[1]^state[3]; ta&=0xffff0000; state[1]^=ta; state[3]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r5,r5,r0
 eors r7,r7,r0
@ next "rotate" the two odd-significance bytes of the state by one register
 eors r1,r7,r4               @ tb=state[3]^state[0]; tb&=0xff00ff00;
 ands r1,r1,#0xff00ff00
 eors r0,r4,r5               @ ta=state[0]^state[1]; ta&=0xff00ff00; state[0]^=ta;
 ands r0,r0,#0xff00ff00
 eors r4,r4,r0
 eors r0,r5,r6               @ ta=state[1]^state[2]; ta&=0xff00ff00; state[1]^=ta;
 ands r0,r0,#0xff00ff00
 eors r5,r5,r0
 eors r0,r6,r7               @ ta=state[2]^state[3]; ta&=0xff00ff00; state[2]^=ta;
 ands r0,r0,#0xff00ff00
 eors r6,r6,r0
 eors r7,r7,r1               @                                       state[3]^=tb;
@ repeat for other share
 eors r0,r8,r10              @ ta=state[0]^state[2]; ta&=0xffff0000; state[0]^=ta; state[2]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r8,r8,r0
 eors r10,r10,r0
 eors r0,r9,r11              @ ta=state[1]^state[3]; ta&=0xffff0000; state[1]^=ta; state[3]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r9,r9,r0
 eors r11,r11,r0

 eors r1,r11,r8              @ tb=state[3]^state[0]; tb&=0xff00ff00;
 ands r1,r1,#0xff00ff00
 eors r0,r8,r9               @ ta=state[0]^state[1]; ta&=0xff00ff00; state[0]^=ta;
 ands r0,r0,#0xff00ff00
 eors r8,r8,r0
 eors r0,r9,r10              @ ta=state[1]^state[2]; ta&=0xff00ff00; state[1]^=ta;
 ands r0,r0,#0xff00ff00
 eors r9,r9,r0
 eors r0,r10,r11             @ ta=state[2]^state[3]; ta&=0xff00ff00; state[2]^=ta;
 ands r0,r0,#0xff00ff00
 eors r10,r10,r0
 eors r11,r11,r1             @                                       state[3]^=tb;
 bx r14
.endif

.if NEED_INV_ROUNDS
.balign 4
.thumb_func
inv_shift_rows_s:
@ first half is the same as shift_rows; halves could be done in opposite order for tail chain
 eors r0,r4,r6               @ ta=state[0]^state[2]; ta&=0xffff0000; state[0]^=ta; state[2]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r4,r4,r0
 eors r6,r6,r0
 eors r0,r5,r7               @ ta=state[1]^state[3]; ta&=0xffff0000; state[1]^=ta; state[3]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r5,r5,r0
 eors r7,r7,r0

 eors r1,r7,r4               @ tb=state[3]^state[0]; tb&=0xff00ff00;
 ands r1,r1,#0xff00ff00
 eors r0,r6,r7               @ ta=state[2]^state[3]; ta&=0xff00ff00; state[3]^=ta;
 ands r0,r0,#0xff00ff00
 eors r7,r7,r0
 eors r0,r5,r6               @ ta=state[1]^state[2]; ta&=0xff00ff00; state[2]^=ta;
 ands r0,r0,#0xff00ff00
 eors r6,r6,r0
 eors r0,r4,r5               @ ta=state[0]^state[1]; ta&=0xff00ff00; state[1]^=ta;
 ands r0,r0,#0xff00ff00
 eors r5,r5,r0
 eors r4,r4,r1               @                                       state[0]^=tb;

 eors r0,r8,r10              @ ta=state[0]^state[2]; ta&=0xffff0000; state[0]^=ta; state[2]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r8,r8,r0
 eors r10,r10,r0
 eors r0,r9,r11              @ ta=state[1]^state[3]; ta&=0xffff0000; state[1]^=ta; state[3]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r9,r9,r0
 eors r11,r11,r0

 eors r1,r11,r8              @ tb=state[3]^state[0]; tb&=0xff00ff00;
 ands r1,r1,#0xff00ff00
 eors r0,r10,r11             @ ta=state[2]^state[3]; ta&=0xff00ff00; state[3]^=ta;
 ands r0,r0,#0xff00ff00
 eors r11,r11,r0
 eors r0,r9,r10              @ ta=state[1]^state[2]; ta&=0xff00ff00; state[2]^=ta;
 ands r0,r0,#0xff00ff00
 eors r10,r10,r0
 eors r0,r8,r9               @ ta=state[0]^state[1]; ta&=0xff00ff00; state[1]^=ta;
 ands r0,r0,#0xff00ff00
 eors r9,r9,r0
 eors r8,r8,r1               @                                       state[0]^=tb;
 bx r14
.endif

@ multiply polynomial over GF(2⁸) by c(x) = 0x03x³ + 0x01x² + 0x01x + 0x02 modulo x⁴+1
@ r0x00 is a register holding 0x00000000;  r0x1b is a register holding 0x1b1b1b1b
.macro mixcol rx,rt,ru,r0x00,r0x1b
                             @ let rx=(a,b,c,d)
 uadd8 \rt,\rx,\rx           @ MSB of each byte into the GE flags
 sel \ru,\r0x1b,\r0x00       @ get bytewise correction for bytewise field multiplication by 2
 eors \rt,\rt,\ru            @ (2a,2b,2c,2d)

 eors \ru,\rt,\rx            @ (3a,3b,3c,3d)
 eors \rt,\rt,\rx,ror#24     @ (2a+b,2b+c,2c+d,2d+a)
 eors \rt,\rt,\rx,ror#16     @ (2a+b+c,2b+c+d,2c+d+a,2d+a+b)
 eors \rx,\rt,\ru,ror#8      @ (2a+b+c+3d,2b+c+d+3a,2c+d+a+3b,2d+a+b+3c)
.endm

@ multiply polynomial over GF(2⁸) by d(x) = 0x0Bx³ + 0x0Dx² + 0x09x + 0x0E modulo x⁴+1; c(x)d(x)=1 modulo x⁴+1
.macro invmixcol rx,rt,ru,rv,rw,r0x00,r0x1b
@ !!! can probably save some registers, e.g. allow trashing of r0x00, r0x1b
@ can possibly also simplify slightly with refactorisation
 uadd8 \rt,\rx,\rx           @ field multiplication by 2 as above
 sel \rw,\r0x1b,\r0x00
 eors \rt,\rt,\rw            @ 2x
 uadd8 \ru,\rt,\rt
 sel \rw,\r0x1b,\r0x00
 eors \ru,\ru,\rw            @ 4x
 uadd8 \rv,\ru,\ru
 sel \rw,\r0x1b,\r0x00
 eors \rv,\rv,\rw            @ 8x

 eors \rx,\rx,\rv            @ 9x
 eors \rw,\rx,\rt            @ 11x
 eors \rw,\rw,\rx,ror#16     @ 11x ^ 9x ROL #16
 eors \rx,\rx,\ru            @ 13x
 eors \rw,\rw,\rx,ror#8      @ 11x ^ 9x ROL #16 ^ 13x ROL #24
 eors \rt,\rt,\ru            @ 6x
 eors \rt,\rt,\rv            @ 14x
 eors \rx,\rt,\rw,ror#8      @ 14x ^ 9x ROL #8 ^ 13x ROL #16 ^ 11x ROL #24
.endm

.if NEED_ROUNDS
.balign 4
.thumb_func
mix_cols_s:
 mov r2,#0x00000000
 mov r3,#0x1b1b1b1b
 mixcol r4 ,r0,r1,r2,r3      @ apply mixcol to each state word
 mixcol r5 ,r0,r1,r2,r3
 mixcol r6 ,r0,r1,r2,r3
 mixcol r7 ,r0,r1,r2,r3
 mixcol r8 ,r0,r1,r2,r3
 mixcol r9 ,r0,r1,r2,r3
 mixcol r10,r0,r1,r2,r3
 mixcol r11,r0,r1,r2,r3
 bx r14
.endif

.if NEED_INV_ROUNDS
.balign 4
.thumb_func
inv_mix_cols_s:
 push {r14}
 GET_CANARY r14,CTAG6
 push {r14}
 mov r12,#0x00000000
 mov r14,#0x1b1b1b1b
 invmixcol r4 ,r0,r1,r2,r3,r12,r14     @ apply invmixcol to each state word
 invmixcol r5 ,r0,r1,r2,r3,r12,r14
 invmixcol r6 ,r0,r1,r2,r3,r12,r14
 invmixcol r7 ,r0,r1,r2,r3,r12,r14
 invmixcol r8 ,r0,r1,r2,r3,r12,r14
 invmixcol r9 ,r0,r1,r2,r3,r12,r14
 invmixcol r10,r0,r1,r2,r3,r12,r14
 invmixcol r11,r0,r1,r2,r3,r12,r14
 pop {r14}
 CHK_CANARY r14,CTAG6
 pop {r15}
.endif

.if SBOX_VIA_INV
@ bytewise EOR-convolution with constant 0x1f
.macro conv_0x1f rx,rt,ru
 eors \rt,\rx,\rx,ror#31     @ t=x^ROL(x,1);
 eors \rt,\rt,\rt,ror#30     @ t=t^ROL(t,2);
 eors \rt,\rt,\rx,ror#28     @ t=t^ROL(x,4);     @ convolution with byte boundaries "trashed"
 ands \ru,\rx,#0xf0f0f0f0    @ u=x&0xf0f0f0f0;
 eors \ru,\ru,\ru,ror#31     @ u=u^ROL(u,1);
 eors \ru,\ru,\ru,ror#30     @ u=u^ROL(u,2);
 ands \ru,\ru,#0x87878787    @ u=u&0x87878787;   @ compensation for trashing
 eors \ru,\ru,\ru,ror#24     @ u=u^ROL(u,8);
 eors \rx,\rt,\ru,ror#7      @ t^=ROR(u,7);      @ with trashing fixed
.endm

@ bytewise EOR-convolution with constant 0x4a
.macro conv_0x4a rx,rt,ru
 eors \rt,\rx,\rx,ror#30     @ t=x^ROL(x,2);
 eors \rt,\rt,\rx,ror#27     @ t=t^ROL(x,5);
 ands \ru,\rx,#0xf8f8f8f8    @ u=x&0xf8f8f8f8;
 eors \ru,\ru,\ru,ror#29     @ u=u^ROL(u,3);
 ands \ru,\ru,#0xc7c7c7c7    @ u=u&0xc7c7c7c7;
 eors \ru,\ru,\ru,ror#24     @ u=u^ROL(u,8);
 eors \rt,\rt,\ru,ror#6      @ t^=ROR(u,6);
 ands \ru,\rt,#0x80808080    @ t=rorbytes(t,7);
 uadd8 \rt,\rt,\rt
 orrs \rx,\rt,\ru,lsr#7
.endm

.balign 4
.thumb_func
map_sbox_s:
 push {r14}
 GET_CANARY r14,CTAG7
 push {r14}
 bl lutmap_state_s           @ the S-box function is an inverse followed by an affine transformation:
 conv_0x1f r4 ,r0,r1         @ see https://en.wikipedia.org/wiki/Rijndael_S-box
 conv_0x1f r5 ,r0,r1
 conv_0x1f r6 ,r0,r1
 conv_0x1f r7 ,r0,r1
 conv_0x1f r8 ,r0,r1
 conv_0x1f r9 ,r0,r1
 conv_0x1f r10,r0,r1
 conv_0x1f r11,r0,r1
 eor r4 ,r4 ,#0xcacacaca     @ scramble the shares slightly: 0x63=0xca^0xa9 etc.
 eor r5 ,r5 ,#0xf5f5f5f5
 eor r6 ,r6 ,#0x0c0c0c0c
 eor r7 ,r7 ,#0xa2a2a2a2
 eor r8 ,r8 ,#0xa9a9a9a9
 eor r9 ,r9 ,#0x96969696
 eor r10,r10,#0x6f6f6f6f
 eor r11,r11,#0xc1c1c1c1
 pop {r14}
 CHK_CANARY r14,CTAG7
 pop {r15}

.if NEED_INV_ROUNDS
.balign 4
.thumb_func
inv_map_sbox_s:
 push {r14}
 GET_CANARY r14,CTAG8
 push {r14}                  @ similarly, the inverse S-box is an affine transformation followed by an inverse
 conv_0x4a r4 ,r0,r1
 conv_0x4a r5 ,r0,r1
 conv_0x4a r6 ,r0,r1
 conv_0x4a r7 ,r0,r1
 conv_0x4a r8 ,r0,r1
 conv_0x4a r9 ,r0,r1
 conv_0x4a r10,r0,r1
 conv_0x4a r11,r0,r1
 eor r4 ,r4 ,#0xd1d1d1d1     @ scramble the shares slightly: 0x05=0xd1^0xd4 etc.
 eor r5 ,r5 ,#0x94949494
 eor r6 ,r6 ,#0xfcfcfcfc
 eor r7 ,r7 ,#0x3a3a3a3a
 eor r8 ,r8 ,#0xd4d4d4d4
 eor r9 ,r9 ,#0x91919191
 eor r10,r10,#0xf9f9f9f9
 eor r11,r11,#0x3f3f3f3f
 bl lutmap_state_s
 pop {r14}
 CHK_CANARY r14,CTAG8
 pop {r15}
.endif

.else

.balign 4
.thumb_func
gen_lut_sbox:
@ set both lut_a and lut_b to the S-box table
@ returns r0=lut_a+256, r1=lut_b+256
 push {r14}
 GET_CANARY r14,CTAG9
 push {r14}                  @ similarly, the inverse S-box is an affine transformation followed by an inverse
 bl gen_lut_inverse          @ first generate the table of inverses in lut_a
 mov r14,#256
1:
 ldrb r2,[r0]
 eors r3,r2,r2,lsl#1         @ convolve byte with 0x1f
 eors r3,r3,r3,lsl#2
 eors r3,r3,r2,lsl#4
 eors r2,r3,r3,lsr#8
 eor r2,r2,#0x63             @ and add 0x63
 strb r2,[r0],#1
 strb r2,[r1],#1
 subs r14,r14,#1
 bne 1b
 pop {r14}
 CHK_CANARY r14,CTAG9
 pop {r15}

.if NEED_INV_ROUNDS
.balign 4
.thumb_func
gen_lut_inv_sbox:
@ set lut_a to the inverse S-box table
 push {r14}
 GET_CANARY r14,CTAG10
 push {r14}
 bl gen_lut_sbox             @ get the forwards S-box
 sub r0,r0,#256
 sub r1,r1,#256
 mov r2,#0
1:
 ldrb r3,[r1],#1             @ get y=S-box(x)...
 strb r2,[r0,r3]             @ ... and store x at location y
 adds r2,r2,#1
 cmp r2,#255
 bls 1b
 pop {r14}
 CHK_CANARY r14,CTAG10
 pop {r15}
.endif
.endif

@ if we are using direct S-box lookup then [inv_]map_sbox_s is the same as lutmap_state_s
.if !SBOX_VIA_INV
.balign 4
.thumb_func
map_sbox_s:
.if NEED_INV_ROUNDS
.thumb_func
inv_map_sbox_s:
.endif
.endif

@ map all bytes of the state through the LUT
.balign 4
lutmap_state_s:
 push {r14}
 GET_CANARY r14,CTAG11
 push {r14}
 ldr r12,=lut_a
 ldr r14,=lut_b
 mov r0,#0x8000              @ "counter" for bytes of state mapped
1:
 ldr r3,[r12,#0x100]         @ lut_a_map
 eor r1,r4,r3                @ share A of x ^ share A of lut_a address map
 eor r1,r1,r8                @ ^ share B of x
 eor r1,r1,r3,ror#8          @ ^ share B of lut_a address map
 uxtb r1,r1
 ldrb r1,[r12,r1]            @ look up in lut_a
 eor r1,r1,r3,ror#16         @ ^ share A of lut_a data map
 ldr r3,[r14,#0x100]         @ lut_b_map
 eor r1,r1,r3,ror#24         @ ^ share B of lut_b data map, generating share A of the result

 eor r2,r4,r3                @ share A of x ^ share A of lut_b address map
 eor r2,r2,r8                @ ^ share B of x
 eor r2,r2,r3,ror#8          @ ^ share B of lut_b address map
 uxtb r2,r2
 ldrb r2,[r14,r2]            @ look up in lut_b
 eor r2,r2,r3,ror#16         @ ^ share A of lut_b data map
 ldr r3,[r12,#0x100]         @ lut_a_map
 eor r2,r2,r3,ror#24         @ ^ share B of lut_a data map, generating share B of the result

 lsrs r4,#8                  @ shift share A of state down one byte...
 orrs r4,r4,r5,lsl#24
 lsrs r5,#8
 orrs r5,r5,r6,lsl#24
 lsrs r6,#8
 orrs r6,r6,r7,lsl#24
 lsrs r7,#8
 orrs r7,r7,r1,lsl#24        @ and insert share A of mapped byte

 lsrs r8,#8                  @ shift share B of state down one byte...
 orrs r8,r8,r9,lsl#24
 lsrs r9,#8
 orrs r9,r9,r10,lsl#24
 lsrs r10,#8
 orrs r10,r10,r11,lsl#24
 lsrs r11,#8
 orrs r11,r11,r2,lsl#24      @ and insert share B of mapped byte

 lsrs r0,#1                  @ count 16 iterations
 bne 1b
 pop {r14}
 CHK_CANARY r14,CTAG11
 pop {r15}

@ perform one EOR step in round key generation
@ !!! can we introduce some more randomness into the shares here?
.balign 4
grk_s_step:
 ldmia r0!,{r5-r7,r12}       @ from last round key_a but one
 eors r5,r5,r4
 eors r6,r6,r5
 eors r7,r7,r6
 eors r12,r12,r7
 stmia r1!,{r5-r7,r12}
 mov r4,r12
.if RK_ROR
 movs r12,#0
 str r12,[r0],#4
 str r12,[r1],#4
.endif
 ldmia r0!,{r9-r11,r12}      @ from last round key_a but one
 eors r9,r9,r8
 eors r10,r10,r9
 eors r11,r11,r10
 eors r12,r12,r11
 stmia r1!,{r9-r11,r12}
 mov r8,r12
.if RK_ROR
 movs r12,#0
 str r12,[r0],#4
 str r12,[r1],#4
.endif
 bx r14

.macro jitter rx
.if IK_JITTER
 rors \rx,\rx,#1
 bcc \@f
\@:
.else
@ nothing
.endif
.endm

.balign 4
.thumb_func
init_key:
@ r0: rkeys_s
@ r1: raw key data (32 bytes)
.if RK_ROR
@ rkeys_s is a 40*15=600-byte region
@ each of the 15 round keys is represented as two 4-word regions rka[0..3] and rkb[0..3], each of which is followed by a word containing
@ four byte-wide rotate values ra[i] and rb[i]
@ such that rk[i]=(rka[i] ROR ra[i])^(rkb[i] ROR rb[i]) gives the round keys
@ rotations always operate mod 32, so we do not bother to mask the rotate amounts to 5 bits
.else
@ rkeys_s is a 32*15=480-byte region
@ each of the 15 round keys is represented as two 4-word regions rka[0..3] and rkb[0..3]
@ such that rk[i]=rka[i]^rkb[i] gives the round keys
.endif
 GET_CANARY r12,CTAG12
 push {r4-r12,r14}
.if IK_JITTER
 push {r0,r1}
 bl gen_rand
 mov r12,r0
 pop {r0,r1}
.endif
 jitter r12
 mov r4,r0
 mov r5,r1
.if IK_SHUFREAD
 SET_COUNT 73
 add r6,r4,#128              @ use 64 bytes of temporary space at r0+128 for buf
 mov r7,#0
1:
 bl gen_rand
 and r0,r0,#0x1f
 strb r0,[r6,#32]            @ buf contains each number 0..31 and 32 more random numbers in that range
 strb r7,[r6],#1             @ so each number at least once...
 adds r7,r7,#1
 cmp r7,#32
 bne 1b
 CHK_COUNT 73
 add r0,r4,#128
 mov r10,r0
 movs r1,#64
 movs r2,#200
 bl array_shuf               @ ... in a random order
 mov r11,#63
 CHK_COUNT 74
.else
 mov r6,#31
.endif
1:
 SET_COUNT 104
 jitter r12
.if IK_SHUFREAD
 ldrb r6,[r10,r11]           @ now process the raw key bytes in the order given by buf, some more than once
.endif
 lsrs r8,r6,#4
.if RK_ROR
 add r7,r6,r8,lsl#3
 add r7,r7,r8,lsl#4          @ 0..15 -> 0..15, 16..31 -> 40..55
.else
 add r7,r6,r8,lsl#4          @ 0..15 -> 0..15, 16..31 -> 32..47
.endif
 ldrb r9,[r5,r6]             @ fetch key byte
 bl gen_rand                 @ make random shares of round key 0
 CHK_COUNT 104
 eor r9,r9,r0
 strb r9,[r4,r7]
.if RK_ROR
 adds r7,#20
.else
 adds r7,#16
.endif
 strb r0,[r4,r7]
.if IK_SHUFREAD
 subs r11,r11,#1
.else
 subs r6,r6,#1
.endif
 CHK_COUNT 105
 bpl 1b
 CHK_COUNT 106
 mov r0,r4
.if RK_ROR
 movs r1,#0
 str r1,[r0,#16]
 str r1,[r0,#36]
.endif
@ now generate the other round keys
 movs r2,#1                  @ round constant
.if RK_ROR
 add r1,r0,#80
 ldr r4,[r0,#52]             @ last word from previous round key_a
 ldr r8,[r0,#72]             @ last word from previous round key_b
.else
 add r1,r0,#64
 ldr r4,[r0,#44]             @ last word from previous round key_a
 ldr r8,[r0,#60]             @ last word from previous round key_b
.endif
 CHK_COUNT 107
1:
 SET_COUNT 42
 rors r4,r4,#8
 rors r8,r8,#8
 push {r0-r3}
.if IK_JUNK
 bl gen_rand                 @ put some junk in r5-r7, r9-r11
 mov r5,r0
 bl gen_rand
 mov r6,r0
 bl gen_rand
 mov r7,r0
 bl gen_rand
 mov r9,r0
 bl gen_rand
 mov r10,r0
 bl gen_rand
 mov r11,r0
.endif
 CHK_COUNT 42
.if IK_REMAP
 bl remap
.endif
 CHK_COUNT 43
.if IK_PERM
 bl gen_rand
 bl vperm
 push {r0}
 bl gen_rand
 bl hperm
 push {r0}
 bl map_sbox_s               @ this actually maps all of r4..r7, r8..r11 - i.e., trashes r5, r6, r7, r9, r10, r11
 pop {r0}
 bl hperm
 pop {r0}
 bl vperm
.else
 bl map_sbox_s               @ this actually maps all of r4..r7, r8..r11 - i.e., trashes r5, r6, r7, r9, r10, r11
.endif
 CHK_COUNT 44
 pop {r0-r3}
 eors r4,r4,r2               @ round constant
 bl grk_s_step
 CHK_COUNT 45
 lsls r2,#1                  @ step round constant
 cmp r2,#0x40                @ done?
 bhi 2f
 push {r0-r2}
 bl map_sbox_s               @ this actually maps all of r4..r7, r8..r11 - i.e., trashes r5, r6, r7, r9, r10, r11
 CHK_COUNT 46
 pop {r0-r2}
 bl grk_s_step
 CHK_COUNT 47
 b 1b
2:
 CHK_COUNT 46
 pop {r4-r12,r14}
 CHK_CANARY r12,CTAG12
 bx r14

@ add the round key shares pointed to by r12 into the state shares
.balign 4
addrkey_s:
 push {r14}
 GET_CANARY r14,CTAG13
 push {r0-r3,r14}
.if RK_ROR
 ldmia r12!,{r0-r3,r14}      @ share A of round key + ROR data
 rors r0,r0,r14              @ ROR first word
 eors r4,r4,r0               @ add to state
 rev16 r0,r14                @ move byte 1 of ROR data into byte 0
 rors r1,r1,r0
 eors r5,r5,r1
 rev r0,r0                   @ move byte 2 of ROR data into byte 0
 rors r2,r2,r0
 eors r6,r6,r2
 rev16 r0,r0                 @ move byte 3 of ROR data into byte 0
 rors r3,r3,r0
 eors r7,r7,r3
.else
 ldmia r12!,{r0-r3}          @ share A of round key
 eors r4,r4,r0
 eors r5,r5,r1
 eors r6,r6,r2
 eors r7,r7,r3
.endif
.if RK_ROR
 ldmia r12!,{r0-r3,r14}      @ share B of round key + ROR data
 rors r0,r0,r14              @ ROR first word
 eors r8,r8,r0               @ etc., as above
 rev16 r0,r14
 rors r1,r1,r0
 eors r9,r9,r1
 rev r0,r0
 rors r2,r2,r0
 eors r10,r10,r2
 rev16 r0,r0
 rors r3,r3,r0
 eors r11,r11,r3
.else
 ldmia r12!,{r0-r3}          @ share B of round key
 eors r8 ,r8 ,r0
 eors r9 ,r9 ,r1
 eors r10,r10,r2
 eors r11,r11,r3
.endif
 pop {r0-r3,r14}
 CHK_CANARY r14,CTAG13
 pop {r15}

.if NEED_ROUNDS

@ perform encryption rounds
@ r4-r7, r8-r11: state
@ preserves r0-r3,r12
.balign 4
rounds_s:
 push {r14}
 GET_CANARY r14,CTAG14
 push {r0-r3,r12,r14}
 mov r2,#0                   @ round counter
1:
 ldr r12,=rkey_s
 add r12,r12,r2,lsl#5        @ pointer to key shares for this round
.if RK_ROR
 add r12,r12,r2,lsl#3
.endif
 bl addrkey_s
.if ST_VPERM
 bl gen_rand
 bl vperm                    @ V shuffle
.endif
 push {r0,r2}                @ save round count
.if ST_HPERM
 bl gen_rand
 bl hperm                    @ H shuffle
 push {r0}
.endif
 bl map_sbox_s
.if ST_HPERM
 pop {r0}
 bl hperm                    @ undo H shuffle
.endif
 bl shift_rows_s
 ldr r2,[r13,#4]             @ increment round counter on stack
 adds r2,r2,#1
 str r2,[r13,#4]
 cmp r2,#14
 beq 2f                      @ break from loop? (last round has no mix_cols)
 bl mix_cols_s
 pop {r0,r2}
.if ST_VPERM
 bl vperm                    @ undo V shuffle
.endif
 b 1b
2:
@ bl inv_mix_cols_s @ or could skip in last round above
 pop {r0,r2}
.if ST_VPERM
 bl vperm                    @ undo V shuffle
.endif
.if RK_ROR
 ldr r12,=rkey_s+14*40      @ final round key shares
.else
 ldr r12,=rkey_s+14*32      @ final round key shares
.endif
 bl addrkey_s
 pop {r0-r3,r12,r14}
 CHK_CANARY r14,CTAG14
 pop {r15}
.endif

.if NEED_INV_ROUNDS
@ perform decryption rounds
@ r4-r7, r8-r11: state
@ preserves r0-r2
.balign 4
inv_rounds_s:
 push {r14}
 GET_CANARY r14,CTAG15
 push {r0-r2,r14}
.if RK_ROR
 ldr r12,=rkey_s+14*40      @ final round key shares
.else
 ldr r12,=rkey_s+14*32      @ final round key shares
.endif
 bl addrkey_s
 mov r2,#13                  @ round counter
 push {r2}
.if ST_VPERM
 bl gen_rand
 bl vperm                    @ V shuffle
 push {r0}
.endif
 b 2f                        @ into middle of loop (last round has no mix_cols)
1:
 push {r2}
.if ST_VPERM
 bl gen_rand
 bl vperm                    @ V shuffle
 push {r0}
.endif
 bl inv_mix_cols_s
2:
 bl inv_shift_rows_s
.if ST_HPERM
 bl gen_rand
 bl hperm                    @ H shuffle
 push {r0}
.endif
 bl inv_map_sbox_s
.if ST_HPERM
 pop {r0}
 bl hperm                    @ undo H shuffle
.endif
.if ST_VPERM
 pop {r0}
 bl vperm                    @ undo V shuffle
.endif
 pop {r2}
 ldr r12,=rkey_s
 add r12,r12,r2,lsl#5        @ pointer to key shares for this round
.if RK_ROR
 add r12,r12,r2,lsl#3
.endif
 bl addrkey_s
 subs r2,r2,#1
 bpl 1b
 pop {r0-r2,r14}
 CHK_CANARY r14,CTAG15
 pop {r15}
.endif

.if INCLUDE_ENCRYPT_CBC
.balign 4
.thumb_func
@ encrypt data in place
@ r0: ivec
@ r1: buf
@ r2: number of blocks
@ this implementation does not scramble the shares properly; consider a better implementation
@ if security is required in encryption
cbc_encrypt_s:
 push {r14}
 GET_CANARY r14,CTAG16
 push {r4-r11,r14}
 ldmia r0,{r4-r7}            @ load iv into share a
2:
 ldmia r1,{r8-r11}           @ load plaintext into share b
 bl rounds_s
 eor r4,r4,r8                @ convert shared to non-shared
 eor r5,r5,r9
 eor r6,r6,r10
 eor r7,r7,r11
 stmia r1!,{r4-r7}
 subs r2,r2,#1
 bne 2b
 pop {r4-r11,r14}
 CHK_CANARY r14,CTAG16
 pop {r15}
.endif

.if INCLUDE_DECRYPT_CBC
.balign 4
.thumb_func
@ decrypt data in place
@ r0: ivec
@ r1: buf
@ r2: number of blocks
@ return
@ r0=0 OK
@ r0=1: fault detected
@ could be simplified to use more ldmia:s at the cost of another 8 words of stack
cbc_decrypt_s:
 push {r14}
 GET_CANARY r14,CTAG17
 push {r4-r11,r14}
 ldmia r0,{r4-r7}            @ load IV
 bl ns_to_s
 push {r4-r11}               @ IV shares on the stack
2:
 bl remap
 bl ref_round_keys_s         @ refresh the round keys
 ldmia r1,{r4-r7}            @ load the ciphertext
 bl ns_to_s                  @ convert to shares
 bl inv_rounds_s             @ do decryption rounds

.if ROUND_TRIP_TEST

@ compute plaintext {r4-r7}^{r8-r11}^{SP[0..3]}^{SP[4..7]}
@ as shares {r4-r7}^{SP[0..3]}, {r8-r11}^{SP[4..7]}
 ldrd r0,r3,[r13,#0]
 eor r0,r0,r4
 eor r3,r3,r5
 strd r0,r3,[r13,#0]
 ldrd r0,r3,[r13,#8]
 eor r0,r0,r6
 eor r3,r3,r7
 strd r0,r3,[r13,#8]
 ldrd r0,r3,[r13,#16]
 eor r0,r0,r8
 eor r3,r3,r9
 strd r0,r3,[r13,#16]
 ldrd r0,r3,[r13,#24]
 eor r0,r0,r10
 eor r3,r3,r11
 strd r0,r3,[r13,#24]        @ plaintext_s now on the stack
 bl rounds_s                 @ restore original ciphertext (or we could have saved it)

 ldmia r1!,{r0,r3}           @ reload actual ciphertext and compare to check for faults
 eors r0,r0,r4
 eors r0,r0,r8
 bne 1f                      @ mismatch? could repeat this bne or add other protection against its being skipped
 eors r3,r3,r5
 eors r3,r3,r9
 bne 1f
 ldmia r1!,{r0,r3}
 eors r0,r0,r6
 eors r0,r0,r10
 bne 1f
 eors r3,r3,r7
 eors r3,r3,r11
 bne 1f
 subs r1,r1,#16

 pop {r0,r3}                 @ now EOR plaintext shares on stack to recover non-shared plaintext
 ldr r14,[sp,#8]
 eors r0,r0,r14
 ldr r14,[sp,#12]
 eors r3,r3,r14
 stmia r1!,{r0,r3}           @ overwrite ciphertext with plaintext

 pop {r0,r3}
 ldr r14,[sp,#8]
 eors r0,r0,r14
 ldr r14,[sp,#12]
 eors r3,r3,r14
 stmia r1!,{r0,r3}           @ overwrite ciphertext with plaintext

 add r13,#16                 @ first share of plaintext has now been popped; skip the other share

.else

@ compute plaintext {r4-r7}^{r8-r11}^{SP[0..3]}^{SP[4..7]}
@ as shares {r4-r7}^{SP[0..3]}, {r8-r11}^{SP[4..7]}
 pop {r0,r3}
 eor r4,r0,r4
 eor r5,r3,r5
 pop {r0,r3}
 eor r6,r0,r6
 eor r7,r3,r7
 pop {r0,r3}
 eor r8,r0,r8
 eor r9,r3,r9
 pop {r0,r3}
 eor r10,r0,r10
 eor r11,r3,r11              @ now plaintext_s in r4-r11
 eor r8,r8,r4                @ convert to non-shared
 eor r9,r9,r5
 eor r10,r10,r6
 eor r11,r11,r7              @ now plaintext_ns in r8-r11
 ldmia r1,{r4-r7}            @ ciphertext_ns in r4-r7
 stmia r1!,{r8-r11}          @ overwrite ciphertext_ns with plaintext_ns
 bl ns_to_s                  @ convert non-shared ciphertext to shared

.endif

 push {r4-r11}               @ push ciphertext_s, replacing iv or previous ciphertext_s on stack
 subs r2,r2,#1               @ count the blocks
 bne 2b
 add r13,#32
 mov r0,#0                   @ return OK status
 pop {r4-r11,r14}
 CHK_CANARY r14,CTAG17
 pop {r15}

.if ROUND_TRIP_TEST
1:
@ fault here
 rcp_panic
.endif
.endif

.if INCLUDE_CRYPT_CTR
.balign 4
.thumb_func
@ de/encrypt data in place
@ r0: ivec
@ r1: buf
@ r2: n, number of blocks, n>0
.if CT_BPERM
@ In AES-CTR each block can be independently en/decrypted as the encryption only depends on
@ the IV, the key, and the block number. We can therefore process them in any order. Hence
@ we generate all the residues mod u=2^k such that u≥n in a pseudo-random order using a linear conguential
@ generator (x_i+1 = a x_i + c mod u), and process the blocks in that order. We choose
@ x_0 and a randomly (subject to a=5 mod 8), as well as adding an overall random offset
@ to the sequence, which is equivalent to choosing a random c.
@
@ For residues greater than or equal to n we "decrypt" an area of scratch
@ memory, taking the same time as a real decryption.  The inefficiency
@ due to rounding up the number of blocks processed to the next power of
@ two is a factor of 2 in the worst case.
@ q.v. https://en.wikipedia.org/wiki/Linear_congruential_generator#m_a_power_of_2,_c_%E2%89%A0_0
.endif
ctr_crypt_s:
 GET_CANARY r3,CTAG0
 SET_COUNT 171
.if CT_BPERM
 push {r0,r1,r3,r4-r11,r14}
 mvn r4,#0
 subs r5,r2,#1               @ make sure we generate optimal mask for n an exact power of 2
 clz r5,r5
 lsrs r4,r4,r5               @ mask m=2^k-1 s.t. m≥n
 orrs r4,r4,#7               @ m≥7
 bl gen_rand
 bic r5,r0,#7
 adds r5,r5,#5               @ multiplier a, randomly initialised, but make sure it is 5 mod 8
 bl gen_rand
 mov r7,r0                   @ initial block pointer x₀, randomly initialised
 bl gen_rand
 mov r8,r0                   @ sequence offset, randomly initialised: this is equivalent to choosing a random c
 mov r6,r4
.else
 push {r0,r3,r4-r11,r14}
 movs r12,#0
.endif
 CHK_COUNT 171
1:
 SET_COUNT 129
.if CT_BPERM
 add r12,r7,r8               @ add sequence offset
 and r12,r12,r4              @ get block pointer mod 2^k
 cmp r12,r2                  @ set C if beyond end of buffer
 sbcs r3,r3,r3               @ r3==0xffffffff in buffer, 0x00000000 past end
 uadd8 r3,r3,r3              @ set/clear all GE flags if in buffer/past end
 ldr r1,[r13,#4]             @ get buffer address from stack
 add r1,r1,r12,lsl#4         @ calculate address of block
 ldr r3,=ctr_scratch
 sel r1,r1,r3                @ if beyond end of buffer, just process scratch area
 ldr r0,[r13]                @ get IV address from stack
 push {r4-r8,r12}
.else
 ldr r0,[r13]                @ get IV address from stack
 push {r12}
.endif
 CHK_COUNT 129
@ It is not clear if the following addition of the block number in r12 to the IV can usefully
@ be done in terms of shares. Instead we do an addition and subtraction whose overall effect
@ is the same, and which provides a small degree of masking. The IV is not a secret anyway.
 ldmia r0,{r4-r7}            @ load IV
 rev r7,r7                   @ prepare for byte-big-endian, bit-little-endian (!) addition
 rev r6,r6
 rev r5,r5
 rev r4,r4
 bl gen_rand
 bic r8,r0,#0x80000000       @ only 31 bits so we don't get any overflows in the following
 add r9,r8,r12               @ "masked" block number
 adds r7,r7,r9               @ 128-bit addition
 adcs r6,r6,#0
 adcs r5,r5,#0
 adcs r4,r4,#0
 subs r7,r7,r8               @ 128-bit subtraction, unmasking block number
 sbcs r6,r6,r8,asr#31
 sbcs r5,r5,r8,asr#31
 sbcs r4,r4,r8,asr#31
 rev r7,r7
 rev r6,r6
 rev r5,r5
 rev r4,r4
 CHK_COUNT 130
 bl remap                    @ shuffle the LUts
 CHK_COUNT 131
 bl ref_round_keys_s         @ refresh the round keys
 CHK_COUNT 132
 bl ns_to_s                  @ convert IV+x to shares
 CHK_COUNT 133
 bl rounds_s                 @ forward AES rounds on IV+x
 CHK_COUNT 134
 ldr r3,[r1]                 @ decrypt ciphertext
 eors r3,r3,r4
 eors r3,r3,r8
 str r3,[r1]
 ldr r3,[r1,#4]
 eors r3,r3,r5
 eors r3,r3,r9
 str r3,[r1,#4]
 ldr r3,[r1,#8]
 eors r3,r3,r6
 eors r3,r3,r10
 str r3,[r1,#8]
 ldr r3,[r1,#12]
 eors r3,r3,r7
 eors r3,r3,r11
 str r3,[r1,#12]
 CHK_COUNT 135
.if CT_BPERM
 pop {r4-r8,r12}
 muls r7,r7,r5               @ LCG step: x<-ax+1
 adds r7,r7,#1
 subs r6,r6,#1
 CHK_COUNT 136
 bcs 1b
 pop {r0,r1,r3,r4-r11,r14}
.else
 pop {r12}
 adds r1,r1,#16
 add r12,r12,#1
 cmp r12,r2
 CHK_COUNT 136
 bne 1b
 pop {r0,r3,r4-r11,r14}
.endif
 CHK_COUNT 137
 CHK_CANARY r3,CTAG0
 bx r14
.endif

.ltorg

.thumb_func
aes_end:
 nop

@@@@@@@@@@@@@@@@@@@@@@@@@ test functions @@@@@@@@@@@@@@@@@@@@@@@@@

@ .global test_v

@ .section .text.test_v,"ax",%progbits
@ .macro fn
@  ldr.n r0,=0x12345678
@  ldr.n r0,=0xedcba987
@ .endm
@ .macro tenfn
@  fn
@  fn
@  fn
@  fn
@  fn
@  fn
@  fn
@  fn
@  fn
@  fn
@ .endm
@ .macro hundredfn
@  tenfn
@  tenfn
@  tenfn
@  tenfn
@  tenfn
@  tenfn
@  tenfn
@  tenfn
@  tenfn
@  tenfn
@ .endm
@
@ .thumb_func
@ test_v:
@ .balign 4
@ 1:
@  hundredfn
@  b 1b
@  bx r14
@ .ltorg

@ switch from shared to non-shared state
@ s_to_ns:
@  eor r4,r4,r8
@  eor r5,r5,r9
@  eor r6,r6,r10
@  eor r7,r7,r11
@  bx r14

.section .text.debugging,"ax",%progbits

.thumb_func
delay:
.if CHIPW
 subs r0,r0,#3     @ we are clocked approximately three times slower
.else
 subs r0,r0,#1
.endif
 bcs delay
 bx r14

.thumb_func
flush_reg:
@ put known values into r0-r3,r12
 mov r0, #0x80808080
 mov r1, #0x81818181
 mov r2, #0x82828282
 mov r3, #0x83838383
 mov r12,#0x8c8c8c8c
 bx r14

.thumb_func
isr_systick:
 mov.w r2,#0xd0000000 @ set GPIO24
 mov.w r3,#0x01000000
 str r3,[r2,#24]
 ldr r0,=systick_data

 ldr r1,[r0]
 adds r1,r1,#1
 stmia r0!,{r1}
 ldr r1,[r13,#0] @ r0..r2
 ldr r2,[r13,#4]
 ldr r3,[r13,#8]
 stmia r0!,{r1-r3}
 ldr r1,[r13,#12] @ r3
 stmia r0!,{r1,r4-r11}
 ldr r1,[r13,#16] @ r12
 ldr r3,[r13,#28] @ RETPSR
 ubfx r2,r3,#9,#1 @ SPREALIGN
 add r2,r13,r2,lsl#2 @ add 4 to SP if SPREALIGN set in RETPSR
 add r2,r2,#0x68 @ r13
 stmia r0!,{r1-r2}

 ldr r1,[r13,#20] @ r14
 ldr r2,[r13,#24] @ ReturnAddress
@ RETPSR still in r3
 stmia r0!,{r1-r3}

 ldr r0,=0xe000e010
 mov r1,#5
 str r1,[r0] @ write to CSR
 mov.w r2,#0xd0000000
 mov.w r3,#0x01000000
 str r3,[r2,#32] @ clear GPIO24
 bx r14