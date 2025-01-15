.syntax unified
.cpu cortex-m33
.thumb

#include "hardware/platform_defs.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sha256.h"

#include "config.h"

.global delay
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
.global gen_rand_sha
.global gen_irand
.global init_key

.global rkey_s
.global lut_a,lut_a_map
.global lut_b,lut_b_map
.global rstate_sha,rstate_lfsr

.if CT_BPERM
@ Use .data section here because everything is initialised to zero in a .bss section
.section .data.aes
.balign 16
murmur3_constants:           @ Five constants used in murmur3_32 hash
.word 0xcc9e2d51
.word 0x1b873593
.word 0xe6546b64
.word 0x85ebca6b
.word 0xc2b2ae35
.endif

@ Put workspace in the second scratch area (was .section .bss.aes)
.section .scratch_y.aes

@ Regardless of configuration, the code uses a single 256-entry LUT. If both
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
.balign 16
lut_a:                       @ LUT share A (must be 0 mod 16 so that init_key_sbox knows how to mask the lookup)
.space 256
lut_a_map:                   @ the current scrambling of lut_a; not particularly secret since it can be deduced from the contents of lut_a and lut_b
.space 4
.space 4                     @ align to 8 mod 16
lut_b:                       @ LUT share B (must be 8 mod 16 so that init_key_sbox knows how to mask the lookup)
.space 256
lut_b_map:
.space 4
.space 4                     @ align to multiple of 8
rkey_s:                      @ round key shares: 600 bytes = 15 rounds * 2 shares * (4+1) words
                             @ every fourth word has a word that is used as a vperm count, and also as a spacer to misalign the shares mod 16
.space 600
rkey4way:                    @ scratch area for init_key; could overlap this with other scratch space if need to save space
.space 128
.if CT_BPERM
bperm_rand:                  @ 32 half words that define the oblivious permutation of blocks
.space 64
.endif
.balign 16
rstate_sha:                  @ 128-bit SHA random state, to be initialised to TRNG bytes; zeroth byte must be initialised to zero
.space 16
rstate_lfsr:                 @ 32-bit LFSR random state and constant used to step it (initialised by C program)
.space 8
.balign 16
permscratch:                 @ Must be 0 mod 16; 16 bytes of scratch space to store permutation(s)
perm16:
.space 16
@ Scratch space of 32 bytes used both by init_key_sbox and map_sbox_s
.balign 16
fourway:                     @ Must be 0 mod 16
shareA:                      @ 0 mod 16
.space 20                    @ Only need 16 bytes, but choosing shareB!=shareA mod 16
shareB:                      @ 4 mod 16
.space 20
shareC:                      @ 8 mod 16
.space 4
statevperm:                  @ 12 mod 16
.space 4                     @ vperm state rotation: only last two bits are operational; other bits random
.balign 16
chaff:                       @ Must be 0 mod 16;    This will be filled with random numbers to do barrier loads
.space 48
.balign 16

@ Put main code in first scratch area (was .section .text.aes,"ax",%progbits)
.section .scratch_x.aes,"ax",%progbits

