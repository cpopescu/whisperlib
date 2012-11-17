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
// A base for all streams
//

#ifndef __WHISPERLIB_IO_STREAM_BASE_H__
#define __WHISPERLIB_IO_STREAM_BASE_H__

#include <whisperlib/base/system.h>

namespace io {

class IoMarker;
class Seeker;

class StreamBase {
 public:
  StreamBase();
  explicit StreamBase(IoMarker* marker, Seeker* seeker);
  virtual ~StreamBase();

  IoMarker* marker() {
    return marker_;
  }
  // Is your job to dealocat it..
  void set_marker(IoMarker* marker) {
    marker_ = marker;
  }
  Seeker* seeker() {
    return seeker_;
  }
  // Is your job to dealocat it..
  void set_seeker(Seeker* seeker) {
    seeker_ = seeker;
  }

  bool CanMark() const {
    return marker_ != NULL;
  }
  bool CanSeek() const {
    return seeker_ != NULL;
  }

 protected:
  IoMarker* marker_;
  Seeker* seeker_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(StreamBase);
};
}

#endif  // __WHISPERLIB_IO_STREAM_BASE_H__
