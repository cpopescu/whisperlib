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

#ifndef __WHISPERLIB_SYNC_EVENT_H__
#define __WHISPERLIB_SYNC_EVENT_H__

#include <whisperlib/base/types.h>
#include <whisperlib/sync/mutex.h>

namespace synch {

class Event {
 protected:
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;

  bool is_signaled_;
  const bool manual_reset_;

 public:
  // is_signaled - initial state
  // manual_reset - when set, the event is reset on an explicit
  //                Reset call; else we reset it implicitly when
  //                releasing a thread from a Wait.
  Event(bool is_signaled, bool manual_reset);

  // All waiting threads are released.
  virtual ~Event();

  // Signals the event and releases all threads in manual_reset_ mode
  // or only *one* thread in automatic mode (!manual_reset_)
  void Signal();

  // resets the signal - makes sense only in manual_reset_ mode
  void Reset();

  // Wait for is_signaled_ to be turned on, else waits for a Signal
  // to happen
  void Wait();

  static const uint32 kInfiniteWait = 0xffffffff;

  // Same as Wait but w/ a given timeout in miliseconds. For an infinite
  // wait give Event::kInfiniteWait as timeout_ms.
  // At timeout_ms == 0 no wait happens
  // returns:
  //   true - if the event was signaled in time.
  //   false - if the event is not signaled, and the timeout
  //           period expired.
  bool Wait(uint32 timeout_in_ms);

 private:
  bool AutoResetLocked() {
    const bool ret = is_signaled_;
    if ( is_signaled_ && !manual_reset_ ) {
      is_signaled_ = false;
    }
    return ret;
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Event);
};
}

#endif  // __COMMON_SYNC_EVENT_H__