.macro gpioput pin,state,reg1,reg2
 mov \reg1,#0xd0000000
 mov \reg2,#(1<<\pin)
 str \reg2,[\reg1,#32-8*\state]
.endm

.macro clear03 offset=0
 ldr r0,=(chaff+\offset)
 ldmia r0,{r0-r3}
.endm

.macro clear01 offset=0
 ldr r0,=(chaff+\offset)
 ldmia r0,{r0,r1}
 rev r0,r0
.endm

.if GEN_RAND_SHA
@ random numbers using SHA256 hardware
.if SHA256_SUM0_OFFSET!=8
.err
.endif

@ Return single random word in r0
@ Preserves r1-r13
.balign 4
gen_rand_sha:
 push {r1-r3,r14}
 bl gen_rand_sha_nonpres
 pop {r1-r3,r15}

@ Return single random word in r0
@ Trashes r1-r3
.balign 4
gen_rand_sha_nonpres:
 ldr r0,=SHA256_BASE
 ldr r2,=rstate_sha
 ldrb r1,[r2]                @ get word counter from bottom byte of rstate_sha[] (offset into SUM registers)
 subs r3,r1,#4               @ decrement it to previous SUM register
 ble 1f                      @ if the offset was 4 or less we have run out of SUM register values
 ldr r0,[r0,r1]              @ read value from SUM register: note that this relies on SHA256_SUM0_OFFSET==8
 strb r3,[r2]                @ save updated SUM register offset in bottom byte of rstate_sha[]
 bx r14
1:
 movs r3,#SHA256_SUM6_OFFSET+1
 strb r3,[r2]                @ reset word counter: the +1 is compensated for later
 movw r1,#(1<<SHA256_CSR_BSWAP_LSB)+(1<<SHA256_CSR_START_LSB)
 str r1,[r0,#SHA256_CSR_OFFSET]        @ start SHA256 hardware
 movs r3,#3                  @ take four words from rstate_sha, incrementing as we go
 ldr r1,[r2]
 adds r1,r1,#255             @ overall this adds 256 to the value in rstate_sha and resets the bottom byte to SHA256_SUM6_OFFSET
1:
 str r1,[r2],#4
 str r1,[r0,#SHA256_WDATA_OFFSET]
 cbz r3,3f
 ldr r1,[r2]
 adcs r1,r1,#0
 sub r3,r3,#1                @ preserve the carry
 b 1b
3:
 movs r1,#0x80               @ End of message bit (with byte-swapped endianity) = start of message padding
 str r1,[r0,#SHA256_WDATA_OFFSET]
 movs r1,#10
1:
 str r3,[r0,#SHA256_WDATA_OFFSET]
 subs r1,r1,#1
 bne 1b
 mov r1,#0x80000000          @ Specifies message length = 128 bits (with byte-swapped endianity)
 str r1,[r0,#SHA256_WDATA_OFFSET]
1:
 ldr r3,[r0,#SHA256_CSR_OFFSET]
 lsrs r3,r3,#SHA256_CSR_SUM_VLD_LSB+1
 bcc 1b                      @ wait for hardware to finish
 ldr r0,[r0,#SHA256_SUM7_OFFSET]
 bx r14
.endif

@ simple LFSR rand versions
@ return a random number in r0
@ This version preserves all r1-r13
@ 23 or 24 cycles including branch = 23 or 24 cycles/word
@ (would be 20 or 21 cycles if written out)
.balign 4
.thumb_func
.if !GEN_RAND_SHA
gen_rand_sha:
.endif
gen_rand_lfsr:
 push {r1,r2,r14}
 bl gen_rand_lfsr_nonpres
 pop {r1,r2,r15}

@ Trashes r1,r2
@ 12 cycles including branch = 12 cycles/word
.balign 4
.if !GEN_RAND_SHA
gen_rand_sha_nonpres:
.endif
gen_rand_lfsr_nonpres:
 ldr r2,=rstate_lfsr
 ldr r0,[r2]
 ldr r1,=0x1d872b41         @ constant for a maximum-length sequence
 and r1,r1,r0,asr#31        @ will we be shifting out a 1? keep the constant, otherwise 0
 eor r0,r1,r0,lsl#1
 str r0,[r2]
 bx r14

@ Return two random words in r0,r1
@ Trashes r2,r3
@ 16 cycles including branch = 8 cycles/word
.balign 4
gen_rand_lfsr2:
 ldr r2,=rstate_lfsr
 ldmia r2,{r1,r3}                           @ r1=state_in, r3=0x1d872b41 = constant for a maximum-length sequence
 and r0,r3,r1,asr#31; eor r0,r0,r1,lsl#1    @ Get new state r0
 and r1,r3,r0,asr#31; eor r1,r1,r0,lsl#1    @ Get new state r1
 str r1,[r2]
 bx r14

@ Return four random words in r0-r3
@ 27 cycles including branch = 6.75 cycles/word
.balign 4
gen_rand_lfsr4:
 push {r14}
 ldr r14,=rstate_lfsr
 ldmia r14,{r3,r14}                         @ r3=state_in, r14=0x1d872b41 = constant for a maximum-length sequence
 and r0,r14,r3,asr#31; eor r0,r0,r3,lsl#1   @ Get new state r0
 and r1,r14,r0,asr#31; eor r1,r1,r0,lsl#1   @ Get new state r1
 and r2,r14,r1,asr#31; eor r2,r2,r1,lsl#1   @ Get new state r2
 and r3,r14,r2,asr#31; eor r3,r3,r2,lsl#1   @ Get new state r3
 ldr r14,=rstate_lfsr
 str r3,[r14]
 pop {r15}

.ltorg

.balign 4
.thumb_func
makesmallperm:
 @ Make a uniformly random permutation of R0 bytes and stores the resulting byte array at R1
 @ Should be very uniform up to R0=10; maybe 11 or 12 are also OK. (10! << 2^32)
 @ To make it valid up to R0=256, move the bl gen_rand_sha inside the loop
 @ Uses inside-out method (slightly more efficient variant of Fisher-Yates)
 @ Trashes r0-r3

 push {r4-r6,r14}
 movs r4,r1
 movs r6,r0
 movs r1,#0
 movs r2,#1
 bl gen_rand_sha
 

1:
 @ r1,r2=i,i+1,   i=0, 2, 4, ...
 cmp r1,r6
 beq 2f
 
 umull r0,r3,r0,r2
 ldrb r5,[r4,r3]
 strb r5,[r4,r1]
 strb r1,[r4,r3]
 adds r1,r1,#2

 @ r2,r1=i,i+1,   i=1, 3, 5, ...
 cmp r2,r6
 beq 2f
 
 umull r0,r3,r0,r1
 ldrb r5,[r4,r3]
 strb r5,[r4,r2]
 strb r2,[r4,r3]
 adds r2,r2,#2
 
 b 1b
 
2:
 pop {r4-r6,r15}

.balign 4
.thumb_func
makeperm16:
 @ Make a random permutation of 16 things using the inside-out method (slightly more efficient variant of Fisher-Yates)
 @ Store it in the 16 bytes at perm16
 @ More efficient than calling makeperm with R0=16, R1=perm16 - fewer calls to gen_rand_sha
 @ Trashes r0-r5

 push {r14}
 ldr r4,=perm16
 bl gen_rand_sha_nonpres
 
 @ i=0
 movs r1,#0
 movs r2,#1       @ r1,r2=i,i+1
 strb r1,[r4]
 
 @ i=1
 adds r1,r1,#2    @ r1,r2=i+1,i
 umull r0,r3,r0,r1
 ldrb r5,[r4,r3]
 strb r5,[r4,r2]
 strb r2,[r4,r3]

1:
 @ i=2, 4, 6, 8
 adds r2,r2,#2    @ r1,r2=i,i+1
 umull r0,r3,r0,r2
 ldrb r5,[r4,r3]
 strb r5,[r4,r1]
 strb r1,[r4,r3]

 @ i=3, 5, 7, 9
 adds r1,r1,#2    @ r1,r2=i+1,i
 umull r0,r3,r0,r1
 ldrb r5,[r4,r3]
 strb r5,[r4,r2]
 cmp r1,#10
 strb r2,[r4,r3]
 bne 1b

 @ refresh random number after extracting 10! from it
 @ 10! and 16!/10! are both much less than 2^32, so the permutation will be extremely close to uniform
 bl gen_rand_sha

1:
 @ i=10, 12, 14
 adds r2,r2,#2    @ r1,r2=i,i+1
 umull r0,r3,r0,r2
 ldrb r5,[r4,r3]
 strb r5,[r4,r1]
 strb r1,[r4,r3]

 @ i=11, 13, 15
 adds r1,r1,#2    @ r1,r2=i+1,i
 umull r0,r3,r0,r1
 ldrb r5,[r4,r3]
 strb r5,[r4,r2]
 cmp r1,#16
 strb r2,[r4,r3]
 bne 1b

 @ Finished making permutation
 pop {r15}
 
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
 push {r0-r11,r14}
 bl gen_rand_sha_nonpres
 ldr r1,=lut_a
 bl remap_1
 bl gen_rand_sha_nonpres
 ldr r1,=lut_b
 bl remap_1
 pop {r0-r11,r15}

remap_1:
@ r0: B0:xa B1:xb B2:ya B3:yb
@ r1: array of 256 bytes, followed by a 4-byte map
@ shuffle LUT share array such that new[i]=old[i^xa^xb]^ya^yb, update map according to r0
 push {r14}
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
 pop {r15}


.if RK_ROR

@ "refresh" shares of rkeys by random eor into both shares of each word
@ Trashes r0-r12
@ If i = word number 0..3,
@ Aptr=memory word pointer to block of 20 bytes containing H&V-rotated share A roundkey (similarly B), then
@ vpermA=Aptr[4]>>30, vpermB=Bptr[4]>>30, and
@ roundkey shareA(i) = Aptr[i+vpermA mod 4] ror ((i+vpermA mod 4)^th byte of Aptr[4])
@ roundkey shareB(i) = Bptr[i+vpermB mod 4] ror ((i+vpermB mod 4)^th byte of Bptr[4])+16
.balign 4
.thumb_func
ref_roundkey_shares_s:
 mov r11,#15                 @ there are 15 expanded keys
ref_roundkey_shares_s_test:  @ entry point for test code to do fewer than 15 rounds
 push {r14}
 ldr r4,=rkey_s
ref_roundkey_shares_s_loop:
 ldmia r4!,{r5-r8,r10}       @ r5-r8 = rkey shareA, r10=X_A=vperm+rotations of rkey shareA

@ ldr r0,=chaff
@ and r1,r11,#7
@ add r0,r0,r1,lsl#2
@ ldmia r0,{r0-r3}

 ldr r12,[r4,#16]            @ r12 = X_B=vperm+rotations of rkey shareB
 mov r0,r12,lsr#30
 sub r9,r0,r10,lsr#30        @ r9 = vperm_B - vperm_A (|junk)
 mov r0,r9,lsl#3             @ r0 = 8*(vperm_B - vperm_A) mod 32
 mov r12,r12,ror r0
 usub8 r12,r10,r12           @ r12 = X_A - (X_B ror r0)
 bl gen_rand_lfsr4
 eors r5,r5,r0; ands r9,r9,#3; ldr r10,[r4,r9,lsl#2]; ror r0,r0,r12; eor r10,r10,r0,ror#16; mov r12,r12,ror#8; str r10,[r4,r9,lsl#2]; adds r9,r9,#1
 eors r6,r6,r1; ands r9,r9,#3; ldr r10,[r4,r9,lsl#2]; ror r1,r1,r12; eor r10,r10,r1,ror#16; mov r12,r12,ror#8; str r10,[r4,r9,lsl#2]; adds r9,r9,#1
 eors r7,r7,r2; ands r9,r9,#3; ldr r10,[r4,r9,lsl#2]; ror r2,r2,r12; eor r10,r10,r2,ror#16; mov r12,r12,ror#8; str r10,[r4,r9,lsl#2]; adds r9,r9,#1
 eors r8,r8,r3; ands r9,r9,#3; ldr r10,[r4,r9,lsl#2]; ror r3,r3,r12; eor r10,r10,r3,ror#16;                    str r10,[r4,r9,lsl#2]
 subs r4,r4,#20
 stmia r4,{r5-r8}
 adds r4,r4,#40
 subs r11,r11,#1
 
@ ldr r0,=chaff
@ add r1,r11,#3
@ and r1,r1,#7
@ add r0,r0,r1,lsl#2
@ ldmia r0,{r0-r3}
 
 bne ref_roundkey_shares_s_loop
 clear03 24
ref_roundkey_shares_s_exit:
 pop {r15}

.balign 4
.thumb_func
@ Rotates roundkey vperms and RK_ROR rotations by random amounts
@ Trashes r0-r10
@ If i = word number 0..3,
@ Aptr=memory word pointer to block of 20 bytes containing H&V-rotated share A roundkey (similarly B), then
@ vpermA=Aptr[4]>>30, vpermB=Bptr[4]>>30, and
@ roundkey shareA(i) = Aptr[i+vpermA mod 4] ror ((i+vpermA mod 4)^th byte of Aptr[4])
@ roundkey shareB(i) = Bptr[i+vpermB mod 4] ror ((i+vpermB mod 4)^th byte of Bptr[4])+16
ref_roundkey_hvperms_s:
 movs r7,#30
ref_roundkey_hvperms_s_test:  @ entry point for test code to do fewer than 30 key shares
 push {r14}
 ldr r10,=rkey_s
ref_roundkey_hvperms_s_loop:
 bl gen_rand_lfsr_nonpres     @ r0=new vperm high|rotations
 ldmia r10,{r2-r5,r9}         @ r2-r5=roundkey share A/B, r9=old vperm high|rotations
 str r0,[r10,#16]
 mov r8,r0,lsr#30             @ r8=new vperm low
 sub r6,r8,r9,lsr#30          @ r6=(new vperm low)-(old vperm low) | junk
 mov r8,r6,lsl#3              @ r8=8*((new vperm low)-(old vperm low)) mod 32
 mov r0,r0,ror r8
 usub8 r0,r9,r0               @ i^th byte of r0 = (i^th byte of old rotations) - ((i+newvperm-oldvperm)^th byte of new rotations)
 movs r2,r2,ror r0; ands r6,r6,#3; str r2,[r10,r6,lsl#2]; movs r0,r0,ror#8; adds r6,r6,#1
 movs r3,r3,ror r0; ands r6,r6,#3; str r3,[r10,r6,lsl#2]; movs r0,r0,ror#8; adds r6,r6,#1
 movs r4,r4,ror r0; ands r6,r6,#3; str r4,[r10,r6,lsl#2]; movs r0,r0,ror#8; adds r6,r6,#1
 movs r5,r5,ror r0; ands r6,r6,#3; str r5,[r10,r6,lsl#2]
 adds r10,r10,#20
 subs r7,r7,#1
 bne ref_roundkey_hvperms_s_loop
 clear03 28
ref_roundkey_hvperms_s_exit:  @ label exit point to be to able to specify to analysis code
 pop {r15}

.else

@ "refresh" shares of rkeys by random eor into both shares of each word
@ Trashes r0-r11
.balign 4
.thumb_func
ref_roundkey_shares_s:
 mov r11,#15                 @ there are 15 expanded keys
ref_roundkey_shares_s_test:  @ entry point for test code to do fewer than 15 rounds
 push {r14}
 ldr r4,=rkey_s
ref_roundkey_shares_s_loop:
 ldmia r4!,{r5-r9}           @ r5-r8 = rkey shareA with vperm r9

@ ldr r0,=chaff
@ and r1,r11,#7
@ add r0,r0,r1,lsl#2
@ ldmia r0,{r0-r3}

 ldr r10,[r4,#16]            @ rkey shareB has a vperm of r10>>30
 mov r10,r10,lsr#30
 sub r9,r10,r9,lsr#30        @ r9 = vperm_B - vperm_A (|junk)
 bl gen_rand_lfsr4
 eors r5,r5,r0; ands r9,r9,#3; ldr r10,[r4,r9,lsl#2]; eor r10,r10,r0,ror#16; str r10,[r4,r9,lsl#2]; adds r9,r9,#1
 eors r6,r6,r1; ands r9,r9,#3; ldr r10,[r4,r9,lsl#2]; eor r10,r10,r1,ror#16; str r10,[r4,r9,lsl#2]; adds r9,r9,#1
 eors r7,r7,r2; ands r9,r9,#3; ldr r10,[r4,r9,lsl#2]; eor r10,r10,r2,ror#16; str r10,[r4,r9,lsl#2]; adds r9,r9,#1
 eors r8,r8,r3; ands r9,r9,#3; ldr r10,[r4,r9,lsl#2]; eor r10,r10,r3,ror#16; str r10,[r4,r9,lsl#2]
 subs r4,r4,#20
 stmia r4,{r5-r8}
 adds r4,r4,#40
 subs r11,r11,#1
 
@ ldr r0,=chaff
@ add r1,r11,#3
@ and r1,r1,#7
@ add r0,r0,r1,lsl#2
@ ldmia r0,{r0-r3}
 
 bne ref_roundkey_shares_s_loop
 clear03 24
ref_roundkey_shares_s_exit:
 pop {r15}

.balign 4
.thumb_func
@ Rotates roundkey vperms by random amounts
@ Trashes r0-r9
ref_roundkey_hvperms_s:
 movs r7,#30
ref_roundkey_hvperms_s_test:  @ entry point for test code to do fewer than 30 key shares
 push {r14}
 bl gen_rand_lfsr_nonpres
 ldr r1,=rkey_s
ref_roundkey_hvperms_s_loop:
 cmp r7,#15
 bne 2f
@ Get a new random r0 after using 15 x 2 bits of the original one
@ Note that the junk bits (2-31) in the vperms are not adjusted independently, but that's no big loss,
@ and the gain is only calling gen_rand_lfsr twice instead of 30 times.
 push {r1}; bl gen_rand_lfsr_nonpres; pop {r1}
 2:
 ldmia r1,{r2-r5,r9}    @ roundkey share A/B=r2-r5, vperm=r9 (including junk bits)
 mov r8,r9,lsr#30       @ r8=old vperm (low)
 add r6,r9,r0           @ r6=new vperm (high) | new junk
 str r6,[r1,#16]
 rsb  r6,r8,r6,lsr#30   @ r6=(new vperm low)-(old vperm low) | junk bits
 ands r6,r6,#3; str r2,[r1,r6,lsl#2]; adds r6,r6,#1
 ands r6,r6,#3; str r3,[r1,r6,lsl#2]; adds r6,r6,#1
 ands r6,r6,#3; str r4,[r1,r6,lsl#2]; adds r6,r6,#1
 ands r6,r6,#3; str r5,[r1,r6,lsl#2]
 adds r1,r1,#20
 movs r0,r0,ror#2
 subs r7,r7,#1
 bne ref_roundkey_hvperms_s_loop
 clear03 28
ref_roundkey_hvperms_s_exit:  @ label exit point to be to able to specify to analysis code
 pop {r15}

.endif

.if NEED_VPERM
.balign 4
.thumb_func
vpermundo:
@ Undo the effects of vperm rotation on share registers r4-r7, r8-r11
@ Expect r1=statevperm (state rotations) on entry
@ Trashes r0-r3,r12
 push {r14}
 ldr r1,=statevperm
 ldr r2,[r1]
 rsbs r0,r2,#0
 b vpermaddr0

.balign 4
.thumb_func
refreshstatevperm:

@ Rotate share registers r4-r7, r8-r11 (r4->r5-r6->r7->r4 etc.) by an addtional random amount and update the rotation at !r1
@ Trashes r0-r3,r12
@ Maintains r4=rorig(4+(-!r1)%4), r5=rorig(4+(1-!r1)%4), ...
@           r8=rorig(8+(-!r1)%4), r9=rorig(8+(1-!r1)%4), ...
@ Note: only low 2 bits of !r1 are used. The rest are random to add to the noise.

 push {r14}
 bl gen_rand_lfsr_nonpres
 ldr r1,=statevperm
 ldr r2,[r1]
vpermaddr0:
 adds r2,r2,r0
 str r2,[r1]
 
 ldr r1,=shareA
 ands r0,r0,#3; str r4,[r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r5,[r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r6,[r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r7,[r1,r0,lsl#2]; adds r0,r0,#1
 ldmia r1,{r4-r7}
 
 ldr r12,=chaff               @ Overwrite temperorary storage with random numbers
 ldmia r12,{r2,r3,r12,r14}
 stmia r1,{r2,r3,r12,r14}

 ldr r1,=shareB
 ands r0,r0,#3; str r8, [r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r9, [r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r10,[r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r11,[r1,r0,lsl#2]; adds r0,r0,#1
 ldmia r1,{r8-r11}
 
 ldr r12,=chaff+16            @ Overwrite temperorary storage with random numbers
 ldmia r12,{r2,r3,r12,r14}
 stmia r1,{r2,r3,r12,r14}

refreshstatevperm_exit:       @ label exit point to be to able to specify to analysis code
 pop {r15}
.endif

@ Switch from non-shared to shared state
@ Trashes r0-r3,r12
.balign 4
ns_to_s:
 push {r14}
.if ST_SHAREC
 bl gen_rand_sha_nonpres                   @ Create state share C; all bytes the same
 ands r0,r0,#255
 orrs r0,r0,r0,lsl#8
 orrs r12,r0,r0,lsl#16
 ldr r1,=shareC
 str r12,[r1]
.else
 movs r12,#0
.endif
 bl gen_rand_sha_nonpres
 eors r4,r4,r0
 eor r8,r12,r0,ror#16
 bl gen_rand_sha_nonpres
 eors r5,r5,r0
 eor r9,r12,r0,ror#16
 bl gen_rand_sha_nonpres
 eors r6,r6,r0
 eor r10,r12,r0,ror#16
 bl gen_rand_sha_nonpres
 eors r7,r7,r0
 eor r11,r12,r0,ror#16
.if ST_VPERM
 bl gen_rand_sha_nonpres
.endif
 ldr r1,=statevperm
 movs r2,#0
 str r2,[r1]
.if ST_VPERM
 b vpermaddr0                              @ Tail call. Initialise state vperm with SHA RNG, refresh with LFSR RNG
.else
 pop {r15}
.endif

@ Conjugate lut_a, lut_b with shareC
@ I.e., EOR the input and output with shareC.
@ We need to pick one input for each share A and B, and one output for ONE of the shares A and B
@ Arbitrarily choosing a0, b1 and d0
.balign 4
conjshareC:
.if ST_SHAREC
 ldr r1,=shareC
 ldr r0,[r1]                   @ Get shareC as a word (all bytes the same)
 ldr r1,=lut_a                 @ Need to EOR share C into inputs of both lut_a and lut_b, and one of their outputs...
 ldr r2,[r1,#0x100]
 eors r2,r2,r0,lsr#24
 str r2,[r1,#0x100]
 movs r0,r0,lsr#16
 ldr r1,=lut_b                 @ ... (continued) Here we're EORing share C into a0, b1 and d0.
 ldr r2,[r1,#0x100]
 eors r2,r2,r0,lsl#8
 str r2,[r1,#0x100]
.endif
 bx r14
 
.if NEED_ROUNDS
.balign 4
.thumb_func
shift_rows_s:
@ First "rotate" the two most-significant bytes of the state by two registers
@ Trashes r0-r3
@ Slightly faster (but not shorter?) with ubfx/bfi
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
@ repeat for other share, conjugated by ror#16
 clear01                     @ barrier
 eors r0,r8,r10              @ ta=state[0]^state[2]; ta&=0x0000ffff; state[0]^=ta; state[2]^=ta;
 lsls r0,r0,#16
 lsrs r0,r0,#16
 eors r8,r8,r0
 eors r10,r10,r0
 eors r0,r9,r11              @ ta=state[1]^state[3]; ta&=0x0000ffff; state[1]^=ta; state[3]^=ta;
 lsls r0,r0,#16
 lsrs r0,r0,#16
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

 clear01                     @ barrier 
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
@ Trashes r0-r3,r12
mix_cols_s:
 mov r2,#0x00000000
 mov r3,#0x1b1b1b1b
 mixcol r4 ,r0,r1,r2,r3      @ apply mixcol to each state word
 mixcol r5 ,r0,r1,r2,r3
 mixcol r6 ,r0,r1,r2,r3
 mixcol r7 ,r0,r1,r2,r3
 ldr r12,=chaff
 ldmia r12!,{r0,r1}          @ overwrite sensitive shareA-related quantities r0,r1 with random numbers
 mixcol r8 ,r0,r1,r2,r3
 mixcol r9 ,r0,r1,r2,r3
 mixcol r10,r0,r1,r2,r3
 mixcol r11,r0,r1,r2,r3
 ldmia r12!,{r0,r1}          @ overwrite  sensitive shareB-related quantities r0,r1 with random numbers
 bx r14
.endif

.if NEED_INV_ROUNDS
.balign 4
.thumb_func
inv_mix_cols_s:
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
map_sbox_s: @ (we're currently still under .if SBOX_VIA_INV) version of map_sbox_x that uses lutmap_state_s as a lookup into a table of inverses
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
 pop {r15}

.if NEED_INV_ROUNDS
.balign 4
.thumb_func
inv_map_sbox_s: @ version that computes via tables of inverses
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
 pop {r15}
.endif

.else

.balign 4
.thumb_func
gen_lut_sbox:
@ gen_lut_sbox sets both lut_a and lut_b to the S-box table and
@ returns r0=lut_a+256, r1=lut_b+256
 push {r14}
 bl gen_lut_inverse          @ first generate the table of inverses in lut_a
                             @ At this point r0=lut_a, r1=lut_b, lut_a[] contains inverses and lut_b[] contains other stuff
 mov r14,#256
1:
 ldrb r2,[r0]
 eors r3,r2,r2,lsl#1         @ convolve byte with 0x1f
 eors r3,r3,r3,lsl#2
 eors r3,r3,r2,lsl#4
 eors r2,r3,r3,lsr#8
 eor r2,r2,#0x63             @ and add 0x63
 strb r2,[r0],#1             @ let lut_a[i]=sbox[i]
 strb r2,[r1],#1             @ let lut_b[i]=sbox[i]
 subs r14,r14,#1
 bne 1b
 pop {r15}

.if NEED_INV_ROUNDS
.balign 4
.thumb_func
gen_lut_inv_sbox:
@ set lut_a to the inverse S-box table
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
 pop {r15}
.endif
.endif

@ Lookup each byte of a word, Rtarg, in a table and replace Rtarg with the result (used for SBOX lookups)
.macro subbytes Rtarg,Rtable,Rspare0,Rspare1,Rspare2,Rspare3
 ubfx \Rspare0,\Rtarg,#0,  #8
 ubfx \Rspare1,\Rtarg,#8,  #8
 ubfx \Rspare2,\Rtarg,#16, #8
 ubfx \Rspare3,\Rtarg,#24, #8

 ldrb \Rspare0,[\Rtable,\Rspare0]
 ldrb \Rspare1,[\Rtable,\Rspare1]
 ldrb \Rspare2,[\Rtable,\Rspare2]
 ldrb \Rspare3,[\Rtable,\Rspare3]
 orr \Rspare0,\Rspare0,\Rspare1,lsl#8
 orr \Rspare2,\Rspare2,\Rspare3,lsl#8
 orr \Rtarg,\Rspare0,\Rspare2,lsl#16
.endm

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

@ lutmap_state_s maps all bytes of the state through the split LUT, lut_a and lut_b
@ This is either the whole of map_sbox_s (if SBOX_VIA_INV=0), or (if SBOX_VIA_INV=1) it's a subroutine called by map_sbox_s
@ Trashes r0-r3,r12
.balign 4
lutmap_state_s:

 push {r14}
 
 ldr r0,=shareA                 @ Write out state share A to memory
 stmia r0,{r4-r7}
 clear03                        @ barrier

 ldr r0,=shareB                 @ Write out state share B to memory
 stmia r0,{r8-r11}
 clear03 4                      @ barrier

 bl makeperm16                  @ Rebuild random 16-way permutation. Maybe do this less frequently
 @ Now combine state shares A and B and apply the split sbox to each byte, in the order given by the above random permutation

 ldr r8,=lut_a
 ldr r9,=lut_b
 ldr r0,[r8,#0x100]             @ R0 = a0 | a1<<8 | c0<<16 | c1<<24   (lut_a_map)
 eors r10,r0,r0,lsr#8
 uxtb r10,r10                   @ R10 = a0^a1
 ldr r1,[r9,#0x100]             @ R1 = b0 | b1<<8 | d0<<16 | d1<<24   (lut_b_map)
 eors r1,r0,r1
 eors r2,r1,r1,lsr#8
 uxtb r11,r2                    @ R11 = a0^a1^b0^b1
 movs r12,r1,lsr#16             @ R12 = c0^d0 | (c1^d1)<<8
 
 ldr r4,=perm16
 ldr r5,=shareA
 ldr r6,=shareB
@ Using r0=loop counter, r4=perm16, r5=shareA, r6=shareB, r8=lut_a, r9=lut_b, r10=a0^a1, r11=a0^a1^b0^b1, r12=(c0^d0) | (c1^d1)<<8
 movs r0,#15
1:                              @ (Ordering instructions to minimise result delays)
 ldrb r1,[r4,r0]                @ r1 = perm[r0]
 eors r7,r1,#2                  @ r7 = perm[r0]^2
 ldrb r2,[r5,r1]                @ r2 = shareA[perm[r0]]
 ldrb r3,[r6,r7]                @ r3 = shareB[perm[r0]^2]
 eors r2,r2,r10                 @ r2 = shareA[perm[r0]]^a0^a1
 eors r2,r2,r3                  @ r2 = shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]
 ldrb r3,[r8,r2]                @ r3 = lut_a[shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]]
 eors r3,r3,r12                 @ r3 = lut_a[shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]]^c0^d0 | (junk<<8)
 eors r2,r2,r11                 @ r2 = shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]
 strb r3,[r5,r1]                @ shareA'[perm[r0]] = lut_a[shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]]^c0^d0
 ldrb r3,[r9,r2]                @ r3 = lut_b[shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]]
 subs r0,r0,#1
 eor  r3,r3,r12,lsr#8           @ r3 = lut_b[shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]]^c1^d1
 strb r3,[r6,r7]                @ shareB'[perm[r0]^2] = lut_b[shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]]^c1^d1
 bpl 1b
 clear03 8                      @ barrier
 
 ldmia r6,{r8-r11}              @ Read state share B back from memory
 clear03 12                     @ barrier
 ldmia r5,{r4-r7}               @ Read state share A back from memory
 clear03 16                     @ barrier

@ Refresh state shares because luts only give imperfect share-by-value
 bl gen_rand_lfsr4
 eors r4,r4,r0; mov r12,#0; eors r8,r8,r0,ror#16              @ Barriers between each pair of eors to prevent implicit r4^r8 etc
 eors r5,r5,r1; mov r12,#0; eors r9,r9,r1,ror#16
 eors r6,r6,r2; mov r12,#0; eors r10,r10,r2,ror#16
 eors r7,r7,r3; mov r12,#0; eors r11,r11,r3,ror#16

 pop {r15}

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
randomisechaff:
@ Randomise 48 bytes of chaff values (random load values)
@ Uses 12 bytes of permscratch
@ Trashes r0-3
 push {r14}
 movs r0,#12
 ldr r1,=permscratch
 bl makesmallperm           @ Store the random words in a random order to make 2nd order attacks harder
 movs r1,#11
1:
 push {r1}
 bl gen_rand_sha_nonpres
 pop {r1}
 ldr r2,=permscratch
 ldrb r2,[r2,r1]
 ldr r3,=chaff
 str r0,[r3,r2,lsl#2]
 subs r1,r1,#1
 bpl 1b
 pop {r15}

.balign 4
refreshchaff:
@ Update 48 bytes of chaff values (random load values) using faster RNG than used for randomisechaff
@ Uses 12 bytes of permscratch
@ Trashes r0-3,12
 push {r14}
 movs r0,#12
 ldr r1,=permscratch
 bl makesmallperm           @ Update the random words in a random order to make 2nd order attacks harder
 movs r1,#11
1:
 push {r1}
 bl gen_rand_lfsr_nonpres
 pop {r1}
 ldr r2,=permscratch
 ldr r3,=chaff
 ldrb r2,[r2,r1]
 ldr r12,[r3,r2,lsl#2]
 add r0,r0,r12
 str r0,[r3,r2,lsl#2]
 subs r1,r1,#1
 bpl 1b
 pop {r15}

.balign 4
.thumb_func
@ Do sbox on the four bytes of the 4-way share r4-r7
@ Trashes r0,r8-r12
init_key_sbox:
 push {r1-r3,r14}
 bl gen_rand_sha_nonpres; mov r8,r0
 bl gen_rand_sha_nonpres; mov r9,r0
 bl gen_rand_sha_nonpres; mov r10,r0
 bl gen_rand_sha_nonpres; mov r11,r0
 ldr r0,=fourway                @ Write out 4-way share to memory
 stmia r0,{r8-r11}              @ Save random values first to obscure saving of state
 stmia r0,{r4-r7}
 movs r4,#0                     @ Clear r4-r7 so that they don't interact with makesmallperm
 movs r5,#0
 movs r6,#0
 movs r7,#0
 
 bl randomisechaff              @ Randomise block of memory mainly used for obscuring loads

 movs r0,#4
 ldr r1,=permscratch
 bl makesmallperm               @ Build random 4-way permutation determining order of bytes to be SBOXed
 ldr r1,=permscratch            @ Write out random addresses in advance to save two registers
 ldr r4,[r1]
 ldr r0,=fourway
 uxtab r5,r0,r4
 uxtab r6,r0,r4,ror#8
 uxtab r7,r0,r4,ror#16
 uxtab r8,r0,r4,ror#24
 stmia r1,{r5-r8}               @ Store fourway+perm[0], fourway+perm[1], fourway+perm[2], fourway+perm[3]

 bl gen_rand_sha                    @ Save some randomness for the resharing operation later
 movs r7,r0
 bl gen_rand_sha
 movs r8,r0

 ldr r2,=lut_a
 ldr r3,=lut_b
 ldr r0,[r2,#0x100]             @ R0 = a0 | a1<<8 | c0<<16 | c1<<24   (lut_a_map)
 eors r10,r0,r0,lsr#8
 uxtb r10,r10                   @ R10 = a0^a1
 ldr r1,[r3,#0x100]             @ R1 = b0 | b1<<8 | d0<<16 | d1<<24   (lut_b_map)
 eors r1,r0,r1
 eors r4,r1,r1,lsr#8
 uxtb r11,r4                    @ R11 = a0^a1^b0^b1
 eor r10,r10,r11,lsl#8          @ R10 = a0^a1 | (a0^a1^b0^b1)<<8
 movs r12,r1,ror#16             @ R12 = c0^d0 | (c1^d1)<<8 | junk<<16 | junk<<24
 
 ldr r1,=permscratch
 ldr r11,=chaff
 @ Using r1=permutedfourwaypointer, r2=lut_a, r3=lut_b, r7,r8=randomness, r10=(a0^a1)|(a0^a1^b0^b1)<<8, r11=chaff, r12=(c0^d0)|(c1^d1)<<8|junk
1:
 ands r5,r1,#12
 adds r5,r11,r5                 @ Align chaff address to r1
 ldr  r6,[r1],#4                @ r6 = fourway + perm[i] (i=0-3, loop iteration)
 ldr  r5,[r5]                   @ Random load to mask previous load
 
 ands r9,r6,#12                 @ r9 = chaff address aligned to r6 mod 16
 add  r9,r11,r9
 ldrb r4,[r6,#0]
 ldr  r14,[r9,#0]               @ Random load to mask previous load
 eor  r4,r4,r10
 eor  r4,r4,r14,lsl#8           @ Add in some junk in bits 8-31
 
 ldrb r5,[r6,#4]
 ldr  r14,[r9,#4]               @ Random load to mask previous load
 eors r4,r4,r5
 eor  r4,r4,r14,lsl#8           @ Add in some junk in bits 8-31
 
 ldrb r5,[r6,#8]
 ldr  r14,[r9,#8]               @ Random load to mask previous load
 eors r4,r4,r5
 eor  r4,r4,r14,lsl#8           @ Add in some junk in bits 8-31
 
 ldrb r5,[r6,#12]
 ldr  r14,[r9,#12]              @ Random load to mask previous load
 eors r4,r4,r5                  @ r4 = unsharedbyte[perm[i]]^a0^a1 | junk
 eor  r4,r4,r14,lsl#8           @ Add in some junk in bits 8-31
 
 ands r14,r4,#255
 ldrb r5,[r2,r14]                @ r5 = lut_a[unsharedbyte[perm[i]]^a0^a1]
 and  r14,r4,#15
 add  r14,r14,#32
 ldrb r14,[r11,r14]             @ Random load to mask previous load (r2 and r11 are both 0 mod 16)
 eors r5,r5,r12                 @ r5 = lut_a[unsharedbyte[perm[i]]^a0^a1]^c0^d0 | junk<<8 | junk<<16 | junk<<24
 @ split r5 into two shares and store at [r6,#0] and [r6,#4]
 strb r7,[r6,#0]
 eors r5,r5,r7
 strb r5,[r6,#4]

 mov r5,r10,lsr#8               @ r5=a0^a1^b0^b1
 ldr  r14,[r11,#44]             @ Need to eor into a random destination register
 eors r14,r4,r5                 @ r14 = unsharedbyte[perm[i]]^b0^b1 | junk<<8
 and r14,r14,#255

 ldrb r5,[r3,r14]               @ r5 = lut_b[unsharedbyte[perm[i]]^b0^b1]
 and  r14,r14,#15
 add  r4,r11,#24
 ldrb r14,[r4,r14]              @ Random load to mask previous load (r3==8 and r11==0 mod 16)
 eor  r5,r5,r12,ror#8           @ r5 = lut_b[unsharedbyte[perm[i]]^b0^b1]^c1^d1 | junk<<8 | junk<<16 | junk<<24
 @ split r5 into two shares and store at [r6,#8] and [r6,#12]
 strb r8,[r6,#8]
 eors r5,r5,r8
 strb r5,[r6,#12]

 movs r7,r7,ror#8
 movs r8,r8,ror#8

 tst r1,#12                     @ This does 4 loop iterations because permscratch is guaranteed to be 0 mod 16
 bne 1b

 ldr r0,=fourway
 ldmia r0,{r4-r7}               @ Load SBOXed values back into register r4-r7
 ldmia r11,{r8-r12,r14}         @ Random load to mask previous load and to obfuscate registers
 
 pop {r1-r3,r15}

.balign 4
.thumb_func
@ r1 = pointer to 4 x 4-way share (16 words); left unchanged
@ r3 = rkey_s+40*roundkeynumber; advanced by 40
@ Trashes r8-r11
@ If i = word number 0..3,
@ Aptr=memory word pointer to block of 20 bytes containing H&V-rotated share A roundkey (similarly B), then
@ vpermA=Aptr[4]>>30, vpermB=Bptr[4]>>30, and
@ roundkey shareA(i) = Aptr[i+vpermA mod 4] ror ((i+vpermA mod 4)^th byte of Aptr[4])
@ roundkey shareB(i) = Bptr[i+vpermB mod 4] ror ((i+vpermB mod 4)^th byte of Bptr[4])+16
storeroundkey:
 push {r2,r14}

@ eor two 4-way share components to make a component of a 2-way share
@ Note that we load from 4-way share at a random address then convert to 2-way share and
@ store at a fixed address, rather than the other way around, so that 2-way shares are obscured
@ by vperm (we don't know which 2-way share is being processed at a particular point in time).
@ And (if RK_ROR) we rotate first before EORing down to 2-way, so there is never an unrotated 2-way share

 bl gen_rand_sha             @ Get r0 = vperm for shareA of the round key
 str r0,[r3,#16]
 mov r8,r0,lsr#30
 rsb r8,r8,#0                @ r8=-vperm
.if RK_ROR
 movs r2,#0
 usub8 r2,r2,r0              @ r2=-hperms
.endif
 mov r9,#4
1:
 and r8,r8,#3
 adds r0,r1,r8,lsl#4

 ldmia r0,{r10,r11}
.if RK_ROR
 mov r10,r10,ror r2
 mov r11,r11,ror r2
 movs r2,r2,ror#8
.endif
 eor r10,r10,r11
 str r10,[r3],#4
 add r8,r8,#1
 subs r9,r9,#1
 bne 1b

 adds r1,r1,#8
 adds r3,r3,#4               @ skip over vperm (already stored)

 bl gen_rand_sha             @ Get r0 = vperm for shareB of the round key
 str r0,[r3,#16]
 mov r8,r0,lsr#30
 rsb r8,r8,#0                @ r8=-vperm
.if RK_ROR
 movs r2,#0
 usub8 r2,r2,r0              @ r2=-hperms
.endif
 mov r9,#4
1:
 and r8,r8,#3
 adds r0,r1,r8,lsl#4
 ldmia r0,{r10,r11}
.if RK_ROR
 mov r10,r10,ror r2
 mov r11,r11,ror r2
 movs r2,r2,ror#8
.endif
 mov r10,r10,ror#16
 mov r11,r11,ror#16
 eor r10,r10,r11
 str r10,[r3],#4
 add r8,r8,#1
 subs r9,r9,#1
 bne 1b

 subs r1,r1,#8               @ Restore r1 = (r1 on entry)
 adds r3,r3,#4               @ Set     r3 = (r3 on entry) + 40

 pop {r2,r15}

.balign 4
.thumb_func
init_key:
@ r0: rkeys_s (this input is ignored because it's defined here in the assembler file)
@ r1: raw key data (32 bytes)
@ rkeys_s is a 40*15=600-byte region
@ each of the 15 round keys is represented as two 4-word regions rka[0..3] and rkb[0..3] (each of which is followed by a zero word),
@ such that rk[i]=rka[i-r]^(rkb[i-r] ROR#16) gives the round keys, where r=!vpermkeyrot and i-r is interpreted in the relevant range, and i-r specifies mod 4

 push {r4-r11,r14}

.if IK_JITTER
 push {r0}
 bl gen_rand_sha
 mov r12,r0
 pop {r0}
.endif
 jitter r12

 mov r5,r1                   @ Here and for the rawkey reading loop, R5=raw key data

 jitter r12

 @ Make lots of small perms so that it's harder for attacker to correlate permutation creation steps with the permutation's use
 @ Can use rkey_s space because it won't be used before init_key_expandloop
 ldr r1,=rkey_s
 movs r2,#64
1:
 movs r0,#8
 push {r1,r2}
 bl makesmallperm            @ make a random permutation of 8 things (to randomise reading of key words)
 pop {r1,r2}
 adds r1,r1,#8
 subs r2,r2,#1
 bne 1b
 bl gen_rand_sha_nonpres                 @ Choose a random one of these 64 to use
 ands r0,r0,#63
 ldr r1,=rkey_s
 adds r7,r1,r0,lsl#3

init_key_loadrawkey:

 bl randomisechaff

@ Loading the raw key and turning it into 4-way shares for round 0 and 1
 ldr r11,=chaff              @ This needs to have 48 bytes of chaff
 sub r0,r7,r11; ands r0,r0,#15; add r10,r11,r0    @ align r10 to r7 mod 16   (permutation array)
 sub r0,r5,r11; ands r0,r0,#15; add r11,r11,r0    @ align r11 to r5 mod 16   (raw key data)
 ldr r4,=rkey4way            @ 128 byte scratch space for 4-way shares, laid out in words as a0 b0 c0 d0 a1 b1 c1 d1 ... a7 b7 c7 d7
 movs r6,#7
@ r4=rkey4way, r5=rawkeydata, r6=loopcounter, r7=permutationarray, r10,r11=zeroarray (same mod 16 alignment as r7,r5 resp)
2:
@ Do calls to gen_rand_sha before we have sensitive values, so that gen_rand_sha doesn't push them on the stack
 bl gen_rand_sha_nonpres; movs r8,r0
 bl gen_rand_sha_nonpres; movs r9,r0
 bl gen_rand_sha_nonpres; movs r1,r0
 bl gen_rand_sha                 @ r0,r1,r8,r9 are fresh random numbers
 ldrb r12,[r10,r6]           @ barrier to following load
 ldrb r2,[r7,r6]             @ r2 = perm8[r6] = which key word to load
 ldrb r12,[r10,r6]           @ barrier load to erase internal version of r2
 movs r14,r0,lsr#29          @ temporarily borrow some randomness to create a random address offset
 ldr  r12,[r11,r14,lsl#2]    @
 ldr  r3,[r11,r2,lsl#2]      @ barrier to following load (random value, same memory bank)
 ldr  r3,[r5,r2,lsl#2]       @ r3 = key word
 ldr  r12,[r11,r2,lsl#2]     @ barrier load to erase internal version of r3
 ldr  r12,[r11,r14,lsl#2]    @ erase internal address
 mov  r14,#0                 @ erase r14
 ldr  r12,[r11,#32]
 eor  r12,r12,r12
 eors r9,r3,r8               @ extra care: sacrifice random r9 to further mask this operation
 eors r3,r9,r0               @ r9=r0^r3^r8  (also has the effect of safely retiring the sensitive value r3)
 eors r3,r3,r1               @ r9=r0^r1^r3^r8 so r0,r1,r8,r9 is a 4-way share of r3
 adds r2,r4,r2,lsl#4
 stmia r2,{r0,r1,r3,r8}      @ Store 4-way share of this key word
 movs r0,#0                  @ Clear sensitive working values so they don't get used somehow (e.g., pushed onto the stack by gen_rand_sha)
 movs r1,#0
 movs r2,#0
 movs r3,#0
 subs r6,r6,#1
 bpl 2b
 mov r8,#0
 mov r9,#0


@ Now raw key is stored in rkey4way[], construct 2-way share in rkey_s[] for
@ the 128-bit roundkeys 0 and 1, then expand from 2 to 15 roundkeys.

 ldr r3,=rkey_s              @ r3=rkey_s
 ldr r1,=rkey4way            @ r1=rkey4way
 bl storeroundkey            @ Store round key 0 and advance r3 by 40
 adds r1,r1,#64
 bl storeroundkey            @ Store round key 1 and advance r3 by 40
 adds r1,r1,#48
 ldmia r1!,{r4-r7}           @ r4-r7 = 4-way share of previous round key word
                             @ r1=rkey4way+128 on entry to main loop
 movs r2,#0                  @ r2=word counter (0-51), offset from word 8
                             
@ Note that r1-r3 are not sensitive values, so it's safe to stack
@ them and conditionally branch on them.

@ rkey4way = 8 x 4 consecutive 4-way share words as cyclic buffer of
@   Rounds 0,1     Rounds 2,3            Rounds 12,13       Round 14
@   a0 b0 c0 d0 -> a8 b8 c8 d8 -> ... -> a48 b48 c48 d48 -> a56 b56 c56 d56
@   a1 b1 c1 d1 -> a9 b9 c9 d9           a49 b49 c49 d49    a57 b57 c57 d57
@   a2 b2 c2 d2    etc                   a50 b50 c50 d50    a58 b58 c58 d58
@   a3 b3 c3 d3                          a51 b51 c51 d51    a59 b59 c59 d59
@   a4 b4 c4 d4                          a52 b52 c52 d52    ===============
@   a5 b5 c5 d5                          a53 b53 c53 d53
@   a6 b6 c6 d6                          a54 b54 c54 d54
@   a7 b7 c7 d7                          a55 b55 c55 d55

init_key_expandloop:
 @ r1 = pointer past one of eight 4-way shares of a roundkey word in the above cyclic buffer (r1=rkey4way+16i for i=1,...,8)
 @ r2 = round key word counter (0-51), offset from word 8 (counting expanded roundkey words)
 @ r3 = pointer to rkey_s+40*roundnumber = rkey_s+40*(2+[r2/4])
 @ r4-r7 = 4-way share of previous roundkey word

 tst r2,#7
 bne 1f
 subs r1,r1,#128             @ Every 8th word, reset cyclic buffer pointer and do ROTWORD
 movs r4,r4,ror#8
 movs r5,r5,ror#8
 movs r6,r6,ror#8
 movs r7,r7,ror#8
1:

 tst r2,#3
 bne 1f
 bl init_key_sbox            @ Every 4th word, do SUBBYTES (sbox) on r4-r7
1:

 tst r2,#7
 bne 1f
 movs r0,r2,lsr#3
 mov r8,#1
 movs r8,r8,lsl r0
 eors r4,r4,r8               @ Every 8th word, add in round constant
1:

 ldmia r1,{r8-r11}           @ eor with key from two rounds ago and advance r1 by 16
 eors r4,r4,r8
 eors r5,r5,r9
 eors r6,r6,r10
 eors r7,r7,r11
 stmia r1!,{r4-r7}

 add r2,r2,#1
 tst r2,#3
 bne 1f
 subs r1,r1,#64
 bl storeroundkey            @ Store round key 1+r2/4 and advance r3 by 40
 adds r1,r1,#64
1:

 cmp r2,#52
 bne init_key_expandloop

 pop {r4-r11,r15}

@ Add the round key shares pointed to by r12 into the state shares
@ Trashes r0-r3
.balign 4
addrkey_s:

 ldr r0,=statevperm
 ldr r0,[r0]                 @ r0=vperm state rotation in bottom two bits
 ldr r1,[r12,#16]            @ r1=vperm key rotation in top two bits
 rsbs r3,r0,r1,lsr#30
 @ Read shareA of roundkey, offset by vpermkeyrot-vpermstaterot, and eor it into shareA of state, offset by -vpermstaterot
.if RK_ROR
 add r2,r12,#16
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; ldrb r1,[r2,r3]; rors r0,r0,r1; eors r4,r4,r0; adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; ldrb r1,[r2,r3]; rors r0,r0,r1; eors r5,r5,r0; adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; ldrb r1,[r2,r3]; rors r0,r0,r1; eors r6,r6,r0; adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; ldrb r1,[r2,r3]; rors r0,r0,r1; eors r7,r7,r0
.else
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; eors r4,r4,r0; adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; eors r5,r5,r0; adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; eors r6,r6,r0; adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; eors r7,r7,r0
.endif
 adds r12,r12,#20

 clear03                     @ barrier to clear internal load registers
 
 ldr r0,=statevperm
 ldr r0,[r0]                 @ r0=vperm state rotation in bottom two bits
 ldr r1,[r12,#16]            @ r1=vperm key rotation in top two bits
 rsbs r3,r0,r1,lsr#30
 @ Read shareB of roundkey, offset by vpermkeyrot-vpermstaterot, and eor it into shareB of state, offset by -vpermstaterot
.if RK_ROR
 add r2,r12,#16
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; ldrb r1,[r2,r3]; rors r0,r0,r1; eors r8,r8,r0;   adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; ldrb r1,[r2,r3]; rors r0,r0,r1; eors r9,r9,r0;   adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; ldrb r1,[r2,r3]; rors r0,r0,r1; eors r10,r10,r0; adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; ldrb r1,[r2,r3]; rors r0,r0,r1; eors r11,r11,r0
.else
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; eors r8,r8,r0;   adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; eors r9,r9,r0;   adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; eors r10,r10,r0; adds r3,r3,#1
 ands r3,r3,#3; ldr r0,[r12,r3,lsl#2]; eors r11,r11,r0  
.endif
 adds r12,r12,#20

 clear03 20                  @ barrier to clear internal load registers

 bx r14
 
.if NEED_ROUNDS

@ perform encryption rounds
@ r4-r7, r8-r11: state
@ Trashes r0-r3,r12
.balign 4
rounds_s:
 push {r14}
 mov r2,#0                   @ round counter
rounds_s_mainloop:
 ldr r12,=rkey_s
 add r12,r12,r2,lsl#5        @ pointer to key shares for this round
 add r12,r12,r2,lsl#3
 push {r2}                   @ save round count
 bl addrkey_s
 bl map_sbox_s
 bl shift_rows_s
.if ST_VPERM
 ldmia r13,{r2}              @ peek at stack to get round count
 cmp r2,#NUMREFSTATEVPERM
 bcs 1f
 bl refreshstatevperm        @ V shuffle of r4-r11
1:
.endif
 pop {r2}
 adds r2,r2,#1               @ increment round counter
 cmp r2,#14
 beq 2f                      @ break from loop? (last round has no mix_cols)
 push {r2}
 bl mix_cols_s
 pop {r2}
 b rounds_s_mainloop
2:
 ldr r12,=rkey_s+14*40       @ final round key shares
 bl addrkey_s
 @eor r0,r4,r8;bl logword
 @eor r0,r5,r9;bl logword
 @eor r0,r6,r10;bl logword
 @eor r0,r7,r11;bl logword
 pop {r15}
.endif

.if NEED_INV_ROUNDS
@ perform decryption rounds
@ r4-r7, r8-r11: state
@ preserves r0-r2
.balign 4
inv_rounds_s:
 push {r0-r2,r14}
 ldr r12,=rkey_s+14*40       @ final round key shares
 bl addrkey_s
 mov r2,#13                  @ round counter
 push {r2}
.if ST_VPERM
 bl gen_rand_sha
 bl vperm                    @ V shuffle
 push {r0}
.endif
 b 2f                        @ into middle of loop (last round has no mix_cols)
1:
 push {r2}
.if ST_VPERM
 bl gen_rand_sha
 bl vperm                    @ V shuffle
 push {r0}
.endif
 bl inv_mix_cols_s
2:
 bl inv_shift_rows_s
 bl inv_map_sbox_s
.if ST_VPERM
 pop {r0}
 bl vperm                    @ undo V shuffle
.endif
 pop {r2}
 ldr r12,=rkey_s
 add r12,r12,r2,lsl#5        @ pointer to key shares for this round
 add r12,r12,r2,lsl#3
 bl addrkey_s
 subs r2,r2,#1
 bpl 1b
 pop {r0-r2,r15}
.endif

.if INCLUDE_ENCRYPT_CBC
.balign 4
.thumb_func
@ encrypt data in place
@ r0: ivec
@ r1: buf: starts with plaintext; ends up with ciphertext
@ r2: number of blocks
@ this implementation does not scramble the shares properly; consider a better implementation
@ if security is required in encryption
cbc_encrypt_s:
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
 pop {r4-r11,r15}
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
 pop {r4-r11,r15}

.if ROUND_TRIP_TEST
1:
@ fault here
 add r13,#32
 mov r0,#1                   @ return fault status
 pop {r4-r11,r15}
.endif
.endif

@ Does mov r(i),#(0x80+i)*0x1010101 for i=flushfrom,flushfrom+1,...,12
@ Assume 0 <= flushfrom <= 3
@ Not possible to do this in a loop (or recursively) in gas without .altmacro?
.macro flush_regs flushfrom
.if \flushfrom<1
 mov r0,#0x80808080
.endif
.if \flushfrom<2
 mov r1,#0x81818181
.endif
.if \flushfrom<3
 mov r2,#0x83838383
.endif
 mov r3,  #0x83838383
 mov r4,  #0x84848484
 mov r5,  #0x85858585
 mov r6,  #0x86868686
 mov r7,  #0x87878787
 mov r8,  #0x88888888
 mov r9,  #0x89898989
 mov r10, #0x8a8a8a8a
 mov r11, #0x8b8b8b8b
 mov r12, #0x8c8c8c8c
.endm


@ numargs is the number of arguments of the function-to-be-wrapped (i.e., excluding systick), assumed to be <=3
.macro prewrap numargs
 push {r4-r12,r14}

@ Reset DWT count registers
 mov r4,#0xe0000000
 add r4,r4,#0x1000
 add r4,r4,#4
 mov r5,#0
 mov r6,#0
 stmia r4!,{r5-r6}
 add r4,r4,#8
 stmia r4!,{r5-r6}

@ Clear any possible pending SysTick interrupt status
 mov r4,#0xe0000000
 add r4,r4,#0xed00
 mov r5,#1<<25
 str r5,[r4,#4] @ ICSR at e000ed04

 isb sy
 dsb sy

@ Allow SysTick interrupts, depending on r0=0 or 1 input
 mov r0,r0,lsl#1
 add r0,r0,#5
 mov r4,#0xe000e000
 str r0,[r4,#0x10] @ SysTick CSR
 
 gpioput 16,1,r4,r5 @ ADC trigger high (starts power trace capture)

@ Shift arguments down to remove systick argument
.if \numargs>=1
 mov r0,r1
.if \numargs>=2
 mov r1,r2
.if \numargs>=3
 mov r2,r3
.endif
.endif
.endif

@ Set registers r\numargs - r12 to definite values
 flush_regs \numargs
@ Set r3 back to non-sentinel value in case the test program never changes r3 or r12 which would confuse the auto-detect of start/end
 mov r3,#0

.endm

@ numreturn is the number of return values, assumed to be 0 or 1
.macro postwrap numreturn
 gpioput 16,0,r1,r2 @ ADC trigger low
 flush_regs \numreturn
 mov r1,#0xe000e000
 mov r2,#4
 str r2,[r1,#0x10] @ Disable SysTick
 ldr r2,[r1,#0x18]
 ldr r1,=lastsystickcvr
 str r2,[r1]

@ Get final DWT cycle count
 ldr r1,=0xe0001000
 ldr r2,[r1,#4]
 ldr r1,=lastdwtcount
 str r2,[r1]
 
 pop {r4-r12,r15}
.endm


.if INCLUDE_CRYPT_CTR
.balign 4
.thumb_func
@ de/encrypt data in place
@ r0: ivec
@ r1: buf
@ r2: n, number of blocks, n>0
.if CT_BPERM
@ In AES-CTR each block can be independently en/decrypted as the encryption only depends on the IV,
@ the key, and the block number. We can therefore process them in any order, and using a
@ random order helps to defeat attacks that work on the output of the AES, since an attacker
@ wouldn't know what plaintext or ciphertext corresponds to a particular instruction.
.endif

ctr_crypt_s:

@ r0=IV, r1=cipher/plaintext buffer, r2=number of blocks
 push {r0,r4-r11,r14}
 
 push {r0-r2}

.if CT_BPERM
@ Initialise 32 random numbers (which fit in half-words)
 ldr r4,=bperm_rand
 movs r5,#32
1:
 bl gen_rand_sha
 umull r0,r3,r0,r2        @ Random number between 0 and n-1 (n=#blocks)
 strh r3,[r4],#2
 subs r5,r5,#1
 bne 1b
.endif

 bl randomisechaff
 pop {r0-r2}
 movs r3,#0

ctr_crypt_mainloop:
@ r0=IV, r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter

@ Do as much preparatory stuff as possible that doesn't involve the IV (to reduce interaction with it)
 push {r0-r2}

@ It's OK for execution time to depend on the block counter r3 ("public"), but not the block number (secret)

 tst r3,#(REFCHAFF_PERIOD-1)
 bne 1f
 push {r3}
 bl refreshchaff
 pop {r3}
 1:

 tst r3,#(REMAP_PERIOD-1)
 bne 1f
 push {r3}
 bl remap                    @ shuffle the LUts
 pop {r3}
 1:

 tst r3,#(REFROUNDKEYSHARES_PERIOD-1)
 bne 1f
 push {r3}
 bl ref_roundkey_shares_s    @ refresh the round key shares
 pop {r3}
 1:

 tst r3,#(REFROUNDKEYHVPERMS_PERIOD-1)
 bne 1f
 push {r3}
 bl ref_roundkey_hvperms_s   @ refresh the round key vperms
 pop {r3}
 1:

 pop {r0-r2}
@ r0=IV, r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter

@ Now calculate r12 = block number-to-be-deciphered from r3 = block counter
.if CT_BPERM
@ Use a "swap-or-not" method to generate an "oblivious" permutation; see makeperm.py version 7
 push {r0,r1}
 ldr r0,=murmur3_constants
 ldmia r0,{r9-r12,r14}       @ load five murmur3_32 hash constants
 ldr r0,=bperm_rand
 movs r1,#31
 movs r4,r3                  @ r4=i
1:
 ldrh r5,[r0],#2             @ r5=k
 subs r5,r5,r4               @ r5=k-i
 ands r6,r2,r5,asr#31        @ r6=n*(k-i<0)
 adds r5,r5,r6               @ r5=j=(k-i)%n
 adds r6,r4,r5               @ r6=i+j
 subs r7,r4,r5               @ r7=i-j
 and  r8,r7,r7,asr#31        @ r8=min(i-j,0)
 sub  r7,r7,r8,lsl#1         @ r7=|i-j|
 mla  r6,r6,r2,r7            @ r6=n(i+j)+|i-j|
 eors r6,r6,r1,lsl#27        @ mix with swap-or-not round counter to get different hash functions
@ Now do murmur3_32 hash of r6
 mul  r6,r6,r9
 movs r6,r6,ror#17
 mul  r6,r6,r10
 movs r6,r6,ror#19
 adds r6,r6,r6,lsl#2
 add  r6,r6,r11
 eors r6,r6,#4
 eors r6,r6,r6,lsr#16
 mul  r6,r6,r12
 eors r6,r6,r6,lsr#13
 mul  r6,r6,r14
 eors r6,r6,r6,lsr#16        @ not actually used here
@ Now set i to j, conditional on the top bit of r6
 subs r7,r5,r4               @ r7=j-i
 ands r7,r7,r6,asr#31        @ r7=(j-1)*(top bit of r6)
 adds r4,r4,r7               @ r4=j if top bit of r6, else i
 subs r1,r1,#1
 bpl 1b
 pop {r0,r1}
 mov r12,r4
.else
 mov r12,r3
.endif

@ r0=IV, r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter, r12=block to be deciphered
 push {r0-r3,r12}

processIV:                   @ non-target label to assist power analysis

@ It is not clear if the following addition of the block number in r12 to the IV can usefully
@ be done in terms of shares. Instead we do an addition and subtraction whose overall effect
@ is the same, and which provides a small degree of masking. The IV is not traditionally a secret,
@ though it will make it harder for the attacker if it is obscured.
 bl gen_rand_sha
 movs r8,r0,lsr#16           @ only use 16 low bits so we don't get any overflows in the following, and so that a carry from the first word is rare
 add r9,r8,r12               @ "masked" block number
@ r8=random, r9=(block number)+r8, stack=IV,...

 ldr r0,[r13]                @ peek at stack to restore r0=IV ptr
 ldmia r0,{r4-r7}            @ load IV
 clear03                     @ barrier to remove traces of IV from internal CPU load registers
 push {r0-r3}                @ We want to randomise the internal memory registers associated with the above LDM load, but this
 pop {r0-r3}                 @ may come from non-scratch memory and have its own internal registers, so we clear it using a
                             @ stack save/load. Either R13 is in non-scratch memory, in which case this works, or it isn't, in
                             @ which case it doesn't matter, because the only subsequent use of non-scratch memory is the stack.
 
@ Add in r9 in byte-big-endian, bit-little-endian (!) fashion, while trying to avoid rev operations
@ as far as possible as these tend to expose (via power fluctuations) byte-level hamming weights.
@ It's worth avoiding revs on r6, r5, r4, even at the cost of introducing a small timing dependency.

@ First do 128-bit addition of r9 to byte-reversed IV
 rev r7,r7; adds r7,r7,r9;            bcc 1f
 rev r6,r6; adcs r6,r6,#0; rev r6,r6; bcc 1f
 rev r5,r5; adcs r5,r5,#0; rev r5,r5; bcc 1f
 rev r4,r4; adcs r4,r4,#0; rev r4,r4
1:
@ At this point, r7 is reversed and r4-r6 are not
@ Now do 128-bit subtraction of r8 from byte-reversed IV
            subs r7,r7,r8; rev r7,r7; bcs 1f
 rev r6,r6; sbcs r6,r6,#0; rev r6,r6; bcs 1f
 rev r5,r5; sbcs r5,r5,#0; rev r5,r5; bcs 1f
 rev r4,r4; sbcs r4,r4,#0; rev r4,r4
1:
 clear01 16
 
@ r4-r7 = IV for the current block
 bl ns_to_s                  @ convert IV+x to shares, which includes choosing and incorporating a random shareC
 bl conjshareC               @ Add the effect of shareC to lut_a, lut_b
 bl rounds_s                 @ Do the 15 AES rounds on (key, state=IV+x), with the (shared) result in the state, R4-R11
 bl conjshareC               @ Undo the effect of shareC from lut_a, lut_b
.if ST_VPERM
 bl vpermundo                @ Undo vperm on the state shares
.endif

 pop {r0-r3,r12}
 push {r0,r3}
@ r0=IV, r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter, r12=block to be deciphered

@ Decrypt ciphertext using AES output in shares: r4-r11
.if ST_SHAREC
 ldr r0,=shareC
 ldr r0,[r0]
.else
 movs r0,#0
.endif
 add r1,r1,r12,lsl#4         @ Temporarily r1 points to block-to-be-deciphered
 ldr r3,[r1]
 eors r3,r3,r4
 eors r3,r3,r8,ror#16        @ Now r4 and r8 are free
 eors r3,r3,r0
 str r3,[r1]
 ldr r3,[r1,#4]
 eors r3,r3,r5
 eors r3,r3,r9,ror#16
 eors r3,r3,r0
 str r3,[r1,#4]
 ldr r3,[r1,#8]
 eors r3,r3,r6
 eors r3,r3,r10,ror#16
 eors r3,r3,r0
 str r3,[r1,#8]
 ldr r3,[r1,#12]
 eors r3,r3,r7
 eors r3,r3,r11,ror#16
 eors r3,r3,r0
 str r3,[r1,#12]
 sub r1,r1,r12,lsl#4         @ Restore r1 to point to start of buffer
 
 pop {r0,r3}                 @ Restore IV and block counter
@ r0=IV, r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter

 adds r3,r3,#1
 cmp r3,r2
 bne ctr_crypt_mainloop
 pop {r0,r4-r11,r15}

.endif

.section .text.debugging,"ax",%progbits

@@@@@@@@@@@@@@@@@@@@@@@@@ test functions @@@@@@@@@@@@@@@@@@@@@@@@@

@ .global test_v

@@ .section .text.test_v,"ax",%progbits
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

.extern o8hex
.extern osp
.extern onl

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
isr_systick:

 @ Stop SysTick counting
 mov r0,#0xe000e000
 mov r1,#4
 str r1,[r0,#0x10] @ SysTick Control and Status Register
 
 @ Clear any possible pending SysTick interrupt status due to SysTick count timing out during its own handler
 add r0,r0,#0xd00
 mov r1,#1<<25
 str r1,[r0,#4] @ ICSR at e000ed04

 gpioput 24,1,r2,r3 @ set GPIO24
 
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

@ Store DWT counts CYCCNT, CPICNT, LSUCNT, FOLDCNT in sysdata[18-21]
 ldr r1,=0xe0001004
 ldmia r1!,{r2,r3}
 stmia r0!,{r2,r3}
 add r1,r1,#8
 ldmia r1!,{r2,r3}
 stmia r0!,{r2,r3}

 gpioput 24,0,r2,r3 @ clear GPIO24

 bx r14

.balign 4
.thumb_func
@ Takes SHA256 of 64-bits (r0,r1) and stores the result at memory pointed to by r2 (32 bytes)
@ This is used to generate random inputs (key and IV) to repeated instances of the crypt code.
@ These random numbers are mimicked in powerpair.py which can then analyse the effect of these random inputs on the power signal.
@ Preserves r0-r13
gen_irand:
 push {r0-r8,r14}
 mov r8,r2
 ldr r4,=SHA256_BASE
 movw r2,#(1<<SHA256_CSR_BSWAP_LSB)+(1<<SHA256_CSR_START_LSB)
 str r2,[r4,#SHA256_CSR_OFFSET]        @ start SHA256 hardware
 str r0,[r4,#SHA256_WDATA_OFFSET]      @ 64-bit input in r0,r1
 str r1,[r4,#SHA256_WDATA_OFFSET]      @
 movs r2,#0x80                         @ End of message bit (with byte-swapped endianity) = start of message padding
 str r2,[r4,#SHA256_WDATA_OFFSET]
 movs r2,#12
 movs r3,#0
1:
 str r3,[r4,#SHA256_WDATA_OFFSET]
 subs r2,r2,#1
 bne 1b
 mov r2,#0x40000000          @ Specifies message length =  64 bits (with byte-swapped endianity)
 str r2,[r4,#SHA256_WDATA_OFFSET]
1:
 ldr r3,[r4,#SHA256_CSR_OFFSET]
 lsrs r3,r3,#SHA256_CSR_SUM_VLD_LSB+1
 bcc 1b                      @ wait for hardware to finish
 add r0,r4,#SHA256_SUM0_OFFSET
 ldmia r0,{r0-r7}
 stmia r8,{r0-r7}
 pop {r0-r8,r15}
