#pragma once

// These options should be enabled because the security risk of not using them is too high
// or because the time cost is very low so you may as well have them.
// They can be set to 0 for analysis or testing purposes.

#ifndef GEN_RAND_SHA
#define GEN_RAND_SHA         1         // use SHA256 hardware to generate some random numbers
#endif
                                       // Some RNG calls are hard coded to LFSR RNG, others to SHA RNG
                                       // Setting GEN_RAND_SHA to 0 has the effect of redirecting the latter to LFSR RNG
#ifndef ST_SHAREC
#define ST_SHAREC            1         // This creates a partial extra share at almost no extra cost
#endif
#ifndef ST_VPERM
#define ST_VPERM             1         // insert random vertical permutations in state during de/encryption?
#endif
#ifndef CT_BPERM
#define CT_BPERM             1         // process blocks in a random order in counter mode?
#endif
#ifndef RK_ROR
#define RK_ROR               1         // store round key shares with random rotations within each word
#endif

// The following options should be enabled to increase resistance to glitching attacks.

#ifndef RC_CANARY
#define RC_CANARY            1         // use rcp_canary feature
#endif
#ifndef RC_COUNT
#define RC_COUNT             1         // use rcp_count feature
#endif

// Although enabling the following option likely has little theoretical benefit, in
// practice randomising the timing of operations can make side-channel attacks very
// much more effort to carry out. It can be disabled for analysis or testing purposes.

#ifndef RC_JITTER
#define RC_JITTER            1         // use random-delay versions of RCP instructions
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The following options can be adjusted, affecting the performance/security tradeoff

// Period = X means that the operation in question occurs every X blocks, so higher = more performance and lower security.
// No point in making them more than 16 or so, since the time taken by the subroutines would be negligible.
// These must be a power of 2. Timings as of commit 24277d13
//                                                                            RK_ROR=0    RK_ROR=1
//                                        Baseline time per 16-byte block = {    14066       14336 }                          cycles
#ifndef REFCHAFF_PERIOD
#define REFCHAFF_PERIOD             1     // Extra cost per 16-byte block = {      462         462 }/REFCHAFF_PERIOD          cycles
#endif
#ifndef REMAP_PERIOD
#define REMAP_PERIOD                4     // Extra cost per 16-byte block = {     4131        4131 }/REMAP_PERIOD             cycles
#endif
#ifndef REFROUNDKEYSHARES_PERIOD
#define REFROUNDKEYSHARES_PERIOD    1     // Extra cost per 16-byte block = {     1107        1212 }/REFROUNDKEYSHARES_PERIOD cycles
#endif
#ifndef REFROUNDKEYHVPERMS_PERIOD
#define REFROUNDKEYHVPERMS_PERIOD   1     // Extra cost per 16-byte block = {      936        1422 }/REFROUnDKEYVPERM_PERIOD  cycles
#endif

// Setting NUMREFSTATEVPERM to X means that state vperm refreshing happens on the first X AES rounds only,
// so lower = more performance and lower security.
// The rationale for doing it this way is that later rounds should be protected by CT_BPERM.
// NUMREFSTATEVPERM can be from 0 to 14.
#ifndef NUMREFSTATEVPERM
#define NUMREFSTATEVPERM            7     // Extra cost per 16-byte block =  80*NUMREFSTATEVPERM cycles
#endif
