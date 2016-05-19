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

#ifndef __WHISPERLIB_IO_OUTPUT_STREAM_H__
#define __WHISPERLIB_IO_OUTPUT_STREAM_H__

#include <string.h>             // strlen
#include <string>
#include "whisperlib/base/types.h"

namespace whisper {
namespace io {

class InputStream;
class OutputStream {
 public:
  OutputStream() {
  }
  virtual ~OutputStream() {
  }

  // Append "len" bytes of data from the given "buffer" to the stream
  // Returns the number of bytes written. Can be less than len if the stream
  // has not enough space, or negative on error.
  virtual ssize_t WriteBuffer(const void* buffer, size_t len) = 0;

  // Append "len" bytes of data from the given input stream "in" to the stream.
  // The default implementation uses a simple copy through a temporary
  // char[] buffer.
  // Subclases are free to provide a particulary better implementation.
  // Returns the number of bytes written. Can be less than len if "in" contains
  // fewer bytes or the stream has not enough space.
  // Returns negative on error.
  virtual ssize_t Write(io::InputStream& in, ssize_t len = -1);

  // Returns the space available for write. If the stream is unlimited return -1
  virtual uint64_t Writable() const = 0;

  // Convenience function for writing a NULL terminated string.
  ssize_t Write(const char* text) {
    return WriteBuffer(text, strlen(text));
  }

  // Convenience function for a string
  ssize_t WriteString(const std::string& s) {
    return WriteBuffer(s.data(), s.size());
  }
  ssize_t Write(const std::string& s) {
    return WriteBuffer(s.data(), s.size());
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(OutputStream);
};
}  // namespace io
}  // namespace whisper
#endif  // __WHISPERLIB_IO_OUTPUT_STREAM_H__
