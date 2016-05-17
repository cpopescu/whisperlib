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

#include "whisperlib/net/selector.h"

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_EVENTFD_H
#include <sys/eventfd.h>
#endif

#include "whisperlib/base/log.h"
#include "whisperlib/base/core_errno.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/net/selectable.h"

using namespace std;

//////////////////////////////////////////////////////////////////////

DEFINE_bool(selector_high_alarm_precission,
            false,
            "Loose some CPU time and gain that extra milisecond precission "
            "for selector alarms..");
DEFINE_int32(selector_num_closures_per_event,
             64,
             "We don't run more than these many closures per event");
DEFINE_int32(selector_events_to_read_per_poll,
             64,
             "We process at most these many events per poll step");
DEFINE_int32(debug_check_long_callbacks_ms,
             500,
             "If greater than zero, we check (in debug mode only !) "
             "that processing functions, callbacks and alarm functions take "
             "less then this amount of time, in miliseconds");

//////////////////////////////////////////////////////////////////////

namespace {
void StopSignal() {
}
whisper::Closure* glb_stop_signal = whisper::NewPermanentCallback(&StopSignal);
#ifdef __USE_LEAN_SELECTOR__
static const int kCallbackQueueSize = 10000;
#endif
}

namespace whisper {
namespace net {

Selector::Selector()
  : tid_(0),
    should_end_(false),
#ifdef __USE_LEAN_SELECTOR__
    to_run_(kCallbackQueueSize),
#else
    base_(NULL),
#endif //   __USE_LEAN_SELECTOR__
    now_(timer::TicksMsec()),
    call_on_close_(NULL) {
#ifndef  __USE_LEAN_SELECTOR__
#ifdef HAVE_EVENTFD_H
  event_fd_ = eventfd(0, 0);
  CHECK(event_fd_ >= 0)
      << " eventfd fail: " << GetLastSystemErrorDescription();
  const int flags = fcntl(event_fd_, F_GETFL, 0);
  CHECK(fcntl(event_fd_, F_SETFL, flags | O_NONBLOCK) >= 0)
      << " fcntl fail: " << GetLastSystemErrorDescription();
#else
  CHECK(!pipe(signal_pipe_)) <<  "pipe() failed:  "
                             << GetLastSystemErrorDescription();
  for ( int i = 0; i < NUMBEROF(signal_pipe_); ++i ) {
    const int flags = fcntl(signal_pipe_[i], F_GETFL, 0);
    CHECK(flags >= 0) << " fcntl fail: " << GetLastSystemErrorDescription();
    CHECK(fcntl(signal_pipe_[i], F_SETFL, flags | O_NONBLOCK) >= 0)
         << " fcntl fail: " << GetLastSystemErrorDescription();
  }
  event_fd_ = signal_pipe_[0];
#endif
  base_ = new SelectorBase(event_fd_,
                           FLAGS_selector_events_to_read_per_poll);
#endif   //  __USE_LEAN_SELECTOR__
}

Selector::~Selector() {
  CHECK(tid_ == 0);
  CHECK(registered_.empty());
#ifndef __USE_LEAN_SELECTOR__
#ifdef HAVE_EVENTFD_H
  close(event_fd_);
#else
  close(signal_pipe_[0]);
  close(signal_pipe_[1]);
#endif
#endif   // __USE_LEAN_SELECTOR__
}

void Selector::Loop() {
  CHECK(tid_ ==  0) << "Loop already started -- bad !";
  should_end_ = false;
  tid_ = pthread_self();
  LOG_INFO << "Starting selector loop";

  vector<SelectorEventData> events;
  while ( !should_end_ ) {
    int32 to_sleep_ms = kStandardWakeUpTimeMs;
    if ( FLAGS_selector_high_alarm_precission ) {
      now_ = timer::TicksMsec();
    }
    if ( !alarms_.empty() ) {
      if ( alarms_.begin()->first < now_ + to_sleep_ms ) {
        to_sleep_ms = alarms_.begin()->first - now_;
      }
    }

    int run_count = 0;
#ifdef __USE_LEAN_SELECTOR__
    Closure* next_to_run = to_run_.Get(to_sleep_ms);
    if (glb_stop_signal == next_to_run) {
        should_end_ = true;
    } else if (next_to_run) {
        now_ = timer::TicksMsec();
        next_to_run->Run();
        ++run_count;
    }
#else          // __USE_LEAN_SELECTOR__
    mutex_.Lock();
    if ( !to_run_.empty() ) {
      to_sleep_ms = 0;
    }
    mutex_.Unlock();
    events.clear();
    if (!base_->LoopStep(to_sleep_ms, &events)) {
      LOG_ERROR << "ERROR in select loop step. Exiting Loop.";
      break;
    }
    now_ = timer::TicksMsec();
#ifdef _DEBUG
    const int64 processing_begin =
        FLAGS_debug_check_long_callbacks_ms > 0 ? timer::TicksMsec() : 0;
#endif
    for ( int i = 0; i < events.size(); ++i ) {
      const SelectorEventData& event = events[i];
      Selectable* const s = reinterpret_cast<Selectable *>(event.data_);
      if ( s == NULL ) {
        // was probably a wake signal..
        continue;
      }
      if ( s->selector() == NULL ) {
        // already unregistered
        continue;
      }
      const int32 desire = event.desires_;
      // During HandleXEvent the obj may be closed loosing so track of
      // it's fd value.
      bool keep_processing = true;
      if ( desire & kWantError ) {
        keep_processing = s->HandleErrorEvent(event) &&
                          s->GetFd() != INVALID_FD_VALUE;
      }
      if ( keep_processing && (desire & kWantRead) ) {
        keep_processing = s->HandleReadEvent(event) &&
                          s->GetFd() != INVALID_FD_VALUE;
      }
      if ( keep_processing && (desire & kWantWrite) ) {
        s->HandleWriteEvent(event);
      }
    }  // else, was probably a timeout
#ifdef _DEBUG
    if ( FLAGS_debug_check_long_callbacks_ms > 0 ) {
      const int64 processing_end = timer::TicksMsec();
      if ( processing_end - processing_begin >
           FLAGS_debug_check_long_callbacks_ms ) {
        LOG_WARN << this << " ====>> Unexpectedly long event processing: "
                  << " time spent:  "
                  << processing_end - processing_begin
                  << " num events: " << events.size();
      }
    }
#endif

    // Runs some closures..
    if ( FLAGS_selector_high_alarm_precission ) {
      now_ = timer::TicksMsec();
    }
    while ( run_count < FLAGS_selector_num_closures_per_event ) {
      int n = RunClosures(FLAGS_selector_num_closures_per_event);
      if ( n == 0 ) {
        break;
      }
      run_count += n;
    }
#endif     //  __USE_LEAN_SELECTOR__

    // Run the alarms
    if ( FLAGS_selector_high_alarm_precission ) {
      now_ = timer::TicksMsec();
    }
    while ( !alarms_.empty() ) {
      if ( alarms_.begin()->first > now_ ) {
        // this alarm and all that follow are in the future
        break;
      }
      Closure* closure = alarms_.begin()->second;
      const bool erased = reverse_alarms_.erase(closure);
      CHECK(erased);
      run_count++;
      alarms_.erase(alarms_.begin());
#ifdef _DEBUG
      const int64 processing_begin =
          FLAGS_debug_check_long_callbacks_ms > 0 ? timer::TicksMsec() : 0;
#endif
      closure->Run();
#ifdef _DEBUG
      if ( FLAGS_debug_check_long_callbacks_ms > 0 ) {
        const int64 processing_end = timer::TicksMsec();
        if ( processing_end - processing_begin >
             FLAGS_debug_check_long_callbacks_ms ) {
          LOG_WARN << this << " ====>> Unexpectedly long alarm processing: "
                    << " callback: " << closure
                    << " time spent:  "
                    << processing_end - processing_begin;
        }
      }
#endif
    }
    if ( run_count > 2 * FLAGS_selector_num_closures_per_event ) {
      LOG_WARN << this << " We run to many closures per event: " << run_count;
    }
  }

  LOG_INFO << "Closing all the active connections in the selector...";
  CleanAndCloseAll();
  CHECK(registered_.empty());

  // Run all the remaining closures...
  now_ = timer::TicksMsec();
  int run_count = 0;
  while ( true ) {
    int n = RunClosures(FLAGS_selector_num_closures_per_event);
    if ( n == 0 ) {
      break;
    }
    run_count += n;
    LOG_INFO << "Running closures on shutdown, count: " << run_count;
  }
#ifndef __USE_LEAN_SELECTOR__
  mutex_.Lock();
  CHECK(to_run_.empty());
  mutex_.Unlock();
#endif

  // Run remaining alarms
  while ( !alarms_.empty() ) {
    now_ = timer::TicksMsec();
    if ( alarms_.begin()->first > now_ ) {
      // this alarm and all that follow are in the future
      break;
    }
    Closure* closure = alarms_.begin()->second;
    const bool erased = reverse_alarms_.erase(closure);
    CHECK(erased);
    alarms_.erase(alarms_.begin());
  }
  if ( !alarms_.empty() ) {
    LOG_WARN << "Now: " << now_ << " ms";
  }
  for ( AlarmSet::const_iterator it = alarms_.begin(); it != alarms_.end();
        ++it ) {
    LOG_WARN << "Leaking alarm, run at: " << it->first
              << " ms, due in: " << (it->first - now_) << " ms";
  }

#ifndef __USE_LEAN_SELECTOR__
  delete base_;
  base_ = NULL;
#endif

  if ( call_on_close_ != NULL ) {
    call_on_close_->Run();
    call_on_close_ = NULL;
  }
  tid_ = 0;
}

void Selector::MakeLoopExit() {
  CHECK(IsInSelectThread());
#ifdef __USE_LEAN_SELECTOR__
  to_run_.Put(glb_stop_signal);
#else
  should_end_ = true;
#endif
}

void Selector::RunInSelectLoop(Closure* callback) {
  DCHECK(tid_ != 0 || !should_end_) << "Selector already stopped";
  CHECK_NOT_NULL(callback);
  #ifdef _DEBUG
  callback->set_selector_registered(true);
  #endif

#ifdef __USE_LEAN_SELECTOR__
  to_run_.Put(callback);
#else
  mutex_.Lock();
  to_run_.push_back(callback);
  mutex_.Unlock();
  if ( !IsInSelectThread() ) {
    SendWakeSignal();
  }
#endif
}
void Selector::RegisterAlarm(Closure* callback, int64 timeout_in_ms) {
  DCHECK(tid_ != 0 || !should_end_) << "Selector already stopped";
  CHECK(IsInSelectThread() || (tid_ == 0 && !should_end_));
  CHECK_NOT_NULL(callback);
  const int64 wake_up_time = now_ + timeout_in_ms;
  CHECK(timeout_in_ms < 0 || timeout_in_ms <= wake_up_time)
    << "Overflow, timeout_in_ms: " << timeout_in_ms << " is too big";

  std::pair<ReverseAlarmsMap::iterator, bool> result =
      reverse_alarms_.insert(make_pair(callback, wake_up_time));
  if ( result.second ) {
    // New alarm inserted; Now add to alarms_ too.
    alarms_.insert(make_pair(wake_up_time, callback));
  } else {
    // Old alarm needs replacement.
    alarms_.erase(make_pair(result.first->second, callback));
    alarms_.insert(make_pair(wake_up_time, callback));
    result.first->second = wake_up_time;
  }
  // We do not need to wake .. we are in the select loop :)
}
void Selector::UnregisterAlarm(Closure* callback) {
  CHECK(IsInSelectThread());
  ReverseAlarmsMap::iterator it = reverse_alarms_.find(callback);
  if ( it != reverse_alarms_.end() ) {
    int count = alarms_.erase(make_pair(it->second, callback));
    CHECK_EQ(count,1);
    reverse_alarms_.erase(it);
  }
}

//////////////////////////////////////////////////////////////////////

void Selector::CleanAndCloseAll() {
  // It is some discussion, whether to do some CHECK if connections are
  // left at this point or to close them or to just skip it. The ideea is
  // that we preffer forcing a close on them for the server case and also
  // client connections when we end a program.
  if ( !registered_.empty() ) {
    DLOG_INFO << "Select loop ended with " << registered_.size()
              << " registered connections.";
    // We just close the fd and care about nothing ..
    while ( !registered_.empty() ) {
      (*registered_.begin())->Close();
    }
  }
}

int Selector::RunClosures(int /*num_closures*/) {
#ifdef __USE_LEAN_SELECTOR__
    int run_count = 0;
    while (run_count < num_closures) {
        Closure* next_to_run = to_run_.Get(0);
        if (glb_stop_signal != next_to_run && next_to_run) {
#ifdef _DEBUG
            closure->set_selector_registered(false);
#endif
            ++run_count;
        } else {
            return run_count;
        }
    }
    return run_count;


#else  // __USE_LEAN_SELECTOR__
#ifdef HAVE_EVENTFD_H
  char buffer[1024];
#else
  char buffer[32];
#endif

  int cb = 0;
  while ( (cb = ::read(event_fd_, buffer, sizeof(buffer))) > 0 ) {
    VLOG(10) << " Cleaned some " << cb << " bytes from signal pipe.";
  }
  int run_count = 0;
  while ( run_count < FLAGS_selector_num_closures_per_event ) {
    mutex_.Lock();
    if ( to_run_.empty() ) {
      mutex_.Unlock();
      break;
    }
    Closure* closure = to_run_.front();
    to_run_.pop_front();
    mutex_.Unlock();

#ifdef _DEBUG
    const int64 processing_begin = FLAGS_debug_check_long_callbacks_ms > 0 ?
        timer::TicksMsec() : 0;
    closure->set_selector_registered(false);
#endif
    closure->Run();
    run_count++;
#ifdef _DEBUG
    if ( FLAGS_debug_check_long_callbacks_ms > 0 ) {
      const int64 processing_end = timer::TicksMsec();
      if ( processing_end - processing_begin >
           FLAGS_debug_check_long_callbacks_ms ) {
        LOG_WARN << this << " ====>> Unexpectedly long callback processing: "
                  << " callback: " << closure
                  << " time spent:  "
                  << processing_end - processing_begin;
      }
    }
#endif
  }
#endif  // __USE_LEAN_SELECTOR__
  return run_count;
}

void Selector::SendWakeSignal() {
#ifndef  __USE_LEAN_SELECTOR__
#ifdef HAVE_EVENTFD_H
  uint64 value = 1ULL;
  if ( sizeof(value) != ::write(event_fd_, &value, sizeof(value)) ) {
    LOG_WARN << "Error writing a wake-up byte in selector event_fd_";
  }
#else
  int8 byte = 0;
  if ( sizeof(byte) != ::write(signal_pipe_[1], &byte, sizeof(byte)) ) {
    LOG_WARN << "Error writing a wake-up byte in selector signal pipe";
  }
#endif
#endif  // __USE_LEAN_SELECTOR__
}

//////////////////////////////////////////////////////////////////////

bool Selector::Register(Selectable* s) {
#ifdef __USE_LEAN_SELECTOR__
  CHECK(false) << " Cannot use selectables in this mode";
  return false;
#else
  DCHECK(IsInSelectThread() || tid_ == 0);
  CHECK(s->selector() == this);
  const int fd = s->GetFd();

  SelectableSet::const_iterator it = registered_.find(s);
  if ( it != registered_.end() ) {
    // selectable obj already registered
    CHECK_EQ((*it)->GetFd(), fd);
    return true;
  }
  // Insert in the local set of registered objs
  registered_.insert(s);

  return base_->Add(fd, s, s->desire_);
#endif   //  __USE_LEAN_SELECTOR__
}

void Selector::Unregister(Selectable* s) {
#ifdef __USE_LEAN_SELECTOR__
  CHECK(false) << " Cannot use selectables in this mode";
#else
  DCHECK(IsInSelectThread() || tid_ == 0);
  CHECK(s->selector() == this);
  const int fd = s->GetFd();

  const SelectableSet::iterator it = registered_.find(s);
  DCHECK(it != registered_.end()) << " Cannot unregister : " << fd;

  base_->Delete(fd);
  registered_.erase(it);
  s->set_selector(NULL);
#endif   //  __USE_LEAN_SELECTOR__
}

//////////////////////////////////////////////////////////////////////

void Selector::UpdateDesire(Selectable* s, bool enable, int32 desire) {
  // DCHECK(registered_.find(s) != registered_.end()) << fd;
  if ( ((s->desire_ & desire) && enable) ||
       ((~s->desire_ & desire) && !enable) ) {
    return;  // already set ..
  }
  if ( enable ) {
    s->desire_ |= desire;
  } else {
    s->desire_ &= ~desire;
  }
#ifndef __USE_LEAN_SELECTOR__
  base_->Update(s->GetFd(), s, s->desire_);
#endif
}

//////////////////////////////////////////////////////////////////////

}  // namespace net

}  // namespace whisper
