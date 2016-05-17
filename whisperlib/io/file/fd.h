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

#ifndef __COMMON_IO_FILE_FD_H__
#define __COMMON_IO_FILE_FD_H__

#include "whisperlib/base/types.h"

namespace whisper {
namespace io {

class FileDescriptor {
 public:
  FileDescriptor();
  virtual ~FileDescriptor();

  int fd() const {
    return fd_;
  }
  bool is_open() const {
    return fd_ != INVALID_FD_VALUE;
  }
  bool is_eof() const {
    return is_eof_;
  }
  bool is_live() const {
    return is_live_;
  }

  //  Use the given fd. This function fails if another fd is already opened.
  //  fd: is the file descriptor to use for our operations.
  //  live: if true specifies the fd stream never ends. The data in stream
  //        may exhaust temporarily, but new data arrives continuously.
  // duplicate: if true specifies the "fd" is duplicated first. The new fd
  //            is stored locally and closed on destructor or on Close() call.
  //            If the fd is not duplicated, it won't be closed on destructor
  //            nor on Close() call.
  virtual bool OpenFD(int fd, bool live = false, bool duplicate = false);


  // Closes the underline fd.
  void Close();

  // Reads len data bytes from the file to given buffer.
  // Returns # of bytes read / negative on error.
  virtual int32 Read(void* buf, int32 len);

  // Skips len data bytes in the file.
  // Returns # of skiped bytes / negative on error.
  virtual int32 Skip(int32 len);

  // Writes len data bytes from buffer to the file.
  // Returns # of written bytes / negative on error.
  virtual int32 Write(const void* buf, int32 len);

 protected:
  int fd_;                 // file descriptor
  bool is_live_;           // true = stream never ends (i.e. IsEos() returns
                           // always false)
  bool is_eof_;            // true = end of file was reached
  bool close_on_exit_;     // true = the fd is owned and will be closed
                           //        on destructor or on Close() call;

  DISALLOW_EVIL_CONSTRUCTORS(FileDescriptor);
};
}  // namespace io
}  // namespace whisper
#endif  // __COMMON_IO_FILE_FD_H__
