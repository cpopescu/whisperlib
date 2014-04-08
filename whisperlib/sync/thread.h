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

#ifndef __WHISPERLIB_SYNC_THREAD_H__
#define __WHISPERLIB_SYNC_THREAD_H__

#include <pthread.h>
#include <limits.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/callback.h>
#include <whisperlib/base/types.h>

namespace thread {

class Thread {
 public:
  // Create a thread that will run the given function
  explicit Thread(Closure* thread_function,
                  bool low_priority = false);
  ~Thread();

  // Actually spawns the thread
  bool Start();
  // Waits for the thread to finish
  bool Join();
  // Enables waiting on this thread (joining)
  bool SetJoinable();
  // Set the stack size for this thread.
  // Min value is: PTHREAD_STACK_MIN (=16384 in /usr/include/bits/local_lim.h)
  // Max value is: system-imposed limit
  // Default value is system dependent:
  //
  //  Architecture #CPUs RAM(GB) DefaultSize (bytes)
  //  ------------------------------------------------
  //  AMD Opteron  8     16      2,097,152
  //  Intel IA64   4     8       33,554,432
  //  Intel IA32   2     4       2,097,152
  //  IBM Power5   8     32      196,608
  //  IBM Power4   8     16      196,608
  //  IBM Power3  16     16      98,304
  //
  bool SetStackSize(size_t stacksize);
  // Stops the thread unconditionally
  bool Kill();

  void set_thread_function(Closure* closure) {
    thread_function_ = closure;
  }

  // Test if the caller is in this thread context
  bool IsInThread() const;

 protected:
  pthread_t thread_;
  pthread_attr_t attr_;
  Closure* thread_function_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Thread);
};
}

#endif  // __COMMON_SYNC_THREAD_H__
