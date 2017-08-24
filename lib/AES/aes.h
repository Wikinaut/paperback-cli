#ifndef _AES_H
#define _AES_H

#include <stdint.h>

typedef unsigned char  uchar;

typedef struct
{
    uint32_t erk[64];      /* encryption round keys */
    uint32_t drk[64];      /* decryption round keys */
    int nr;             /* number of rounds */
}
aes_context;

int  aes_set_key( aes_context *ctx, uchar *key, int nbits );
void aes_encrypt( aes_context *ctx, uchar input[16], uchar output[16] );
void aes_decrypt( aes_context *ctx, uchar input[16], uchar output[16] );

#endif /* aes.h */
