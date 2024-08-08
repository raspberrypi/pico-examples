#pragma once

#ifndef CM_PROFILE
#define CM_PROFILE 0
#endif

#define DEBUG                0         // for use in debugging with serial output (timing not repeatable)
#define CHIPW                0         // change clock to 48MHz for use with CW hardware
#define SYSTICK_IMAP         0         // use SYSTICK to get a map of instruction execution (set DEBUG to 0 to get useful timings)
#define INCLUDE_ENCRYPT_CBC  0         // include code to perform encryption in CBC mode?
#define INCLUDE_DECRYPT_CBC  0         // include code to perform decryption in CBC mode?
#define INCLUDE_CRYPT_CTR    1         // include code to perform de/encryption in CTR mode?
#define ROUND_TRIP_TEST      0         // do the glitch detection test in CBC mode where we re-encrypt each block and compare against original ciphertext?
#define SBOX_VIA_INV         1         // compute (inverse) S-box values via a table of field inverses rather than via a direct table?
#define GEN_RAND_SHA         0         // use SHA256 hardware to generate random numbers (disable for Qemu testing)

#if ROUND_TRIP_TEST && !SBOX_VIA_INV
#error Sorry, if you want to do the round-trip test then SBOX_VIA_INV must also be set
#endif

#if CM_PROFILE==0

#define RANDOMIZE            0         // new random seed on each reset?
#define RC_CANARY            0         // use rcp_canary feature
#define RC_JITTER            0         // use random-delay versions of RCP instructions
#define RC_COUNT             0         // use rcp_count feature
#define IK_SHUFREAD          0         // read key bytes in random order?
#define IK_JUNK              0         // add some random distraction in init_key?
#define IK_PERM              0         // permute bytes (and possibly distraction bytes) in round key generation?
#define IK_REMAP             0         // remap S-box in round key generation?
#define IK_JITTER            0         // jitter timing in init_key?
#define RK_ROR               0         // store round keys with random RORs?
#define ST_HPERM             0         // insert random horizontal permutations in state during de/encryption?
#define ST_VPERM             0         // insert random vertical permutations in state during de/encryption?
#define CT_BPERM             0         // process blocks in a random order in counter mode?

#elif CM_PROFILE==1

#define RANDOMIZE            1         // new random seed on each reset?
#define RC_CANARY            0         // use rcp_canary feature
#define RC_JITTER            0         // use random-delay versions of RCP instructions
#define RC_COUNT             0         // use rcp_count feature
#define IK_SHUFREAD          1         // read key bytes in random order?
#define IK_JUNK              1         // add some random distraction in init_key?
#define IK_PERM              1         // permute bytes (and possibly distraction bytes) in round key generation?
#define IK_REMAP             1         // remap S-box in round key generation?
#define IK_JITTER            0         // jitter timing in init_key?
#define RK_ROR               1         // store round keys with random RORs?
#define ST_HPERM             0         // insert random horizontal permutations in state during de/encryption?
#define ST_VPERM             0         // insert random vertical permutations in state during de/encryption?
#define CT_BPERM             0         // process blocks in a random order in counter mode?

#elif CM_PROFILE==2

#define RANDOMIZE            1         // new random seed on each reset?
#define RC_CANARY            0         // use rcp_canary feature
#define RC_JITTER            0         // use random-delay versions of RCP instructions
#define RC_COUNT             0         // use rcp_count feature
#define IK_SHUFREAD          0         // read key bytes in random order?
#define IK_JUNK              0         // add some random distraction in init_key?
#define IK_PERM              0         // permute bytes (and possibly distraction bytes) in round key generation?
#define IK_REMAP             0         // remap S-box in round key generation?
#define IK_JITTER            0         // jitter timing in init_key?
#define RK_ROR               0         // store round keys with random RORs?
#define ST_HPERM             1         // insert random horizontal permutations in state during de/encryption?
#define ST_VPERM             1         // insert random vertical permutations in state during de/encryption?
#define CT_BPERM             0         // process blocks in a random order in counter mode?

#elif CM_PROFILE==3

#define RANDOMIZE            1         // new random seed on each reset?
#define RC_CANARY            0         // use rcp_canary feature
#define RC_JITTER            0         // use random-delay versions of RCP instructions
#define RC_COUNT             0         // use rcp_count feature
#define IK_SHUFREAD          0         // read key bytes in random order?
#define IK_JUNK              0         // add some random distraction in init_key?
#define IK_PERM              0         // permute bytes (and possibly distraction bytes) in round key generation?
#define IK_REMAP             0         // remap S-box in round key generation?
#define IK_JITTER            0         // jitter timing in init_key?
#define RK_ROR               0         // store round keys with random RORs?
#define ST_HPERM             0         // insert random horizontal permutations in state during de/encryption?
#define ST_VPERM             0         // insert random vertical permutations in state during de/encryption?
#define CT_BPERM             1         // process blocks in a random order in counter mode?

#elif CM_PROFILE==4

#define RANDOMIZE            1         // new random seed on each reset?
#define RC_CANARY            0         // use rcp_canary feature
#define RC_JITTER            0         // use random-delay versions of RCP instructions
#define RC_COUNT             0         // use rcp_count feature
#define IK_SHUFREAD          0         // read key bytes in random order?
#define IK_JUNK              0         // add some random distraction in init_key?
#define IK_PERM              0         // permute bytes (and possibly distraction bytes) in round key generation?
#define IK_REMAP             0         // remap S-box in round key generation?
#define IK_JITTER            1         // jitter timing in init_key?
#define RK_ROR               0         // store round keys with random RORs?
#define ST_HPERM             0         // insert random horizontal permutations in state during de/encryption?
#define ST_VPERM             0         // insert random vertical permutations in state during de/encryption?
#define CT_BPERM             0         // process blocks in a random order in counter mode?

#elif CM_PROFILE==5

#define RANDOMIZE            1         // new random seed on each reset?
#define RC_CANARY            1         // use rcp_canary feature
#define RC_JITTER            1         // use random-delay versions of RCP instructions
#define RC_COUNT             1         // use rcp_count feature
#define IK_SHUFREAD          1         // read key bytes in random order?
#define IK_JUNK              1         // add some random distraction in init_key?
#define IK_PERM              1         // permute bytes (and possibly distraction bytes) in round key generation?
#define IK_REMAP             1         // remap S-box in round key generation?
#define IK_JITTER            1         // jitter timing in init_key?
#define RK_ROR               1         // store round keys with random RORs?
#define ST_HPERM             1         // insert random horizontal permutations in state during de/encryption?
#define ST_VPERM             1         // insert random vertical permutations in state during de/encryption?
#define CT_BPERM             1         // process blocks in a random order in counter mode?

#endif

#if RC_COUNT && (INCLUDE_ENCRYPT_CBC || INCLUDE_DECRYPT_CBC)
#error Sorry, RC_COUNT is only tested in CTR mode
#endif

// derived values
#define NEED_ROUNDS          (INCLUDE_ENCRYPT_CBC || (INCLUDE_DECRYPT_CBC && ROUND_TRIP_TEST) || INCLUDE_CRYPT_CTR)
#define NEED_INV_ROUNDS      (INCLUDE_DECRYPT_CBC)
#define NEED_HPERM           (IK_PERM || ST_HPERM)
#define NEED_VPERM           (IK_PERM || ST_VPERM)