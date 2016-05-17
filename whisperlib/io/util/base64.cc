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
//
// This is part of the libb64 project, and has been placed
// in the public domain.
// For details, see http://sourceforge.net/projects/libb64
//
// Modified 2009 WhisperSoft s.r.l.
//

#include <vector>
#include <string>
#include "whisperlib/io/util/base64.h"

using std::string;

namespace whisper {
namespace base64 {

string EncodeString(const string& s) {
  const int32 buflen = 2 * s.size() + 4;
  char* encbuf = new char[buflen];
  base64::Encoder encoder;
  const int32 len = encoder.Encode(s.c_str(), s.size(), encbuf, buflen);
  const int32 len2 = encoder.EncodeEnd(encbuf + len);
  *(encbuf + len + len2) = '\0';
  const string ret(encbuf, len + len2);
  delete [] encbuf;
  return ret;
}
string EncodeVector(const std::vector<uint8>& v) {
  const int32 buflen = 2 * v.size() + 4;
  char* encbuf = new char[buflen];
  base64::Encoder encoder;
  const int32 len = encoder.Encode((const char*)&(v[0]), v.size(), encbuf, buflen);
  const int32 len2 = encoder.EncodeEnd(encbuf + len);
  *(encbuf + len + len2) = '\0';
  const string ret(encbuf, len + len2);
  delete [] encbuf;
  return ret;
}

void InitEncodeState(EncodeState* state_in) {
  state_in->step = EncodeState::STEP_A;
  state_in->result = 0;
  state_in->stepcount = 0;
}


int EncodeBlock(const char* plaintext_in, int length_in,
                char* code_out, EncodeState* state_in,
                int chars_per_line) {
  const char* plainchar = plaintext_in;
  const char* const plaintextend = plaintext_in + length_in;
  char* codechar = code_out;
  char result;
  char fragment;

  result = state_in->result;

  switch (state_in->step) {
    while (1) {
      case EncodeState::STEP_A:
        if (plainchar == plaintextend) {
          state_in->result = result;
          state_in->step = EncodeState::STEP_A;
          return codechar - code_out;
        }
        fragment = *plainchar++;
        result = (fragment & 0x0fc) >> 2;
        *codechar++ = EncodeValue(result);
        result = (fragment & 0x003) << 4;
      case EncodeState::STEP_B:
        if (plainchar == plaintextend) {
          state_in->result = result;
          state_in->step = EncodeState::STEP_B;
          return codechar - code_out;
        }
        fragment = *plainchar++;
        result |= (fragment & 0x0f0) >> 4;
        *codechar++ = EncodeValue(result);
        result = (fragment & 0x00f) << 2;
      case EncodeState::STEP_C:
        if (plainchar == plaintextend) {
          state_in->result = result;
          state_in->step = EncodeState::STEP_C;
          return codechar - code_out;
        }
        fragment = *plainchar++;
        result |= (fragment & 0x0c0) >> 6;
        *codechar++ = EncodeValue(result);
        result  = (fragment & 0x03f) >> 0;
        *codechar++ = EncodeValue(result);

        ++(state_in->stepcount);
        if (chars_per_line > 0 && state_in->stepcount == chars_per_line/4) {
          *codechar++ = '\n';
          state_in->stepcount = 0;
        }
    }
  }
  // control should not reach here
  return codechar - code_out;
}

int EncodeBlockEnd(char* code_out, EncodeState* state_in) {
  char* codechar = code_out;
  switch (state_in->step) {
    case EncodeState::STEP_B:
      *codechar++ = EncodeValue(state_in->result);
      *codechar++ = '=';
      *codechar++ = '=';
      break;
    case EncodeState::STEP_C:
      *codechar++ = EncodeValue(state_in->result);
      *codechar++ = '=';
      break;
    case EncodeState::STEP_A:
      break;
  }
  return codechar - code_out;
}

//////////////////////////////////////////////////////////////////////

void InitDecodeState(DecodeState* state_in) {
  state_in->step = DecodeState::STEP_A;
  state_in->decchar = 0;
}

int DecodeBlock(const char* code_in, const int length_in,
                uint8* decoded_out, DecodeState* state_in) {
  const char* codechar = code_in;
  uint8* decchar = decoded_out;
  char fragment;

  *decchar = state_in->decchar;

  switch (state_in->step) {
    while (1) {
      case DecodeState::STEP_A:
        do {
          if (codechar == code_in+length_in) {
            state_in->step = DecodeState::STEP_A;
            state_in->decchar = *decchar;
            return decchar - decoded_out;
          }
          fragment = (char)DecodeValue(*codechar++);
        } while (fragment < 0);
        *decchar    = (fragment & 0x03f) << 2;
      case DecodeState::STEP_B:
        do {
          if (codechar == code_in+length_in) {
            state_in->step = DecodeState::STEP_B;
            state_in->decchar = *decchar;
            return decchar - decoded_out;
          }
          fragment = (char)DecodeValue(*codechar++);
        } while (fragment < 0);
        *decchar++ |= (fragment & 0x030) >> 4;
        *decchar    = (fragment & 0x00f) << 4;
      case DecodeState::STEP_C:
        do {
          if (codechar == code_in+length_in) {
            state_in->step = DecodeState::STEP_C;
            state_in->decchar = *decchar;
            return decchar - decoded_out;
          }
          fragment = (char)DecodeValue(*codechar++);
        } while (fragment < 0);
        *decchar++ |= (fragment & 0x03c) >> 2;
        *decchar    = (fragment & 0x003) << 6;
      case DecodeState::STEP_D:
        do {
          if (codechar == code_in+length_in) {
            state_in->step = DecodeState::STEP_D;
            state_in->decchar = *decchar;
            return decchar - decoded_out;
          }
          fragment = (char)DecodeValue(*codechar++);
        } while (fragment < 0);
        *decchar++   |= (fragment & 0x03f);
    }
  }
  // control should not reach here
  return decchar - decoded_out;
}

}  // namespace base64
}  // namespace whisper
