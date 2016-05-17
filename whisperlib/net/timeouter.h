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
// Authors: Catalin Popescu

#ifndef __NET_BASE_TIMEOUTER_H__
#define __NET_BASE_TIMEOUTER_H__

#include <map>
#include "whisperlib/net/selector.h"
#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/callback.h"

namespace whisper {
namespace net {

class Timeouter {
 public:
  typedef Callback1<int64> TimeoutCallback;

  // callback: has to be permanent, we take ownership of it
  Timeouter(Selector* selector, TimeoutCallback* callback)
      : selector_(selector),
        callback_(callback),
        timeout_closures_() {
    CHECK(callback->is_permanent());
  }
  ~Timeouter() {
    UnsetAllTimeouts();
    delete callback_;
  }

  // Registers (or reregisters) a timeout call in timeout_in_ms ms from
  // this moment with the given timeout_id.
  // On timeout we call Connection::HandleTimeoutEvent, which in turn calls
  // the virtual HandleTimeout - override that to
  void SetTimeout(int64 timeout_id, int64 timeout_in_ms);
  // Clears a previously set timeout callback. Returns true if indeed a timeout
  // callback was set for that particular timeout_id
  bool UnsetTimeout(int64 timeout_id);

  // Clears all timeouts (normally on close / destructor)
  void UnsetAllTimeouts();

 private:
  void TimeoutFunction(int64 timeout_id) {
    // First clear the timeout that we got from the map of timeouts !
    CHECK(timeout_closures_.erase(timeout_id));
    callback_->Run(timeout_id);
  }

  Selector* const selector_;
  TimeoutCallback* const callback_;

  // Timeout callbacks - use them w/ SetTimeout/Unset
  typedef std::map<int64, Closure*> ClosureMap;
  ClosureMap timeout_closures_;
};
}  // namespace net
}  // namespace whisper

#endif  // __NET_BASE_TIMEOUTER_H__
