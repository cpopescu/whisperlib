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
// Authors: Cosmin Tudorache & Catalin Popescu


#include "whisperlib/net/selectable.h"

#include <unistd.h>
#include <fcntl.h>
#if defined(HAVE_SYS_UIO_H)
#include <sys/uio.h>
#endif
#include <sys/socket.h>
#include "whisperlib/base/core_errno.h"
#include "whisperlib/base/gflags.h"

using namespace std;

namespace whisper {
namespace net {

int32 Selectable::Write(const char* buf, int32 size) {
  const int32 cb = ::write(GetFd(), buf, size);
  if ( cb <= 0 ) {
    if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
      // Not really an error for non-blocking sockets
      return 0;
    }
    return -1;
  }
  return cb;
}

//////////////////////////////////////////////////////////////////////

// Is unclear so far if (and what) advantage seems to add writev
// to the overall performance..
//
// 10/6/2009:
// Updated note: apparently is better w/ new versions of kernel to use
//    writev (by some large margin..)
//

#if defined(HAVE_SYS_UIO_H)
#define __USE_WRITEV__
#endif

#ifdef  __USE_WRITEV__

//////////////////////////////////////////////////////////////////////

DEFINE_int32(default_read_for_writev_size, 16384,
             "You can tune this in order to get some "
             "better network performance");

//////////////////////////////////////////////////////////////////////

int32 Selectable::Write(io::MemoryStream* ms, int32 size) {
  int32 scratch = 0;
  int32 cb = 0;
#ifdef _DEBUG
  int64 initial_size = ms->Size();
#endif
  const int fd = GetFd();

  while ( !ms->IsEmpty() && (size < 0 || cb < size) ) {
    ms->MarkerSet();
    struct ::iovec* iov = NULL;
    int iovcnt = 0;
    scratch = ms->ReadForWritev(&iov, &iovcnt,
        (size < 0) ? FLAGS_default_read_for_writev_size :
          min(size-cb, FLAGS_default_read_for_writev_size));
    if ( iovcnt > 0 ) {
      const ssize_t crt_cb = ::writev(fd, iov, iovcnt);
      const int err = errno;
      delete [] iov;
      if ( crt_cb < 0 ) {
        ms->MarkerRestore();
        if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
          // Not really an error for non-blocking sockets
#ifdef _DEBUG
          DCHECK_EQ(initial_size, cb + ms->Size());
#endif
          return cb;
        }
        return -1;
      }
      if ( crt_cb < scratch ) {
        // Written something but not all - we need to return and mark
        // the right amount in the buffer
        ms->MarkerRestore();
        ms->Skip(crt_cb);
      } else {
        // Everything written - no need for the marker
        ms->MarkerClear();
      }
      cb += crt_cb;
      if ( crt_cb < scratch || err != 0 ) {
        break;   // EAGAIN || EWOULDBLOCK
      }
    } else {
      LOG_FATAL << "Dumb shit: " << scratch << " -- " << ms->Size();
    }
  }
#ifdef _DEBUG
  DCHECK_EQ(initial_size, cb + ms->Size());
#endif
  return cb;
}

//////////////////////////////////////////////////////////////////////

# else

int32 Selectable::Write(io::MemoryStream* ms, int32 len) {
  const char* buf;
  int size = 0;
  int32 cb = 0;
  while ( !ms->IsEmpty() && (len < 0 || cb < len)) {
    ms->MarkerSet();
    size = 0;
    CHECK(ms->ReadNext(&buf, &size))
        << " bugs: unempty MemoryStream is unreadable."
        << " ms " << ms->Size();
    CHECK_GE(size, 0);
    if ( size ) {
      const int32 sent = Write(buf, size);
      const int err = errno;
      if ( sent <= 0 ) {
        // Some error - nothing sent - restore the marker and bail out.
        // (Error treated in underlying Write)
        ms->MarkerRestore();
        return cb;
      } else if ( sent < size ) {
        // Written something but not all - we need to return and mark
        // the right ammount in the buffer
        ms->MarkerRestore();
        ms->Skip(sent);
      } else {
        // Everything written - no need for the marker
        ms->MarkerClear();
      }
      cb += sent;
      if ( sent < size && err != 0 ) {
        break;   // EAGAIN || EWOULDBLOCK
      }
    }
  }
  // Basically the entire buffer was sent.
  return cb;
}

#endif

//////////////////////////////////////////////////////////////////////

int32 Selectable::Read(char* buf, int32 size) {
  const int32 cb = ::read(GetFd(), buf, size);
  if ( cb <= 0 ) {
    if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
      // Not really an error for non-blocking sockets
      return 0;
    }
    return -1;
  }
  return cb;
}

int32 Selectable::Read(io::MemoryStream* ms, int32 size) {
  char* buffer;
  int32 cb = 0;
  while ( (size < 0) || (cb < size) ) {
    int32 scratch;
    ms->GetScratchSpace(&buffer, &scratch);
    const int32 received = Read(buffer, scratch);
    if ( received <= 0 ) {
      ms->ConfirmScratch(0);
      return cb;
    }
    cb += received;
    ms->ConfirmScratch(received);
  }
  return cb;
}
}  // namespace net
}  // namespace whisper
