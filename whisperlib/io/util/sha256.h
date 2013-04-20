/*
 * FILE:	sha2.h
 * AUTHOR:	Aaron D. Gifford - http://www.aarongifford.com/
 *
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: sha2.h,v 1.1 2001/11/08 00:02:01 adg Exp adg $
 *
 * Hmac extension Catalin Popescu
 * (c) 2009 Whispersoft srl.
 */

#ifndef __SHA256_H__
#define __SHA256_H__

#include <whisperlib/base/types.h>

#ifdef __cplusplus
#include <string>
extern "C" {
#endif

/*** SHA-256/384/512 Various Length Definitions ***********************/
#define SHA256_BLOCK_LENGTH		64
#define SHA256_DIGEST_LENGTH		32
#define SHA256_DIGEST_STRING_LENGTH	(SHA256_DIGEST_LENGTH * 2 + 1)

/*** SHA-256/384/512 Context Structures *******************************/
/* NOTE: If your architecture does not define either u_intXX_t types or
 * uintXX_t (from inttypes.h), you may need to define things by hand
 * for your system:
 */

typedef struct __SHA256_CTX {
  uint32 state[8];
  uint64 bitcount;
  uint8  buffer[SHA256_BLOCK_LENGTH];
} _SHA256_CTX;

/*** SHA-256 Function Prototypes ******************************/

void SHA256_Init(_SHA256_CTX* context);
void SHA256_Update(_SHA256_CTX* context, const uint8*, size_t);
void SHA256_Final(uint8* digest /*[SHA256_DIGEST_LENGTH]*/, _SHA256_CTX* context);
char* SHA256_End(_SHA256_CTX* context, char* buffer /*[SHA256_DIGEST_STRING_LENGTH]*/);
void SHA256_Data(const uint8* data, size_t len,
                 uint8* digest /*[SHA256_DIGEST_LENGTH]*/);
char* SHA256_DataHex(const uint8* data, size_t len,
                     char* digest  /*[SHA256_DIGEST_STRING_LENGTH]*/);

int HmacSha256PrepareKey(const uint8* key,
                         size_t key_len,
                         uint8* key_to_use /*[SHA256_DIGEST_LENGTH]*/);
void HmacSha256WithPreparedKey(const uint8* key_to_use,
                               size_t key_to_use_len,
                               const uint8* data,
                               size_t data_len,
                               uint8* result /*[SHA256_DIGEST_LENGTH]*/);
void HmacSha256(const uint8* key, size_t key_len,
                const uint8* data, size_t data_len,
                uint8* result /*[SHA256_DIGEST_LENGTH]*/);

#ifdef  __cplusplus
}

string SHA256_StringHex(const char* str);

#endif /* __cplusplus */


#endif /* __SHA256_H__ */
