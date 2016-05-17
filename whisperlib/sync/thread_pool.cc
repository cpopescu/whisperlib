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

#include "whisperlib/sync/thread_pool.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/sync/event.h"
#include "whisperlib/base/gflags.h"

DEFINE_bool(detail_threadpool_log, false,
              "Log detailed information about thread pool activity");

#define LOG_INFO_THREADS LOG_INFO_IF(FLAGS_detail_threadpool_log)

namespace whisper {
namespace thread {

ThreadPoolBase::ThreadPoolBase(size_t num_threads, bool low_priority,
                               Closure* completion_callback)
  : num_threads_(num_threads),
    wait_mutex_(true),
    started_(false),
    num_completed_(0),
    completion_callback_(completion_callback) {
  // Create the threads, but don't start them yet!
  // Otherwise it results in them calling ThreadRun() which is pure virtual call!
  // We need to start the Threads from the derived class.
  for ( size_t i = 0; i < num_threads; ++i ) {
    threads_.push_back(
      new Thread(whisper::NewCallback(this, &ThreadPoolBase::Run, i),
                 low_priority));
  }
}
ThreadPoolBase::~ThreadPoolBase() {
  synch::MutexLocker l(&wait_mutex_);
  WaitForFinishLocked();
}

void ThreadPoolBase::Start() {
  synch::MutexLocker l(&wait_mutex_);
  started_ = true;
  for (size_t i = 0; i < threads_.size(); ++i) {
    threads_[i]->SetJoinable();
    threads_[i]->Start();
  }
}

void ThreadPoolBase::Run(size_t thread_index) {
  ThreadRun(thread_index);
  bool done = false;
  {
    synch::MutexLocker l(&completion_mutex_);
    ++num_completed_;
    if (num_completed_ >= threads_.size()) {
      done = true;
    }
  }
  if (done && completion_callback_) {
    completion_callback_->Run();
  }
}

void ThreadPoolBase::WaitForFinishLocked() {
  for ( size_t i = 0; i < threads_.size(); ++i ) {
    if (started_) {
      threads_[i]->Join();
    }
    delete threads_[i];
  }
  threads_.clear();
}
void ThreadPoolBase::WaitForFinish() {
  synch::MutexLocker l(&wait_mutex_);
  WaitForFinishLocked();
}
void ThreadPoolBase::KillAll() {
  synch::MutexLocker l(&wait_mutex_);
  for ( size_t i = 0; i < threads_.size(); ++i ) {
    if (started_) {
      threads_[i]->Kill();
    }
    delete threads_[i];
  }
  threads_.clear();
}

//////////////////////////////////////////////////////////////////////

LockFreeThreadPool::LockFreeThreadPool(
  size_t num_producers, size_t num_threads, size_t backlog_size, size_t sleep_usec,
  bool low_priority, Closure* completion_callback, bool auto_start)
  : ThreadPoolBase(num_threads, low_priority, completion_callback),
    num_producers_(num_producers), num_consumers_(num_threads), sleep_usec_(sleep_usec),
    jobs_(backlog_size, num_producers + 1, num_threads + 1, sleep_usec) {
  if (auto_start) {
    Start();
  }
}

LockFreeThreadPool::~LockFreeThreadPool() {
  FinishWork();
}

void LockFreeThreadPool::FinishWork() {
  LOG_INFO_THREADS << " LockFreeThreadPool Finishing work for: " << thread_count();
  synch::MutexLocker l(&wait_mutex_);
  for ( size_t i = 0; i < thread_count(); ++i ) {
    jobs_.Put(NULL, num_producers_);
  }
  LOG_INFO_THREADS << " LockFreeThreadPool Waiting for finish: " << thread_count();
  WaitForFinishLocked();
  LOG_INFO_THREADS << " LockFreeThreadPool Work Finished: " << thread_count();
}

void LockFreeThreadPool::ThreadRun(size_t thread_index) {
  int64 total_wait = 0;
  int64 total_run = 0;
  const int64 thread_start_time = timer::TicksNsec();
  size_t num_jobs = 0;
  while ( true ) {
    const int64 start_time = timer::TicksNsec();
    Callback1<size_t>* callback = jobs_.Get(thread_index);
    const int64 got_time = timer::TicksNsec();
    total_wait += (got_time - start_time);
    if ( callback == NULL ) {
      LOG_INFO_THREADS << " LockFreeThreadPool ended - thread id: " << thread_index
               << " with: " << num_jobs << " jobs."
               << " total time: " << (got_time - thread_start_time) * 1e-6 << " ms "
               << " wait time: " << total_wait * 1e-6 << " ms "
               << " run time: " << total_run * 1e-6 << " ms ";
      break;
    }
    callback->Run(thread_index);
    const int64 run_time = timer::TicksNsec();
    total_run += run_time - got_time;
    ++num_jobs;
  }
}

LockFreeMultiQueueThreadPool::LockFreeMultiQueueThreadPool(
  size_t num_producers, size_t num_threads,
  size_t backlog_size, size_t sleep_usec,
  bool low_priority, Closure* completion_callback, bool auto_start)
  : ThreadPoolBase(num_threads, low_priority, completion_callback),
    num_producers_(num_producers), num_consumers_(num_threads), sleep_usec_(sleep_usec),
    jobs_(num_threads) {
  for (size_t i = 0; i < num_threads; ++i) {
    jobs_[i] = new synch::LockFreeProducerConsumerQueue< Callback1<size_t> >(
      backlog_size, num_producers + 1, num_threads + 1, sleep_usec);
  }
  if (auto_start) {
    Start();
  }
}

LockFreeMultiQueueThreadPool::~LockFreeMultiQueueThreadPool() {
  FinishWork();
  for (size_t i = 0; i < jobs_.size(); ++i) {
    delete jobs_[i];
  }
  jobs_.clear();
}

void LockFreeMultiQueueThreadPool::FinishWork() {
  LOG_INFO_THREADS << " LockFreeMultiQueueThreadPool Finishing work for: " << thread_count();
  synch::MutexLocker l(&wait_mutex_);
  DCHECK_EQ(jobs_.size(), thread_count());
  for (size_t i = 0; i < jobs_.size(); ++i ) {
    jobs_[i]->Put(NULL, num_producers_);
  }
  LOG_INFO_THREADS << " LockFreeMultiQueueThreadPool Waiting for finish: " << thread_count();
  WaitForFinishLocked();
  LOG_INFO_THREADS << " LockFreeMultiQueueThreadPool Work Finished: " << thread_count();
}


void LockFreeMultiQueueThreadPool::ThreadRun(size_t thread_index) {
  LOG_INFO_THREADS << " LockFreeMultiQueueThreadPool Running thread: " << thread_index;
  int64 total_wait = 0;
  int64 total_run = 0;
  const int64 thread_start_time = timer::TicksNsec();
  size_t num_jobs = 0;
  while ( true ) {
    const int64 start_time = timer::TicksNsec();
    Callback1<size_t>* callback = jobs_[thread_index]->Get(thread_index);
    const int64 got_time = timer::TicksNsec();
    total_wait += (got_time - start_time);
    if ( callback == NULL ) {
      LOG_INFO_THREADS << " LockFreeMultiQueueThreadPool ended -  thread id: "
               << thread_index << " with: " << num_jobs << " jobs."
               << " total time: " << (got_time - thread_start_time) * 1e-6 << " ms "
               << " wait time: " << total_wait * 1e-6 << " ms "
               << " run time: " << total_run * 1e-6 << " ms ";
      break;
    }
    callback->Run(thread_index);
    const int64 run_time = timer::TicksNsec();
    total_run += run_time - got_time;
    ++num_jobs;
  }
}

//////////////////////////////////////////////////////////////////////

ThreadPool::ThreadPool(size_t num_threads, size_t backlog_size, bool low_priority,
                       Closure* completion_callback, bool auto_start)
  : ThreadPoolBase(num_threads, low_priority, completion_callback),
    jobs_(backlog_size),
    count_completed_(num_threads, 0) {
  CHECK_GT(backlog_size, num_threads);
  if (auto_start) {
    Start();
  }
}


ThreadPool::~ThreadPool() {
  Closure* callback;
  do {
    callback = jobs_.Get(0);
    if ( callback != NULL && !callback->is_permanent() ) {
      delete callback;
    }
  } while ( callback != NULL );

  synch::MutexLocker l(&wait_mutex_);
  for (size_t i = 0; i < thread_count(); ++i ) {
    while ( !jobs_.Put(NULL, 0) ) {
      callback = jobs_.Get(0);
      if ( callback != NULL && !callback->is_permanent() ) {
        delete callback;
      }
    }
  }
  WaitForFinishLocked();
}

namespace {
// A batch of jobs, to be executed in parallel.
struct Batch {
  Batch(const vector<Closure*>& jobs, Closure* completion)
    : completion_callback_(completion),
      completion_event_(false, true) {
      // copy only valid jobs (ignore NULLs)
      for (Closure* job : jobs) { if (job) { jobs_.push_back(job); } }
      num_to_finish_ = jobs_.size();
  }
  ~Batch() {
    CHECK(jobs_.empty());
    delete executor_;
  }
  size_t jobs_size() const { return jobs_.size(); }
  Closure* executor() { return executor_; }
  void set_executor(Closure* executor) {
    CHECK(executor->is_permanent());
    executor_ = executor;
  }
  void Wait() {
    CHECK_NULL(completion_callback_);
    completion_event_.Wait();
  }
private:
  // popped one by one and executed
  std::list<Closure*> jobs_;

