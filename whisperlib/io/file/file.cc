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
#include "whisperlib/base/log.h"
// #include "whisperlib/base/common.h"
#include "whisperlib/base/core_errno.h"

#include "whisperlib/io/file/file.h"
#include "whisperlib/io/file/fd.h"

using namespace std;

#ifdef HAVE_LSEEK64
#define __LSEEK lseek64
#else
#define __LSEEK lseek
#endif

#ifdef HAVE_FDATASYNC
#define __FDATASYNC(fd) fdatasync(fd)
#else
  #ifdef F_FULLFSYNC
  #define __FDATASYNC(fd) fcntl((fd), F_FULLFSYNC)
  #else
  #define __FDATASYNC(fd) fsync(fd)
  #endif
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0;
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif

#if defined(HAVE_SYS_UIO_H)
static const int  kReadForWritevSize = 16384;
#endif

namespace whisper {
namespace io {

const char* File::AccessName(Access access) {
  switch ( access ) {
    CONSIDER(GENERIC_READ);
    CONSIDER(GENERIC_WRITE);
    CONSIDER(GENERIC_READ_WRITE);
  }
  return "UnknownAccess";
}

const char* File::CreationDispositionName(CreationDisposition cd) {
  switch ( cd ) {
    CONSIDER(CREATE_ALWAYS);
    CONSIDER(CREATE_NEW);
    CONSIDER(OPEN_ALWAYS);
    CONSIDER(OPEN_EXISTING);
    CONSIDER(TRUNCATE_EXISTING);
  }
  return "UnknownCreationDisposition";
}

const char* File::MoveMethodName(MoveMethod mm) {
  switch ( mm ) {
    CONSIDER(FILE_SET);
    CONSIDER(FILE_CUR);
    CONSIDER(FILE_END);
  }
  return "UnknownMoveMethod";
}

File::File()
  : filename_(""),
    fd_(INVALID_FD_VALUE),
    size_(0),
    position_(0) {
}

File::~File() {
  Close();
}

File* File::OpenFileOrDie(const char* filename) {
  File* const f = new File();
  CHECK(f->Open(filename, GENERIC_READ, OPEN_EXISTING));
  return f;
}

File* File::TryOpenFile(const char* filename) {
  File* const f = new File();
  if ( !f->Open(filename, GENERIC_READ, OPEN_EXISTING) ) {
    delete f;
    return NULL;
  }
  return f;
}

File* File::CreateFileOrDie(const char* filename) {
  File* const f = new File();
  CHECK(f->Open(filename, GENERIC_READ_WRITE, CREATE_ALWAYS));
  return f;
}
File* File::TryCreateFile(const char* filename) {
  File* const f = new File();
  if ( !f->Open(filename, GENERIC_READ_WRITE, CREATE_ALWAYS) ) {
    delete f;
    return NULL;
  }
  return f;
}


bool File::Open(const string& filename,  Access acc, CreationDisposition cd) {
  CHECK(!is_open());

  int flags = O_NOCTTY | O_LARGEFILE;
  switch ( cd ) {
  case CREATE_ALWAYS:     flags |= O_CREAT | O_TRUNC; break;
  case CREATE_NEW:        flags |= O_CREAT | O_EXCL; break;
  case OPEN_ALWAYS:       flags |= O_CREAT; break;
  case OPEN_EXISTING:     flags |= 0; break;
  case TRUNCATE_EXISTING: flags |= O_TRUNC; break;
  default:
    LOG_FATAL << filename << " - Invalid Disposition: " << cd;
  };

  mode_t mode = 0;
  switch ( acc )  {
  case GENERIC_READ:       flags |= O_RDONLY; mode = 00444; break;
  case GENERIC_WRITE:      flags |= O_WRONLY; mode = 00644; break;
  case GENERIC_READ_WRITE: flags |= O_RDWR;   mode = 00644; break;
  default:
    LOG_FATAL << filename << " Invalid Access: " << acc;
  };
  flags |= O_LARGEFILE;

  const int fd = ::open(filename.c_str(), flags, mode);
  if ( fd == INVALID_FD_VALUE ) {
    LOG_ERROR << "Cannot open file " << filename
              << " using access: " << AccessName(acc)
              << " creationDisposition: " << CreationDispositionName(cd)
              << " reason: " << GetLastSystemErrorDescription();
    return false;
  }

  filename_ = filename;
  fd_ = fd;
  UpdateSize();
  UpdatePosition();

  return true;
}
void File::Set(const string& filename, int fd) {
  filename_ = filename;
  fd_ = fd;
  UpdateSize();
  UpdatePosition();
}

void File::Close() {
  if ( !is_open() ) {
    return;
  }
  if ( ::close(fd_) < 0 ) {
    LOG_ERROR << filename_ << " - Error closing file."
              << " Reason: " << GetLastSystemErrorDescription();
  }
  fd_ = INVALID_FD_VALUE;
  filename_.clear();
  size_ = 0;
  position_ = 0;
}

uint64_t File::Size() const {
  CHECK(is_open());
  return size_;
}

uint64_t File::Position() const {
  CHECK(is_open());
  return position_;
}

int64_t File::SetPosition(int64_t distance,
                          MoveMethod move_method) {
  CHECK(is_open());
  int whence = SEEK_SET;
  switch ( move_method ) {
  case FILE_SET: whence = SEEK_SET; break;
  case FILE_CUR: whence = SEEK_CUR; break;
  case FILE_END: whence = SEEK_END; break;
  default:
    LOG_FATAL << filename_ << " Invalid MoveMethod : " << move_method;
  }

  const int64_t crt = ::__LSEEK(fd_, distance, whence);
  if ( crt < 0 ) {
    LOG_ERROR << filename_ << " lseek failed "
              << GetLastSystemErrorDescription();
    return -1;
  }
  // update cached position_
  position_ = uint64_t(crt);
  return position_;
}
void File::Rewind() {
    SetPosition(0, FILE_SET);
}

void File::Truncate(int64 pos) {
  if ( pos == -1 )  {
    pos = Position();
  }
  if ( ::ftruncate(fd_, pos) < 0 ) {
    LOG_ERROR << "::ftruncate() failed: " << GetLastSystemErrorDescription();
    return;
  }
  SetPosition(0, FILE_END);
  UpdateSize();
}

ssize_t File::ReadBuffer(void* buf, size_t len) {
  CHECK(is_open()) << filename_;
  const ssize_t cb = ::read(fd_, buf, len);
  if ( cb < 0 ) {
    LOG_ERROR << "::read() failed for file: [" << filename_
              << "], err: " << GetLastSystemErrorDescription();
    return cb;
  }
  position_ += cb;
  if ( size_ < position_ ) {
    // happens when the file gets bigger while we're reading
    UpdateSize();
  }
  return cb;
}

ssize_t File::Read(io::MemoryStream* out, size_t len) {
  char* buffer;
  ssize_t cb = 0;
  while ( size_t(cb) < len ) {
    size_t scratch;
    out->GetScratchSpace(&buffer, &scratch);
    if ( buffer == NULL ) {
        return -1;         // oom
    }
    const size_t cb_to_read = std::min(scratch, len - cb);
    const ssize_t cb_read = ReadBuffer(buffer, cb_to_read);
    if ( cb_read < 0 ) {
        out->ConfirmScratch(0);
        return cb_read;    // hard error
    }
    out->ConfirmScratch(cb_read);
    if ( cb_read == 0 ) {
        break;
    }
    cb += cb_read;
  }
  return cb;
}

void File::Skip(int64_t len) {
    SetPosition(len, FILE_CUR);
}

ssize_t File::WriteBuffer(const void* buf, size_t len) {
  DCHECK(is_open()) << filename_;
  const ssize_t cb = ::write(fd_, buf, len);
  if ( cb < 0 ) {
    LOG_ERROR << "::write() failed for file: [" << filename_
              << "], err: " << GetLastSystemErrorDescription();
    UpdatePosition();  // don't know where the file pointer ended-up
    return cb;
  }
  position_ += cb;
  size_ = std::max(size_, position_);
  return cb;
}

ssize_t File::Write(const string& s) {
  return WriteBuffer(s.data(), s.size());
}

ssize_t File::Write(io::MemoryStream* ms, ssize_t len) {
#if defined(HAVE_SYS_UIO_H)
  size_t scratch = 0;
  ssize_t cb = 0;
  while ( !ms->IsEmpty() && (len < 0 || cb < len) ) {
    ms->MarkerSet();
    struct ::iovec* iov = NULL;
    int iovcnt = 0;
    scratch = ms->ReadForWritev(&iov, &iovcnt,
                                (len < 0) ? kReadForWritevSize :
                                std::min(len - cb, ssize_t(kReadForWritevSize)));
    if ( iovcnt > 0 ) {
      const ssize_t crt_cb = ::writev(fd_, iov, iovcnt);
      delete [] iov;
      if ( crt_cb < 0 ) {
        ms->MarkerRestore();
        UpdatePosition();  // don't know where the file pointer ended-up
        size_ = std::max(size_, position_);
        return crt_cb;
      }
      if ( size_t(crt_cb) < scratch ) {
        // Written something but not all - we need to return and mark
        // the right amount in the buffer
        ms->MarkerRestore();
        ms->Skip(crt_cb);
      } else {
        // Everything written - no need for the marker
        ms->MarkerClear();
      }
      cb += crt_cb;
      if ( size_t(crt_cb) < scratch ) {
        break;
      }
    }
  }
  position_ += cb;
  size_ = std::max(size_, position_);
  return cb;
#else
  // Much slower version w/o writeev
  CHECK(is_open()) << filename_;
  if ( len < 0 ) {
    len = ms->Size();
  }
  const char* buf = NULL;
  size_t size = 0;
  ssize_t written = 0;

  while( len > 0 && ms->ReadNext(&buf, &size) ) {
    const size_t cb_to_write = std::min(len, size);
    const ssize_t cb_written = WriteBuffer(buf, cb_to_write);
    if ( cb_written < 0 ) {
      return cb_written;  // hard error
    }
    written += cb_written;
    if ( cb_written != cb_to_write ) {
      return written;     // whatever..
    }
    len -= cb_written;
  }
  return written;
#endif
}


void File::Flush() {
  CHECK(is_open()) << filename_;
  // Well this should not happen - we are screwed otherwise
  CHECK(::__FDATASYNC(fd_) != -1) << "::fdatasync() failed for file: ["
      << filename_ << "], err: " << GetLastSystemErrorDescription();
}


uint64_t File::UpdateSize() {
  const int64_t crt = ::__LSEEK(fd_, 0, SEEK_CUR);
  CHECK_GE(crt, 0) << GetLastSystemErrorDescription();
  const int64_t size =  ::__LSEEK(fd_, 0, SEEK_END);
  CHECK_GE(size, 0) << GetLastSystemErrorDescription();
  size_ = uint64_t(size);
  CHECK(::__LSEEK(fd_, crt, SEEK_SET) == crt)
      << " crt: " << crt << " - error: " << GetLastSystemErrorDescription();
  return size_;
}

uint64_t File::UpdatePosition() {
  const int64_t position = ::__LSEEK(fd_, 0, SEEK_CUR);
  CHECK_GE(position, 0) << GetLastSystemErrorDescription();
  position_ = uint64_t(position);
  return position_;
}
}  // namespace io
}  // namespace whisper
