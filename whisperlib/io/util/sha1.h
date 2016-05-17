#ifndef _LRAD_SHA1_H
#define _LRAD_SHA1_H

#include "whisperlib/base/types.h"

#ifdef __cplusplus
#include <string>
extern "C" {
#endif

/*
 *  FreeRADIUS defines to ensure globally unique SHA1 function names,
 *  so that we don't pick up vendor-specific broken SHA1 libraries.
 */
#define SHA1_CTX        librad_SHA1_CTX
#define SHA1Transform       librad_SHA1Transform
#define SHA1Init        librad_SHA1Init
#define SHA1Update      librad_SHA1Update
#define SHA1Final           librad_SHA1Final

typedef struct {
    uint32 state[5];
    uint32 count[2];
    uint8 buffer[64];
} SHA1_CTX;

void SHA1Transform(uint32 state[5], const uint8 buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, const uint8* data, unsigned int len);
void SHA1Final(uint8 digest[20], SHA1_CTX* context);

/*
 * this version implements a raw SHA1 transform, no length is appended,
 * nor any 128s out to the block size.
 */
void SHA1FinalNoLen(uint8 digest[20], SHA1_CTX* context);

/*
 * FIPS 186-2 PRF based upon SHA1.
 */
extern void fips186_2prf(uint8 mk[20], uint8 finalkey[160]);


///////////////////////////////////////////////////////////////////////////////////////////////////

void HmacSha1(const uint8* key, size_t key_len,
              const uint8* text, size_t text_len,
              uint8* digest /*[20]*/);

#ifdef __cplusplus
}
#endif

#endif /* _LRAD_SHA1_H */
