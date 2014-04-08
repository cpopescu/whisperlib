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

#ifndef __NET_BASE_SELECTOR_H__
#define __NET_BASE_SELECTOR_H__

#include <sys/types.h>
#include <list>
#include <deque>
#include <set>
#include <map>

#include <whisperlib/base/types.h>
#include WHISPER_HASH_MAP_HEADER

#include <whisperlib/base/callback.h>
#include <whisperlib/sync/mutex.h>
#include <whisperlib/sync/thread.h>

#ifdef __USE_LEAN_SELECTOR__
#include <whisperlib/sync/producer_consumer_queue.h>
#else
#include <whisperlib/net/selector_base.h>
#endif

// Just a helper function

namespace net {

class Selectable;

// //////////////////
//
// NOTE:  IT IS THREAD-UNSAFE - except where noted
//
//
class Selector {
 public:
  Selector();
  ~Selector();

  // Register an I/O object for read/write/error event callbacks. By default:
  //  - read callback is enabled.
  //  - write callback is disabled.
  //  - error callback is enabled.
  // returns:
  //   success status.
  bool Register(Selectable* s);

  // Unregister a previously registered I/O object.
  //  s - the selectable object to be unregistered
  void Unregister(Selectable* s);

  // Enable/disable a certain event callback for the given selectable
  // -- Call this only from the select loop
  void EnableWriteCallback(Selectable* s, bool enable) {
    UpdateDesire(s, enable, kWantWrite);
  }
  void EnableReadCallback(Selectable* s, bool enable) {
    UpdateDesire(s, enable, kWantRead);
  }
  void set_call_on_close(Closure* call_on_close) {
    call_on_close_ = call_on_close;
  }

  // Runs the main select loop
  void Loop();

  // Makes an exit from the select loop
  void MakeLoopExit();
  // Returns true if the selector is no longer in the loop (though
  // registered callbacks can still execute !!)
  bool IsExiting() const {
    return should_end_;
  }

  // Returns true if this call was made from the select server thread
  bool IsInSelectThread() const {
    return tid_ == pthread_self();
  }

  //
  // Runs this closure in the select loop
  // - THIS IS SAFE TO CALL FROM ANOTHER THREAD -
  // NOTE: It is legal to add closures while the selector is shutting down..
  //       (i.e. IsExiting() == true)
  //
 private:
  template <typename T> static void GeneralAsynchronousDelete(T* ob) {
    delete ob;
  }
 public:
  void RunInSelectLoop(Closure* callback);
  template <typename T> void DeleteInSelectLoop(T* ob) {
    RunInSelectLoop(
        ::NewCallback(&Selector::GeneralAsynchronousDelete<T>, ob));
  }

  // Functions for running in the select loop the given Closure after
  // a specified time interval

  // -- THESE ARE NOT THREAD SAFE --

  // Registers or Re-Registers the given callback to be run after timeout_in_ms.
  void RegisterAlarm(Closure* callback, int64 timeout_in_ms);
  // Cancels a previously registered alarm.
  void UnregisterAlarm(Closure* callback);

  // The current moment when the select loop was broken:
  int64 now() const { return now_; }

  // Desires of selectables
  static const int32 kWantRead  = 1;
  static const int32 kWantWrite = 2;
  static const int32 kWantError = 4;

 private:
  // helper that turns on/off fd desires in the assoiciated RegistrationData
  void UpdateDesire(Selectable* s, bool enable, int32 desire);

 public:
  // Cleans and closes the entire list of selectable objects
  void CleanAndCloseAll();

 private:
  // This runs all the functions from to_run_ (if any)
  int RunClosures(int max_num_closures);

  // This writes a byte in the internal pipe in order to make the
  // select loop wake up
  void SendWakeSignal();

 private:
  // selector's internal thread id
  pthread_t tid_;

  // MakeLoopExits marks this flag. The Loop() thread should end.
  bool should_end_;

  typedef set<Selectable*> SelectableSet;
  typedef list<Selectable*> SelectableList;
  // typedef multimap<int64, Closure*> AlarmsMap;
  typedef set< pair<int64, Closure*> > AlarmSet;
  // Map from alarm to wake up time
  typedef hash_map<Closure*, int64> ReverseAlarmsMap;

  // the set of registered I/O objects
  SelectableSet registered_;
  // Alarms..
  AlarmSet alarms_;
  // The same alarms, mapped by closure; allows us to cancel an alarm
  ReverseAlarmsMap reverse_alarms_;

  // guards access to closure queue: to_run_
  synch::Mutex mutex_;

  // Internal control:

#ifdef __USE_LEAN_SELECTOR__
  synch::ProducerConsumerQueue<Closure*> to_run_;

#else
  // these file descriptors are for waking the selector when a function
  // needs to be executed in the select loop
  int event_fd_;
#ifndef __USE_EVENTFD__
  int signal_pipe_[2];     // when not using eventfd, we use sigan_pipe_[0] as
                           //  event_fd_
#endif

    // functions registered to be run in the select loop
  deque<Closure*> to_run_;

    // OS specific selector base implementation
  SelectorBase* base_;

#endif   // __USE_LEAN_SELECTOR__

  // Cache for timer::TicksMsec(); Instead of calling TicksMsec() you can easily
  // take the value of selector_->now()
  int64 now_;

  // we wake up in loop every 100 ms by default
#ifdef __USE_LEAN_SELECTOR__
  static const int32 kStandardWakeUpTimeMs = 1000;
#else
  static const int32 kStandardWakeUpTimeMs = 100;
#endif


  // called when we end our loop
  Closure* call_on_close_;

  DISALLOW_EVIL_CONSTRUCTORS(Selector);
};

class SelectorThread {
 public:
  SelectorThread()
      : thread_(NewCallback(this, &SelectorThread::Execution)) {
  }
  ~SelectorThread() {
      Stop();
  }
  void Start() {
    CHECK(thread_.SetJoinable());
    CHECK(thread_.Start());
  }
  void Stop() {
      selector_.RunInSelectLoop(NewCallback(&selector_, &net::Selector::MakeLoopExit));
      thread_.Join();
  }
  void CleanAndCloseAll() {
    selector_.RunInSelectLoop(NewCallback(&selector_,
                                          &net::Selector::CleanAndCloseAll));
  }

  const Selector* selector() const {
    return &selector_;
  }
  Selector* mutable_selector() {
    return &selector_;
  }

 private:
  void Execution() {
    selector_.Loop();
  }
  thread::Thread thread_;
  Selector selector_;

  DISALLOW_EVIL_CONSTRUCTORS(SelectorThread);
};
}
#endif  // __NET_BASE_SELECTOR__
