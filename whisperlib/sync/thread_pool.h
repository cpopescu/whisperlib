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

#ifndef __WHISPERLIB_SYNC_THREAD_POOL_H__
#define __WHISPERLIB_SYNC_THREAD_POOL_H__

#include <vector>
#include <whisperlib/sync/thread.h>
#include <whisperlib/sync/producer_consumer_queue.h>

namespace thread {

class ThreadPool {
 public:
  // Constructs a thread pool w/ pool_size threads and a queue of
  // backlog_size (condition: backlog_size > pool_size).
  ThreadPool(int pool_size, int backlog_size);
  // Stops all threads (as soon as they are idle).
  ~ThreadPool();

  // Use this accessors to add jobs to the pool (in a blocking on non
  // blocking way).
  // Example:
  //   thread_pool->jobs()->Put(NewCallback(this, &Worker::DoWork, work_data));
  synch::ProducerConsumerQueue<Closure*>* jobs() {
    return &jobs_;
  }

  // Finalizes the threads - possibly waiting for their work to be done.
  // The destructor would try to flush the jobs queue, use this first to
  // have the work finished
  void FinishWork();

 private:
  void ThreadRun();
  synch::ProducerConsumerQueue<Closure*> jobs_;
    vector<Thread*> threads_;

  DISALLOW_EVIL_CONSTRUCTORS(ThreadPool);
};
}

#endif  // __COMMON_SYNC_THREAD_POOL_H__
