// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
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
#include "whisperlib/sync/thread.h"
#include "whisperlib/sync/producer_consumer_queue.h"
#include "whisperlib/sync/lock_free_producer_consumer_queue.h"

namespace whisper {
namespace thread {

class ThreadPoolBase {
public:
  ThreadPoolBase(size_t num_threads, bool low_priority,
                 Closure* completion_callback);
  virtual ~ThreadPoolBase();

  size_t size() const {
    return num_threads_;
  }
  size_t thread_count() const {
    return num_threads_;
  }
  bool IsCompleted() const {
    synch::MutexLocker l(&completion_mutex_);
    return num_completed_ >= threads_.size();
  }

  void WaitForFinish();

  // Start all internal threads. They may already be started.
  void Start();

  // Hard kiss the threads.
  void KillAll();

protected:

  void Run(size_t thread_index);

  // Waits for all threads to complete and joins them (need to hold wait_mutex_)
  void WaitForFinishLocked();

  virtual void ThreadRun(size_t thread_index) = 0;

protected:
  mutable synch::Mutex wait_mutex_;

private:
  const size_t num_threads_;
  vector<Thread*> threads_;
  bool started_;
  mutable synch::Mutex completion_mutex_;
  size_t num_completed_;
  Closure* completion_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(ThreadPoolBase);
};

//////////////////////////////////////////////////////////////////////

class ThreadPool : public ThreadPoolBase {
public:
  // Constructs a thread pool w/ num_threads threads and a queue of
  // backlog_size (condition: backlog_size > num_threads).
  ThreadPool(size_t num_threads, size_t backlog_size, bool low_priority=false,
             Closure* completion_callback=NULL, bool auto_start=true);
  // Stops all threads (as soon as they are idle).
  ~ThreadPool();

  // Use this accessors to add jobs to the pool (in a blocking on non
  // blocking way).
  // Example:
  //   thread_pool->jobs()->Put(NewCallback(this, &Worker::DoWork, work_data));
  synch::ProducerConsumerQueue<Closure*>* jobs() {
    return &jobs_;
  }

  // jobs: list of closures to run. It does not have to stay alive until
  //       completion. NULL jobs are acceptable and safely ignored.
  // completion: [optional] If NULL, this call blocks until all jobs completed.
  //             If non-NULL, you will be notified on the supplied callback
  //             when all jobs completed.
  void RunBatch(const vector<Closure*>& jobs, Closure* completion);

  // Terminates the threads waiting for all of their work to be done! The ThreadPool becomes
  // useless afterwards.
  // The ThreadPool destructor however: first deletes all pending jobs, then terminates the threads.
  void FinishWork();

protected:
  virtual void ThreadRun(size_t thread_index);

private:
  synch::ProducerConsumerQueue<Closure*> jobs_;

  // the count of jobs processed by each thread
  vector<size_t> count_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(ThreadPool);
};

//////////////////////////////////////////////////////////////////////

class LockFreeThreadPool : public ThreadPoolBase {
public:
  LockFreeThreadPool(size_t num_producers, size_t num_threads,
                     size_t backlog_size, size_t sleep_usec,
                     bool low_priority=false,
                     Closure* completion_callback=NULL,
                     bool auto_start=true);
  ~LockFreeThreadPool();

  // Use this accessor to add jobs to the pool.
  synch::LockFreeProducerConsumerQueue< Callback1<size_t> >* jobs() {
    return &jobs_;
  }

  // Finalizes the threads - possibly waiting for their work to be done.
  // The destructor would try to flush the jobs queue, use this first to
  // have the work finished
  void FinishWork();

protected:
  virtual void ThreadRun(size_t thread_index);

private:
  /** Number of producers for our jobs, we are an extra one */
  const size_t num_producers_;
  /** Number of consumers for our jobs, we are an extra one */
  const size_t num_consumers_;
  /** Sleep interval for underlying lock free producer consumer queue */
  const size_t sleep_usec_;

  synch::LockFreeProducerConsumerQueue< Callback1<size_t> > jobs_;

  DISALLOW_EVIL_CONSTRUCTORS(LockFreeThreadPool);
};


//////////////////////////////////////////////////////////////////////

/** A thread pool with independent job queues - to insure that jobs reach
 * consistent threads. The threads are created unstarted
 */
class LockFreeMultiQueueThreadPool : public ThreadPoolBase {
public:
  LockFreeMultiQueueThreadPool(size_t num_producers, size_t num_threads,
                               size_t backlog_size, size_t sleep_usec,
                               bool low_priority=false,
                               Closure* completion_callback=NULL,
                               bool auto_start=true);
  ~LockFreeMultiQueueThreadPool();

  // Use this accessor to add jobs to the pool.
  synch::LockFreeProducerConsumerQueue< Callback1<size_t> >* jobs(size_t ndx) {
    return jobs_[ndx];
  }

  // Finalizes the threads - possibly waiting for their work to be done.
  // The destructor would try to flush the jobs queue, use this first to
  // have the work finished
  void FinishWork();

protected:
  virtual void ThreadRun(size_t thread_index);

private:
  /** Number of producers for our jobs, we are an extra one */
  const size_t num_producers_;
  /** Number of consumers for our jobs, we are an extra one */
  const size_t num_consumers_;
  /** Sleep interval for underlying lock free producer consumer queue */
  const size_t sleep_usec_;

  vector< synch::LockFreeProducerConsumerQueue< Callback1<size_t> >*> jobs_;

  DISALLOW_EVIL_CONSTRUCTORS(LockFreeMultiQueueThreadPool);
};

////////////////////////////////////////////////////////////////////////////////


/** This is a thread pool that matches the interface of the lockfree counterpart,
 * and the jobs would receive the thread id in which they run.
 */
class LockedThreadPool : public ThreadPoolBase {
public:
  LockedThreadPool(size_t num_producers, size_t num_threads,
                   size_t backlog_size, size_t sleep_usec,
                   bool low_priority=false,
                   Closure* completion_callback=NULL,
                   bool auto_start=true);
  ~LockedThreadPool();

  synch::ProducerConsumerQueue< Callback1<size_t>* >* jobs() {
    return &jobs_;
  }

  void FinishWork();

protected:
  virtual void ThreadRun(size_t thread_index);

private:
  synch::ProducerConsumerQueue< Callback1<size_t>* > jobs_;
  vector<size_t> count_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(LockedThreadPool);
};

/** A thread pool with independent job queues - to insure that jobs reach
 * consistent threads. The threads are created unstarted.
 */
class MultiQueueThreadPool : public ThreadPoolBase {
public:
  MultiQueueThreadPool(size_t num_threads, size_t backlog_size,
                       bool low_priority=false,
                       Closure* completion_callback=NULL,
                       bool auto_start=true);
  ~MultiQueueThreadPool();

  // Use this accessor to add jobs to the pool.
  synch::ProducerConsumerQueue<Closure*>* jobs(size_t ndx) {
    return jobs_[ndx];
  }

  // Finalizes the threads - possibly waiting for their work to be done.
  // The destructor would try to flush the jobs queue, use this first to
  // have the work finished
  void FinishWork();

protected:
  virtual void ThreadRun(size_t thread_index);

private:

  vector< synch::ProducerConsumerQueue<Closure*>* > jobs_;

  DISALLOW_EVIL_CONSTRUCTORS(MultiQueueThreadPool);
};
}  // namespace thread
}  // namespace whisper

#endif  // __COMMON_SYNC_THREAD_POOL_H__
