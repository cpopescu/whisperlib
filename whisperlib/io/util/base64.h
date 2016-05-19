// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Original source taken from:
// This is part of the libb64 project, and has been placed
// in the public domain.
// For details, see http://sourceforge.net/projects/libb64
//
// Modified 2009 WhisperSoft s.r.l.
//

#ifndef __NET_UTIL_BASE64_H__
#define __NET_UTIL_BASE64_H__

#include <string>
#include <vector>
#include "whisperlib/base/types.h"

namespace whisper {
namespace base64 {

//////////////////////////////////////////////////////////////////////
//
// BASE64 Encoding
//
struct  EncodeState {
  enum EncodeStep {
    STEP_A, STEP_B, STEP_C
  };
  EncodeStep step;
  char result;
  int stepcount;
};

void InitEncodeState(EncodeState* state_in);

static const char kEncoding[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline char EncodeValue(char value_in)  {
  if ( size_t(value_in) >= sizeof(kEncoding) )
    return '=';
  return kEncoding[(int)value_in];
}
static const int kCharsPerLine = 72;

int  EncodeBlock(const char* plaintext_in, int length_in,
                 char* code_out, EncodeState* state_in,
                 int chars_per_line);
int  EncodeBlockEnd(char* code_out, EncodeState* state_in);

struct Encoder {
  EncodeState state_;
  int buffersize_;
  Encoder(int buffersize_in = 4096)
      : buffersize_(buffersize_in) {
    InitEncodeState(&state_);
  }
  int Encode(char value_in) {
    return EncodeValue(value_in);
  }
  // IMPORTANT - you need twice length_in available at plaintext_out !!
  int Encode(const char* code_in, const int length_in,
             char* plaintext_out, int chars_per_line = kCharsPerLine) {
    return EncodeBlock(code_in, length_in, plaintext_out,
                       &state_, chars_per_line);
  }
  int EncodeEnd(char* plaintext_out) {
    return EncodeBlockEnd(plaintext_out, &state_);
  }
};

std::string EncodeString(const std::string& s);
std::string EncodeVector(const std::vector<uint8>& v);

//////////////////////////////////////////////////////////////////////
//
// BASE64 - Decoding

struct DecodeState {
  enum DecodeStep {
    STEP_A, STEP_B, STEP_C, STEP_D
  };
  DecodeStep step;
  uint8 decchar;
};

void InitDecodeState(DecodeState* state_in);

static const char kDecoding[] = {
  62, char(-1), char(-1), char(-1), 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
  char(-1), char(-1), char(-1), char(-2), char(-1), char(-1), char(-1),
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  char(-1), char(-1), char(-1), char(-1), char(-1), char(-1),
  26, 27, 28, 29, 30, 31, 32, 33, 34,
  35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

inline int DecodeValue(char value_in) {
  value_in -= 43;
  if (value_in < 0 || size_t(value_in) >= sizeof(kDecoding))
    return -1;
  return kDecoding[(int)value_in];
}

int DecodeValue(char value_in);

int DecodeBlock(const char* code_in, const int length_in,
                uint8* decoded_out, DecodeState* state_in);

struct Decoder {
  DecodeState state_;
  Decoder() {
    InitDecodeState(&state_);
  }
  int Decode(char value_in) {
    return DecodeValue(value_in);
  }
  int Decode(const char* code_in, const int length_in,
             uint8* decoded_out) {
    return DecodeBlock(code_in, length_in, decoded_out, &state_);
  }
};
}  // namespace base64
}  // namespace whisper

#endif  // BASE64_ENCODE_H
