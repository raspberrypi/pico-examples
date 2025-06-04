#include <mbedtls/aes.h>
#include "pico/stdlib.h"

extern void lock_key();

int mb_aes_crypt_ctr_xor(mbedtls_aes_context *ctx,
    size_t length,
    unsigned char iv0[16],
    unsigned char nonce_xor[16],
    unsigned char stream_block[16],
    const unsigned char *input,
    unsigned char *output)
{
    int c;
    int ret = 0;
    size_t n = 0;
    uint32_t counter = 0;

    assert(length == (uint32_t)length);

    while (length--) {
        if (n == 0) {
            for (int i = 16; i > 0; i--) {
                nonce_xor[i-1] = iv0[i-1];
                if (i - (int)(16 - sizeof(counter)) > (int)0) {
                    nonce_xor[i-1] ^= (unsigned char)(counter >> ((16-i)*8));
                }
            }

            ret = mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, nonce_xor, stream_block);
            if (ret != 0) {
                break;
            }
            counter++;
        }
        c = *input++;
        *output++ = (unsigned char) (c ^ stream_block[n]);

        n = (n + 1) & 0x0F;
    }

    return ret;
}

void decrypt(uint8_t* key4way, uint8_t* IV_OTPsalt, uint8_t* IV_public, uint8_t(*buf)[16], int nblk) {
    mbedtls_aes_context aes;

    uint32_t aes_key[8];
    uint32_t* key4waywords = (uint32_t*)key4way;
    // Key is stored as a 4-way share of each word, ie X[0] = A[0] ^ B[0] ^ C[0] ^ D[0], stored as A[0], B[0], C[0], D[0]
    for (int i=0; i < count_of(aes_key); i++) {
        int skip = (i/4)*16;    // skip every other 16 words (64 bytes), due to the FIB workaround
        aes_key[i] = key4waywords[i*4 + skip]
                   ^ key4waywords[i*4 + 1 + skip]
                   ^ key4waywords[i*4 + 2 + skip]
                   ^ key4waywords[i*4 + 3 + skip];
    }

    uint8_t iv[16];
    for (int i=0; i < sizeof(iv); i++) {
        iv[i] = IV_OTPsalt[i] ^ IV_public[i];
    }

    int len = nblk * 16;

    mbedtls_aes_setkey_enc(&aes, (uint8_t*)aes_key, 256);

    lock_key();

    uint8_t xor_working_block[16] = {0};
    uint8_t stream_block[16] = {0};
    mb_aes_crypt_ctr_xor(&aes, len, (uint8_t*)iv, xor_working_block, stream_block, (uint8_t*)buf, (uint8_t*)buf);
}
