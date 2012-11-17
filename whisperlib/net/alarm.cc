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

#include <whisperlib/net/alarm.h>

// Disabled - most of the time.. :)
#define LOG_ALARM if(true); else LOG_WARNING

namespace util {

Alarm::Alarm(net::Selector & selector)
  : selector_(selector),
    fire_callback_(NewPermanentCallback(this, &Alarm::Fire)),
    alarm_(NULL),
    auto_delete_(false),
    timeout_(0),
    repeat_(false),
    last_start_ts_(-1),
    last_fire_ts_(-1),
    firing_(false) {
}
Alarm::~Alarm() {
  CHECK(!firing_) << "Do NOT delete the alarm from notification callback!";
  Clear();
  delete fire_callback_;
  fire_callback_ = NULL;
}

void Alarm::Set(Closure * closure, bool auto_delete, int64 timeout,
    bool repeat, bool start) {
  CHECK(closure->is_permanent()) << " Alarm uses only permanent closures";
  Clear();
  alarm_ = closure;
  auto_delete_ = auto_delete;
  timeout_ = timeout;
  repeat_ = repeat;
  if ( start ) {
    Start();
  }
}
bool Alarm::IsSet() {
  return alarm_ != NULL;
}
bool Alarm::IsStarted() {
  return IsSet() && last_start_ts_ >= 0;
}

void Alarm::ResetTimeout(int64 timeout) {
  CHECK(IsSet());
  timeout_ = timeout;
  Start();
}
Closure * Alarm::Release() {
  Stop();

  Closure* const alarm = alarm_;
  alarm_ = NULL;
  return alarm;
}
void Alarm::Clear() {
  LOG_ALARM << "Clearing Alarm: " << alarm_;
  Stop();
  if ( auto_delete_ && alarm_ != NULL ) {
    LOG_ALARM << "Auto deleting alarm: " << alarm_;
    delete alarm_;
  }
  alarm_ = NULL;
  auto_delete_ = false;
  LOG_ALARM << "Cleared";
}

void Alarm::Stop() {
  if ( !IsStarted() ) {
    // nothing to stop
    return;
  }
  selector_.RunInSelectLoop(
      NewCallback(&selector_, &net::Selector::UnregisterAlarm, fire_callback_));
  last_start_ts_ = -1;
  LOG_ALARM << "Stop Alarm: " << alarm_;
}
void Alarm::Start() {
  LOG_ALARM << "Start Alarm: " << alarm_;
  CHECK(IsSet());
  selector_.RunInSelectLoop(NewCallback(&selector_, &net::Selector::RegisterAlarm,
                                        fire_callback_, timeout_));
  last_start_ts_ = selector_.now();
}

void Alarm::Fire() {
  LOG_ALARM << "Firing alarm: " << alarm_;
  last_fire_ts_ = selector_.now();
  last_start_ts_ = -1;
  Closure * alarm = alarm_;

  firing_ = true;
  alarm->Run();
  firing_ = false;

  LOG_ALARM << "Fired alarm: " << alarm_;
  if ( alarm_ == NULL || alarm_ != alarm ) {
    return; // either: alarm Cleared or Set in Run()
  }
  if ( repeat_ ) {
    LOG_ALARM << "Repeat alarm: " << alarm_;
    Start(); // start again
  }
}

} // namespace util
