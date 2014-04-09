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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// #include "whisperlib/base/common.h"
#include "whisperlib/base/core_errno.h"
#include "whisperlib/base/log.h"
#include "whisperlib/io/file/fd.h"

namespace io {

FileDescriptor::FileDescriptor()
  : fd_(INVALID_FD_VALUE),
    is_live_(false),
    is_eof_(true),
    close_on_exit_(true) {
}

FileDescriptor::~FileDescriptor() {
  Close();
}

bool FileDescriptor::OpenFD(int fd,
                            bool is_live,       // = false
                            bool duplicate) {   // = false
  CHECK(!is_open());
  if ( duplicate ) {
    fd_ = dup(fd);
    if ( fd_ == INVALID_FD_VALUE ) {
      LOG_ERROR << " dup (fd=" << fd << ") failed with error: "
                << GetLastSystemErrorDescription();
      return false;
    }
  } else {
    fd_ = fd;
  }
  close_on_exit_ = duplicate;
  is_live_ = is_live;
  is_eof_ = false;
  DCHECK(is_open());

  return true;
}

void FileDescriptor::Close() {
  if ( close_on_exit_ ) {
    ::close(fd_);
  }
  fd_ = INVALID_FD_VALUE;
  is_live_ = false;
  is_eof_ = true;
  close_on_exit_ = false;
}

int32 FileDescriptor::Read(void* buf, int32 len) {
  DCHECK(is_open());

  int32 cb = ::read(fd_, buf, len);
  if ( cb < 0 ) {
    if ( GetLastSystemError() == EAGAIN ) {
      cb = 0;
    } else {
      LOG_ERROR << "::read failed: " << GetLastSystemErrorDescription();
      return -1;
    }
  }
  if ( cb < len && !is_live_ ) {
    DLOG_INFO <<  "Reached EOF for fd: " << fd_;
    is_eof_ = true;
  }

  return cb;
}

int32 FileDescriptor::Write(const void* buf, int32 len) {
  DCHECK(is_open());
  const int32 cb = ::write(fd_, buf, len);
  if ( cb < 0 ) {
    LOG_ERROR << "::write failed: " << GetLastSystemErrorDescription();
    return cb;
  }
  return cb;
}

int32 FileDescriptor::Skip(int32 len) {
  DCHECK(is_open());

  uint8 tmp_buf[256] = { 0, };
  int32 total_read = 0;
  int32 crt_read = 0;
  while ( total_read < len ) {
    const int32 to_read = min(static_cast<int32>(sizeof(tmp_buf)), len);
    if ( (crt_read = Read(tmp_buf, to_read) <= 0 ) ) {
      break;
    }
    total_read += crt_read;
  }
  return total_read;
}
}
