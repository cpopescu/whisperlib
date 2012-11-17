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
// Author: Catalin Popescu
//
// A base class for seek providers.
//
// We should probably, slowly phase this out.
//

#ifndef __WHISPERLIB_IO_SEEKER_H__
#define __WHISPERLIB_IO_SEEKER_H__

#include <whisperlib/base/types.h>

namespace io {

class Seeker {
 public:
  enum Position {
    STREAM_SET = 0,
    STREAM_CUR,
    STREAM_END
  };
  Seeker() {}
  virtual ~Seeker() {}

  //////////////////////////////////////////////////////////////////////
  //
  // Interface functions:
  //

  // Seeks to the offset - the main meet function
  virtual int64 Seek(int64 offset, Position relative_position) = 0;
  // Returns the current position in the underlying stream
  virtual int64 Tell() const = 0;

  //////////////////////////////////////////////////////////////////////
  //
  // Helper functions:
  //

  // Helper for going to an absolute position
  int64 Seek(int64 absolute_offset) {
    return Seek(absolute_offset, STREAM_SET);
  }
  // Helper for going to the beginning of a stream
  int64 SeekToBegin() { return Seek(0, STREAM_SET); }
  // Helper for going at the end of the stream
  int64 SeekToEnd() { return Seek(0, STREAM_END); }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Seeker);
};
}

#endif  // ___WHISPERLIB_IO_SEEKER_H__
