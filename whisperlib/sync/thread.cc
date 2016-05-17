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

#include <signal.h>
#include <sched.h>
#include "whisperlib/sync/thread.h"

namespace whisper {
namespace thread {

Thread::Thread(Closure* thread_function, bool low_priority)
  : thread_(0),
    thread_function_(thread_function), self_delete_(false), joinable_(false) {
  pthread_attr_init(&attr_);
#if !defined(NACL) && !defined(EMSCRIPTEN)
#ifdef HAVE_SCHED_GET_PRIORITY_MIN
  if (low_priority) {
      struct sched_param param;
      param.sched_priority = sched_get_priority_min(SCHED_RR);
      const int status = pthread_attr_setschedparam(&attr_, &param);
      VLOG(1) << " Thread priority set to: " <<  param.sched_priority
              << " with status: " << status;
  }
#endif
#endif
}

Thread::~Thread() {
  pthread_attr_destroy(&attr_);
}
bool Thread::Start() {
  CHECK(thread_function_ != NULL);
  if (!IsJoinable() && !self_delete_) {
      LOG_ERROR << " --> Non joinable thread w/o self delete.";
  }
  const int error = pthread_create(&thread_, &attr_, Thread::InternalRun, this);
  if (error != 0) {
      LOG_ERROR << " Thread starting failed with error: " << error;
  }
  return error == 0;
}
bool Thread::Join() {
  void* status = NULL;
  return pthread_join(thread_, &status) == 0;
}
bool Thread::SetJoinable(bool joinable) {
  joinable_ = joinable;
  return pthread_attr_setdetachstate(&attr_,
          joinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED) == 0;
}
bool Thread::IsJoinable() const {
#if defined(HAVE_PTHREAD_ATTR_GETDETACHSTATE) && !defined(EMSCRIPTEN)
    int state = 0;
    const int error = pthread_attr_getdetachstate(&attr_, &state);
    if (error != 0) {
        LOG_ERROR << "pthread_attr_getdetachstate() failed, error: " << error;
        return false;
    }
    return state == PTHREAD_CREATE_JOINABLE;
#else
    return joinable_;
#endif
}
bool Thread::SetStackSize(size_t stacksize) {
#ifdef NACL
  return true;
#else
#if defined(PTHREAD_STACK_MIN) && defined(PAGE_SIZE)
  // Apparently android does not have this
  CHECK_LE(PTHREAD_STACK_MIN, stacksize);
#endif
  return pthread_attr_setstacksize(&attr_, stacksize) == 0;
#endif  // NACL
}
bool Thread::Kill() {
#ifdef NACL
  return false;
#else
  return 0 == pthread_kill(thread_, SIGKILL);
#endif
}
bool Thread::IsInThread() const {
  return thread_ == ::pthread_self();
}
void* Thread::InternalRun(void* param) {
  Thread* th = reinterpret_cast<Thread*>(param);
  th->thread_function_->Run();
  if (!th->IsJoinable() && th->self_delete()) {
    LOG_INFO << " Deleting non joinable self delete thread: " << th;
    delete th;
  }
  pthread_exit(NULL);
  return NULL;
}

}  // namespace thread
}  // namespace whisper