  // how many jobs are left to_finish
  size_t num_to_finish_ = 0;

  // Permanent callback to RunBatchJob()
  Closure* executor_ = NULL;

  // [optional]
  // If non NULL, call this when all jobs completed.
  Closure* completion_callback_;

  // signaled when all jobs completed.
  synch::Event completion_event_;

  // synchronize jobs_ management
  synch::Mutex mutex_;

  // friend function
  friend void RunBatchJob(Batch* batch);
};

// Run a single job from the given batch
void RunBatchJob(Batch* batch) {
  Closure* job = NULL;
  {
    synch::MutexLocker l(&batch->mutex_);
    job = batch->jobs_.front();
    batch->jobs_.pop_front();
  }

  CHECK_NOT_NULL(job);
  job->Run();

  bool was_last_job = false;
  {
    synch::MutexLocker l(&batch->mutex_);
    --batch->num_to_finish_;
    was_last_job = (batch->num_to_finish_ == 0);
  }
  if (was_last_job) {
    if (batch->completion_callback_) {
      batch->completion_callback_->Run();
      delete batch;
      return;
    }
    // NOTE: Signal() must be last, because the client may
    // immediately delete the batch
    batch->completion_event_.Signal();
    return;
  }
}
}

void ThreadPool::RunBatch(const vector<Closure*>& jobs, Closure* completion) {
  Batch* batch = new Batch(jobs, completion);
  batch->set_executor(NewPermanentCallback(&RunBatchJob, batch));
  // WARN: the execution starts as soon each job is inserted into jobs_!
  //       So the batch modifies (jobs are executed) as we iterate.
  for (size_t i = 0, count = batch->jobs_size(); i < count; i++) {
    jobs_.Put(batch->executor());
  }
  if (!completion) {
    // wait for completion
    batch->Wait();
    delete batch;
  }
  // the 'batch' will be automatically deleted after the last job is executed
}


void ThreadPool::FinishWork() {
  LOG_INFO_THREADS << " ThreadPool Finishing work for: " << thread_count();
  synch::MutexLocker l(&wait_mutex_);
  for ( size_t i = 0; i < thread_count(); ++i ) {
    jobs_.Put(NULL);
  }
  LOG_INFO_THREADS << " ThreadPool Waiting for finish: " << thread_count();
  WaitForFinishLocked();
  LOG_INFO_THREADS << " ThreadPool Work Finished: " << thread_count();
}


void ThreadPool::ThreadRun(size_t thread_index) {
  LOG_INFO_THREADS << " ThreadPool Running thread: " << thread_index;
  int64 total_wait = 0;
  int64 total_run = 0;
  const int64 thread_start_time = timer::TicksNsec();
  size_t num_jobs = 0;
  while ( true ) {
    const int64 start_time = timer::TicksNsec();
    Closure* callback = jobs_.Get();
    const int64 got_time = timer::TicksNsec();
    total_wait += (got_time - start_time);
    if ( callback == NULL ) {
      LOG_INFO_THREADS << " ThreadPool ended -  thread id: "
               << thread_index << " with: " << num_jobs << " jobs."
               << " total time: " << (got_time - thread_start_time) * 1e-6 << " ms "
               << " wait time: " << total_wait * 1e-6 << " ms "
               << " run time: " << total_run * 1e-6 << " ms ";
      break;
    }
    callback->Run();
    count_completed_[thread_index]++;
    const int64 run_time = timer::TicksNsec();
    total_run += run_time - got_time;
    ++num_jobs;
  }
}

////////////////////////////////////////////////////////////////////////////////

MultiQueueThreadPool::MultiQueueThreadPool(size_t num_threads, size_t backlog_size,
                                           bool low_priority,
                                           Closure* completion_callback, bool auto_start)
  : ThreadPoolBase(num_threads, low_priority, completion_callback) {
  for (size_t i = 0; i < num_threads; ++i) {
    jobs_.push_back(new synch::ProducerConsumerQueue<Closure*>(backlog_size));
  }
  if (auto_start) {
    Start();
  }
}


MultiQueueThreadPool::~MultiQueueThreadPool() {
  Closure* callback;
  for (size_t i = 0; i < jobs_.size(); ++i) {
    do {
      callback = jobs_[i]->Get(0);
      if ( callback != NULL && !callback->is_permanent() ) {
        delete callback;
      }
    } while ( callback != NULL );
  }
  synch::MutexLocker l(&wait_mutex_);
  for (size_t i = 0; i < jobs_.size(); ++i ) {
    while ( !jobs_[i]->Put(NULL, 0) ) {
      callback = jobs_[i]->Get(0);
      if ( callback != NULL && !callback->is_permanent() ) {
        delete callback;
      }
    }
  }
  WaitForFinishLocked();
  for (size_t i = 0; i < jobs_.size(); ++i) {
    delete jobs_[i];
  }
}


void MultiQueueThreadPool::FinishWork() {
  LOG_INFO_THREADS << " MultiQueueThreadPool Finishing work for: " << thread_count();
  synch::MutexLocker l(&wait_mutex_);
  for ( size_t i = 0; i < jobs_.size(); ++i ) {
    jobs_[i]->Put(NULL);
  }
  LOG_INFO_THREADS << " MultiQueueThreadPool Waiting for finish: " << thread_count();
  WaitForFinishLocked();
  LOG_INFO_THREADS << " MultiQueueThreadPool Work Finished: " << thread_count();
}


void MultiQueueThreadPool::ThreadRun(size_t thread_index) {
  LOG_INFO_THREADS << " ThreadPool Running thread: " << thread_index;
  int64 total_wait = 0;
  int64 total_run = 0;
  const int64 thread_start_time = timer::TicksNsec();
  size_t num_jobs = 0;
  while ( true ) {
    const int64 start_time = timer::TicksNsec();
    Closure* callback = jobs_[thread_index]->Get();
    const int64 got_time = timer::TicksNsec();
    total_wait += (got_time - start_time);
    if ( callback == NULL ) {
      LOG_INFO_THREADS << " ThreadPool ended -  thread id: "
               << thread_index << " with: " << num_jobs << " jobs."
               << " total time: " << (got_time - thread_start_time) * 1e-6 << " ms "
               << " wait time: " << total_wait * 1e-6 << " ms "
               << " run time: " << total_run * 1e-6 << " ms ";
      break;
    }
    callback->Run();
    const int64 run_time = timer::TicksNsec();
    total_run += run_time - got_time;
    ++num_jobs;
  }
}

////////////////////////////////////////////////////////////////////////////////

LockedThreadPool::LockedThreadPool(size_t /*num_producers*/, size_t num_threads,
                                   size_t backlog_size, size_t /*sleep_usec*/,
                                   bool low_priority, Closure* completion_callback,
                                   bool auto_start)
  : ThreadPoolBase(num_threads, low_priority, completion_callback),
    jobs_(backlog_size),
    count_completed_(num_threads, 0) {
  CHECK_GT(backlog_size, num_threads);
  if (auto_start) {
    Start();
  }
}


LockedThreadPool::~LockedThreadPool() {
  Callback1<size_t>* callback;
  do {
    callback = jobs_.Get(0);
    if ( callback != NULL && !callback->is_permanent() ) {
      delete callback;
    }
  } while ( callback != NULL );

  synch::MutexLocker l(&wait_mutex_);
  for (size_t i = 0; i < thread_count(); ++i ) {
    while ( !jobs_.Put(NULL, 0) ) {
      callback = jobs_.Get(0);
      if ( callback != NULL && !callback->is_permanent() ) {
        delete callback;
      }
    }
  }
  WaitForFinishLocked();
}

void LockedThreadPool::FinishWork() {
  LOG_INFO_THREADS << " LockedThreadPool Finishing work for: " << thread_count();
  synch::MutexLocker l(&wait_mutex_);
  for ( size_t i = 0; i < thread_count(); ++i ) {
    jobs_.Put(NULL);
  }
  LOG_INFO_THREADS << " LockedThreadPool Waiting for finish: " << thread_count();
  WaitForFinishLocked();
  LOG_INFO_THREADS << " LockedThreadPool Work Finished: " << thread_count();
}


void LockedThreadPool::ThreadRun(size_t thread_index) {
  LOG_INFO_THREADS << " LockedThreadPool Running thread: " << thread_index;
  int64 total_wait = 0;
  int64 total_run = 0;
  const int64 thread_start_time = timer::TicksNsec();
  size_t num_jobs = 0;
  while ( true ) {
    const int64 start_time = timer::TicksNsec();
    Callback1<size_t>* callback = jobs_.Get();
    const int64 got_time = timer::TicksNsec();
    total_wait += (got_time - start_time);
    if ( callback == NULL ) {
      LOG_INFO_THREADS << " ThreadPool ended -  thread id: "
               << thread_index << " with: " << num_jobs << " jobs."
               << " total time: " << (got_time - thread_start_time) * 1e-6 << " ms "
               << " wait time: " << total_wait * 1e-6 << " ms "
               << " run time: " << total_run * 1e-6 << " ms ";
      break;
    }
    callback->Run(thread_index);
    count_completed_[thread_index]++;
    const int64 run_time = timer::TicksNsec();
    total_run += run_time - got_time;
    if (run_time - got_time > 1e9) {
      LOG_INFO_THREADS << "At index: " << thread_index << " ==> Long run time of: "
                       << (run_time - got_time) * 1e-9;
    }
    ++num_jobs;
  }
}

}  // namespace thread
}  // namespace whisper
