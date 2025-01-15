#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////////////////


// These options should be enabled in production because the security risk of not using them is too high
// or because the time cost is very low so you may as well have them.
// (Can be set to 0 for analysis/diagnosis purposes.)

#define GEN_RAND_SHA         1         // use SHA256 hardware to generate some random numbers (disable for Qemu testing)
                                       // Some RNG calls are hard coded to LFSR RNG, others to SHA RNG
                                       // Setting GEN_RAND_SHA to 0 has the effect of redirecting the latter to LFSR RNG
#define ST_SHAREC            1         // This creates a partial extra share at almost no extra cost

#define IK_JITTER            1         // jitter timing in init_key? Need to keep this at 0 for analysis purposes, but change to 1 in production
#define ST_JITTER            1         // jitter timing in decryption? Need to keep this at 0 for analysis purposes, but change to 1 in production
#define ST_VPERM             1         // insert random vertical permutations in state during de/encryption?
#define CT_BPERM             1         // process blocks in a random order in counter mode?

#define RANDOMIZE            3         // 0 means RNG reset to the same thing on every call to *crypt_s; 3 means fully random
                                       // Currently overridden at runtime by analysis code

////////////////////////////////////////////////////////////////////////////////////////////////////////////


// The following options can be adjusted, affecting the performance/security tradeoff

// Period = X means that the operation in question occurs every X blocks, so higher = more performance and lower security.
// No point in making them more than 16 or so, since the time taken by the subroutines would be negligible
// These must be a power of 2. Timings as of commit 24277d13
//                                                                            RK_ROR=0    RK_ROR=1
//                                        Baseline time per 16-byte block = {    14066       14336 }                          cycles
#define REFCHAFF_PERIOD             1     // Extra cost per 16-byte block = {      462         462 }/REFCHAFF_PERIOD          cycles
#define REMAP_PERIOD                4     // Extra cost per 16-byte block = {     4131        4131 }/REMAP_PERIOD             cycles
#define REFROUNDKEYSHARES_PERIOD    1     // Extra cost per 16-byte block = {     1107        1212 }/REFROUNDKEYSHARES_PERIOD cycles
#define REFROUNDKEYHVPERMS_PERIOD   1     // Extra cost per 16-byte block = {      936        1422 }/REFROUnDKEYVPERM_PERIOD  cycles

// Setting this to X means that state vperm refreshing happens on the first X AES rounds only,
// so lower = more performance and lower security.
// The rationale for doing it this way is that later rounds should be protected by CT_BPERM
// This can be from 0 to 14
#define NUMREFSTATEVPERM            7     // Extra cost per 16-byte block =  80*NUMREFSTATEVPERM cycles

#define RK_ROR                      1

///////////////////////////////////////////////////////////////////////////////////////////////////////////


// Changing these options is not currently supported

#define DEBUG                0         // for use in debugging with serial output (timing not repeatable)
#define CHIPW                0         // change clock to 48MHz for use with CW hardware
#define INCLUDE_ENCRYPT_CBC  0         // include code to perform encryption in CBC mode?
#define INCLUDE_DECRYPT_CBC  0         // include code to perform decryption in CBC mode?
#define INCLUDE_CRYPT_CTR    1         // include code to perform de/encryption in CTR mode?
#define ROUND_TRIP_TEST      0         // do the glitch detection test in CBC mode where we re-encrypt each block and compare against original ciphertext?
#define SBOX_VIA_INV         0         // compute (inverse) S-box values via a table of field inverses rather than via a direct table?
#if ROUND_TRIP_TEST && !SBOX_VIA_INV
#error Sorry, if you want to do the round-trip test then SBOX_VIA_INV must also be set
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////


// derived values
#define NEED_ROUNDS          (INCLUDE_ENCRYPT_CBC || (INCLUDE_DECRYPT_CBC && ROUND_TRIP_TEST) || INCLUDE_CRYPT_CTR)
#define NEED_INV_ROUNDS      (INCLUDE_DECRYPT_CBC)
#define NEED_VPERM           (ST_VPERM)
