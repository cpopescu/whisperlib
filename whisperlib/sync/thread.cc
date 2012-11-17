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
#include "whisperlib/sync/thread.h"

namespace {

void* InternalThreadRun(void* param) {
  Closure* closure = reinterpret_cast<Closure*>(param);
  closure->Run();
  pthread_exit(NULL);
  return NULL;
}
}

namespace thread {

Thread::Thread(Closure* thread_function)
  : thread_(0),
    thread_function_(thread_function) {
  pthread_attr_init(&attr_);
}

Thread::~Thread() {
  pthread_attr_destroy(&attr_);
}
bool Thread::Start() {
  CHECK(thread_function_ != NULL);
  return pthread_create(&thread_, &attr_,
                        InternalThreadRun, thread_function_) == 0;
}
bool Thread::Join() {
  void* status;
  return pthread_join(thread_, &status) == 0;
}
bool Thread::SetJoinable() {
  return pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_JOINABLE) == 0;
}
bool Thread::SetStackSize(size_t stacksize) {
#if defined(PTHREAD_STACK_MIN) && defined(PAGE_SIZE)
  // Apparently android does not have this
  CHECK_LE(PTHREAD_STACK_MIN, stacksize);
#endif
  return pthread_attr_setstacksize(&attr_, stacksize) == 0;
}
bool Thread::Kill() {
  return 0 == pthread_kill(thread_, SIGKILL);
}
bool Thread::IsInThread() const {
  return thread_ == ::pthread_self();
}
}
