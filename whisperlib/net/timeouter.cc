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

#include "whisperlib/net/timeouter.h"

using namespace std;

namespace whisper {
namespace net {

void Timeouter::SetTimeout(int64 timeout_id, int64 timeout_in_ms) {
  Closure* closure = NULL;
  ClosureMap::iterator it = timeout_closures_.find(timeout_id);
  if ( it != timeout_closures_.end() ) {
    closure = it->second;
  } else {
    closure = NewCallback(this, &Timeouter::TimeoutFunction, timeout_id);
    timeout_closures_.insert(make_pair(timeout_id, closure));
  }
  selector_->RegisterAlarm(closure, timeout_in_ms);
}

bool Timeouter::UnsetTimeout(int64 timeout_id) {
  ClosureMap::iterator it = timeout_closures_.find(timeout_id);
  if ( it == timeout_closures_.end() ) {
    return false;
  }
  Closure* c = it->second;
  timeout_closures_.erase(it);
  delete c;
  selector_->UnregisterAlarm(c);
  return true;
}

void Timeouter::UnsetAllTimeouts() {
  for ( ClosureMap::iterator it = timeout_closures_.begin();
        it != timeout_closures_.end(); ++it ) {
    selector_->UnregisterAlarm(it->second);
    delete it->second;
  }
  timeout_closures_.clear();
}
}  // namespace net
}  // namespace whisper
