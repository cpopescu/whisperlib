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

#ifndef __COMMON_IO_FILE_FD_INPUT_STREAM_H__
#define __COMMON_IO_FILE_FD_INPUT_STREAM_H__

#include <whisperlib/io/input_stream.h>
#include <whisperlib/io/file/fd.h>

//
// An Input Stream which reads data from a generic file descriptor with
// no SEEK capabilities.
//

namespace io {

class FDInputStream : public InputStream {
 public:
  FDInputStream(FileDescriptor* fd);
  virtual ~FDInputStream();

  // Input Stream interface
  virtual int32 Read(void* buffer, int32 len);
  virtual int32 Peek(void* buffer, int32 len) {
    return 0;  // unsupported
  }
  virtual int64 Skip(int64 len);

  // You never know how much data is there ..
  virtual int64 Readable() const {
    return -1;
  }
  virtual bool IsEos() const;

 private:
  FileDescriptor* const fd_;

  DISALLOW_EVIL_CONSTRUCTORS(FDInputStream);
};
}

#endif  // __COMMON_IO_FILE_FD_INPUT_STREAM_H__
