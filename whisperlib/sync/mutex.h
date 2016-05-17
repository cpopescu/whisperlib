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
// Author: Cosmin Tudorache

#ifndef __WHISPERLIB_SYNC_MUTEX_H__
#define __WHISPERLIB_SYNC_MUTEX_H__

#include <unistd.h>
#include <pthread.h>
#include "whisperlib/base/log.h"
#include "whisperlib/base/types.h"

#if HAVE_ATOMIC
#include <atomic>
#if HAVE_EMMINTRIN
#include <emmintrin.h>
#endif  // HAVE_EMMINTRIN
#endif

namespace whisper {
namespace synch {

class Mutex {
 protected:
  pthread_mutex_t mutex_;
  pthread_mutexattr_t attr_;
  bool initialized_;
  size_t is_held_count_;
public:
  explicit Mutex(bool is_reentrant = false);
  explicit Mutex(const pthread_mutex_t& mutex);
  virtual ~Mutex();

  const pthread_mutex_t& mutex() const {
      return mutex_;
  }
  pthread_mutex_t& mutex() {
      return mutex_;
  }

  void Lock() {
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
    ++is_held_count_;
  }
  void Unlock() {
    DCHECK_GT(is_held_count_, 0);
    --is_held_count_;
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
  }

  // Classic named Lock
  void P() {  Lock(); }
  // Classic named Unlock
  void V() {  Unlock(); }

  // Use this for assertions and such, only,
  bool IsHeld() const {
      return is_held_count_ > 0;
  }

private:
  template<typename T> Mutex(T disallowed_all_types_except_bool);
  DISALLOW_EVIL_CONSTRUCTORS(Mutex);
};

#if HAVE_ATOMIC
class Spin {
public:
    Spin() {
    }
    void Lock() {
        while (true) {
            if (!locker_.test_and_set(std::memory_order_acquire)) {
                return;
            }
#if HAVE_EMMINTRIN
            _mm_pause();
#endif
        }
    }
    void  Unlock() {
        locker_.clear(std::memory_order_release);
    }
private:
    std::atomic_flag locker_ = ATOMIC_FLAG_INIT;
};

class SpinWithSleep {
public:
    explicit SpinWithSleep(size_t sleep_usec)
        : sleep_usec_(sleep_usec) {
    }
    void Lock() {
        while (true) {
            if (!locker_.test_and_set(std::memory_order_acquire)) {
                return;
            }
            usleep(sleep_usec_);
        }
    }
    void  Unlock() {
        locker_.clear(std::memory_order_release);
    }

    void set_sleep_usec(size_t sleep_usec) {
        sleep_usec_ = sleep_usec;
    }
private:
    size_t sleep_usec_;
    std::atomic_flag locker_ = ATOMIC_FLAG_INIT;
};

#else  // ! HAVE_ATOMIC

class Spin : public Mutex {};

struct SpinWithSleep : public Mutex {
    explicit SpinWithSleep(size_t /*sleep_usec*/) {}
};

#endif  // HAVE_ATOMIC


// A pool of mutex - from how GetMutex works, try to use a prime numbers for num,
// else it will choose same mutexes..
class MutexPool {
 public:
  MutexPool(size_t num, bool is_reentrant = false);
  ~MutexPool();

  Mutex* GetMutex(size_t p) {
    return mutex_[p % num_];
  }
  Mutex* GetMutex(const void* p) {
    // 'p' can have any value!
    // e.g. 0xffffffff when cast to int => -1
    return GetMutex(reinterpret_cast<size_t>(p));
  }
  size_t num() const {
    return num_;
  }
 private:
  Mutex** mutex_;
  const size_t num_;
};

// Utility class for scope protected locking
template <class L>
class LockerBase {
 public:
  explicit LockerBase(L* mutex) : mutex_(mutex) {
    mutex_->Lock();
  }
  ~LockerBase() {
    mutex_->Unlock();
  }
 protected:
  L* const mutex_;

  DISALLOW_EVIL_CONSTRUCTORS(LockerBase);
};

// The only difference from LockerBase is that mutex_ can be NULL
template <class L>
class OptionalLockerBase {
 public:
  explicit OptionalLockerBase(L* mutex) : mutex_(mutex) {
    if (mutex_) mutex_->Lock();
  }
  ~OptionalLockerBase() {
    if (mutex_) mutex_->Unlock();
  }
 protected:
  L* const mutex_;
};

typedef LockerBase<Mutex> MutexLocker;
typedef OptionalLockerBase<Mutex> OptionalMutexLocker;

#if HAVE_ATOMIC
typedef LockerBase<Spin> SpinLocker;
typedef LockerBase<SpinWithSleep> SpinWithLocker;
#else
typedef LockerBase<Mutex> SpinLocker;
typedef LockerBase<Mutex> SpinWithLocker;
#endif   // HAVE_ATOMIC

}  // namespace synch
}  // namespace whisper

#endif  // __COMMON_SYNC_MUTEX_H__
