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

#ifndef __WHISPERLIB_PRODUCER_CONSUMER_QUEUE_H__
#define __WHISPERLIB_PRODUCER_CONSUMER_QUEUE_H__

#include <errno.h>
#include <pthread.h>
#include <deque>
#include <vector>
#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/timer.h>
#include <whisperlib/sync/mutex.h>

namespace synch {

template<typename C>
class ProducerConsumerQueue {
 public:
    explicit ProducerConsumerQueue(int max_size, bool fifo_policy = true)
        : max_size_(max_size),
          fifo_policy_(fifo_policy) {
    CHECK_SYS_FUN(pthread_mutex_init(&mutex_, NULL), 0);
    CHECK_SYS_FUN(pthread_cond_init(&cond_full_, NULL), 0);
    CHECK_SYS_FUN(pthread_cond_init(&cond_empty_, NULL), 0);
  }
  ~ProducerConsumerQueue() {
    CHECK_SYS_FUN(pthread_mutex_destroy(&mutex_), 0);
    CHECK_SYS_FUN(pthread_cond_destroy(&cond_full_), 0);
    CHECK_SYS_FUN(pthread_cond_destroy(&cond_empty_), 0);
  }
  pthread_mutex_t mutex() {
        return mutex_;
  }
  deque<C>* data() {
      return &data_;
  }
  void Put(C p) {
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    while ( data_.size() >= max_size_ ) {
      CHECK_SYS_FUN(pthread_cond_wait(&cond_empty_, &mutex_), 0);
    }
    if (fifo_policy_) {
        data_.push_back(p);
    } else {
        data_.push_front(p);
    }
    CHECK_SYS_FUN(pthread_cond_signal(&cond_full_), 0);
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
  }
  bool Put(C p, uint32 timeout_in_ms) {
    if ( kInfiniteWait == timeout_in_ms ) {
      Put(p);
      return true;
    }
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    if ( timeout_in_ms && data_.size() >= max_size_ ) {
      struct timespec ts = timer::TimespecAbsoluteMsec(timeout_in_ms);
      const int result = pthread_cond_timedwait(&cond_empty_, &mutex_, &ts);
      CHECK(result == 0 || result == ETIMEDOUT)
        << " Invalid result: " << result;
    }
    if ( data_.size() >= max_size_ ) {
      CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
      return false;
    }
    if (fifo_policy_) {
        data_.push_back(p);
    } else {
        data_.push_front(p);
    }
    CHECK_SYS_FUN(pthread_cond_signal(&cond_full_), 0);
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
    return true;
  }
  C Get() {
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    while ( data_.empty() ) {
      CHECK_SYS_FUN(pthread_cond_wait(&cond_full_, &mutex_), 0);
    }
    C const ret = data_.front();
    data_.pop_front();
    CHECK_SYS_FUN(pthread_cond_signal(&cond_empty_), 0);
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
    return ret;
  }
  C Get(uint32 timeout_in_ms) {
    if ( kInfiniteWait == timeout_in_ms ) {
      return Get();
    }
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    if ( timeout_in_ms && data_.empty() ) {
      struct timespec ts = timer::TimespecAbsoluteMsec(timeout_in_ms);
      const int result = pthread_cond_timedwait(&cond_full_, &mutex_, &ts);
      CHECK(result == 0 || result == ETIMEDOUT)
        << " Invalid result: " << result;
    }
    C ret(NULL);
    if ( !data_.empty() ) {
      ret = data_.front();
      data_.pop_front();
    }
    CHECK_SYS_FUN(pthread_cond_signal(&cond_empty_), 0);
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
    return ret;
  }
  void GetAll(vector<C>* out) {
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    while ( !data_.empty() ) {
      out->push_back(data_.front());
      data_.pop_front();
    }
    CHECK_SYS_FUN(pthread_cond_signal(&cond_empty_), 0);
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
  }
  void Clear() {
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    data_.clear();
    CHECK_SYS_FUN(pthread_cond_signal(&cond_empty_), 0);
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
  }
  bool IsFull() {
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    const bool is_full (data_.size() >= max_size_);
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
    return is_full;
  }
  int32 Size() {
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    const int32 size = data_.size();
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
    return size;
  }

  static const uint32 kInfiniteWait = 0xffffffff;

 protected:
  pthread_mutex_t mutex_;
  pthread_cond_t cond_full_;   // triggered when is some element in queue
  pthread_cond_t cond_empty_;  // triggered when is some free space in queue

  const int max_size_;
  const int fifo_policy_;
  deque<C> data_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ProducerConsumerQueue);
};
}

#endif  //  __COMMON_PRODUCER_CONSUMER_QUEUR_H__
