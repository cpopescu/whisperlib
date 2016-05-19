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
// Author: Cosmin Tudorache

#include <algorithm>
#include "whisperlib/io/output_stream.h"
#include "whisperlib/io/input_stream.h"

ssize_t whisper::io::OutputStream::Write(io::InputStream& in, ssize_t len) {
  ssize_t written = 0;
  uint8 tmp[1024];
  while ( len < 0 || written < len ) {
    const size_t to_read = len < 0 ? sizeof(tmp) :
                                 std::min(size_t(len - written), sizeof(tmp));;
    // "in" -> tmp
    const ssize_t r = in.ReadBuffer(tmp, to_read);
    if ( r < 0 ) {
      return r;
    }
    // tmp -> "this"
    const ssize_t w = WriteBuffer(tmp, r);
    if ( w < 0 ) {
      return w;
    }
    written += w;
    // end of "in" reached or end of writable space in this stream
    if ( size_t(r) < to_read || w < r ) {
      break;
    }
  }
  return written;
}
