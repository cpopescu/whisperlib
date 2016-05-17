// Copyright (c) 2011, Whispersoft s.r.l.
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

#ifndef ALARM_H_
#define ALARM_H_

#include "whisperlib/base/types.h"
#include "whisperlib/base/callback.h"
#include "whisperlib/net/selector.h"

namespace whisper {
namespace util {

// A class to run a closure at fixed intervals of time.
//
class Alarm {
public:
  Alarm(net::Selector & selector);
  virtual ~Alarm();

  int64 timeout() const { return timeout_; }
  int64 last_fire_ts() const { return last_fire_ts_; }
  int64 last_start_ts() const { return last_start_ts_; }
  int64 ms_from_last_start() const { return selector_.now() - last_start_ts_; }

  //  Set the given closure to be executed after timeout.
  //  You can set a single closure at a time. Previous closure will be Clear().
  //  The first time alarm will fire after timeout.
  // input:
  //  closure: to be run when alarm fires. Must be permanent (just for
  //           simplicity; Non-permanent closures could be implemented, but
  //           the Alarm would become much more complex; For now, just permanent
  //           closures appear to suffice).
  //  auto_delete: the closure is deleted on Clear() and an ~Alarm().
  //  timeout: milliseconds until alarm fires measured from start.
  //  repeat: repeat the alarm. It MUST be a permanent closure.
  //  start: start the alarm timer right away. Or you can do it later by Start().
  void Set(Closure * closure, bool auto_delete, int64 timeout, bool repeat,
           bool start);

  // Test if a closure is set.
  bool IsSet();
  // Test if a closure is set and the alarm is started.
  bool IsStarted();

  //  Change the timeout. It will fire timeout milliseconds from now.
  //  If the alarm is not repeating and already fired then it won't fire again.
  void ResetTimeout(int64 timeout);
  // remove and return existing closure.
  Closure * Release();
  // remove and delete existing closure.
  void Clear();

  // Stop the alarm. During stop the alarm won't fire.
  void Stop();
  // Start or restart the alarm. The alarm will fire after timeout_ ms from now.
  // If repeat == true, the alarm will repeat.
  // If repeat == false, the alarm will fire once then stop.
  void Start();

private:
  void Fire();

private:
  net::Selector & selector_;

  // permanent callback to Fire
  Closure * fire_callback_;

  Closure * alarm_;
  bool auto_delete_;

  // milliseconds
  int64 timeout_;
  bool repeat_;

  // timestamp of the last Start. Might be useful.
  int64 last_start_ts_;
  // timestamp of the last Fire. Might be useful.
  int64 last_fire_ts_;

  // BUG TRAP: Prevents Alarm deletion from Fire() call
  bool firing_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Alarm);
};

}  // namespace util
}  // namespace whisper

#endif /*ALARM_H_*/
