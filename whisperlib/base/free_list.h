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

#ifndef __COMMON_BASE_FREE_LIST_H__
#define __COMMON_BASE_FREE_LIST_H__

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <stdlib.h>

#include <vector>
#include <whisperlib/base/types.h>
#include <whisperlib/base/timer.h>
#include <whisperlib/base/errno.h>
#include <whisperlib/sync/mutex.h>

namespace util {

template <typename T>
class FreeList {
 public:
  explicit FreeList(size_t max_size)
    : max_size_(max_size) {
  }
  virtual ~FreeList() {
    while ( !free_list_.empty() ) {
      T* const p = free_list_.back();
      free_list_.pop_back();
      delete p;
    }
  }
  virtual T* New() {
    if ( !free_list_.empty() ) {
      T* const p = free_list_.back();
      free_list_.pop_back();
      return p;
    } else {
      return new T;
    }
  }
  virtual bool Dispose(T* p) {
    if ( free_list_.size() < max_size_ ) {
      free_list_.push_back(p);
      return false;
    }
    delete p;
    return true;
  }
  size_t max_size() const {
    return max_size_;
  }
 private:
  const size_t max_size_;
  vector<T*> free_list_;
};

template <typename T>
class ThreadSafeFreeList : public FreeList<T> {
 public:
  explicit ThreadSafeFreeList(size_t max_size)
    : FreeList<T>(max_size) {
  }
  virtual T* New() {
    synch::MutexLocker ml(&mutex_);
    return FreeList<T>::New();
  }
  virtual bool Dispose(T* p) {
    synch::MutexLocker ml(&mutex_);
    return FreeList<T>::Dispose(p);
  }
 private:
  synch::Mutex mutex_;
};

//////////////////////////////////////////////////////////////////////

// a FreeList that stores objects. Each object is an array of T: T[]
// The size of the arrays are fixed, estabilished by constructor.
template <typename T>
class FreeArrayList {
 public:
  // size: This list stores array. This is the size of each array.
  // max_size: maximum list size.
  FreeArrayList(size_t size, size_t max_size)
    : size_(size),
      max_size_(max_size) {
  }
  virtual ~FreeArrayList() {
    while ( !free_list_.empty() ) {
      T* const p = free_list_.back();
      free_list_.pop_back();
      delete [] p;
    }
  }
  virtual T* New() {
    if ( !free_list_.empty() ) {
      T* const p = free_list_.back();
      free_list_.pop_back();
      return p;
    } else {
      return new T[size_];
    }
  }
  virtual bool Dispose(T* p) {
    if ( free_list_.size() < max_size_ ) {
      free_list_.push_back(p);
      return false;
    }
    delete [] p;
    return true;
  }
  size_t size() const {
    return size_;
  }
 protected:
  const size_t size_;
  const size_t max_size_;
  vector<T*> free_list_;
};

template <typename T>
class ThreadSafeFreeArrayList : public FreeArrayList<T> {
 public:
  ThreadSafeFreeArrayList(size_t size, size_t max_size)
    : FreeArrayList<T>(size, max_size) {
  }
  virtual T* New() {
    synch::MutexLocker ml(&mutex_);
    return FreeArrayList<T>::New();
  }
  virtual bool Dispose(T* p) {
    synch::MutexLocker ml(&mutex_);
    return FreeArrayList<T>::Dispose(p);
  }
 protected:
  synch::Mutex mutex_;
};

//////////////////////////////////////////////////////////////////////

#if !defined(HAVE_MEMALIGN) && !defined(HAVE_POSIX_MEMALIGN)
inline void* aligned_malloc(size_t size, int align) {
  const size_t alloc_size = size + (align-1) + sizeof(void*);
  void* const mem = malloc(alloc_size);
  const int64 pos = reinterpret_cast<int64>(reinterpret_cast<char*>(mem) +
                                            sizeof(void*));
  const size_t delta = align - (pos % align);
  *(reinterpret_cast<void**>(reinterpret_cast<char*>(mem) + delta)) = mem;
  return reinterpret_cast<void*>(reinterpret_cast<char*>(mem) +
                                 delta + sizeof(void*));
}
inline void aligned_free(void* mem ) {
  free( ((void**)mem)[-1] );
}
#endif

//////////////////////////////////////////////////////////////////////

// ATTENTION : here the size is in blocks !!
class MemAlignedFreeArrayList : public ThreadSafeFreeArrayList<char> {
 public:
  MemAlignedFreeArrayList(size_t size_in_blocks,
                          size_t alignment,
                          size_t max_size,
                          bool alloc_initially = false)
    : ThreadSafeFreeArrayList<char>(size_in_blocks, max_size),
      alignment_(alignment),
      alloc_initially_(false),
      last_error_log_ts_(0) {
    if ( alloc_initially ) {
      vector<char*> p;
      for ( size_t i = 0; i < max_size; ++i ) {
        p.push_back(NewInternal());
        CHECK(p.back() != NULL);
      }
      for ( size_t i = 0; i < p.size(); ++i ) {
        DisposeInternal(p[i]);
      }
      alloc_initially_ = true;
    }
  }
  virtual ~MemAlignedFreeArrayList() {
    while ( !free_list_.empty() ) {
      char* const p = free_list_.back();
      free_list_.pop_back();
      free(p);
    }
  }
  virtual char* New() {
    synch::MutexLocker ml(&mutex_);
    return NewInternal();
  }
  virtual bool Dispose(char* p) {
    synch::MutexLocker ml(&mutex_);
    return DisposeInternal(p);
  }
  size_t alignment() const {
    return alignment_;
  }
 private:
  char* NewInternal() {
    if ( !free_list_.empty() ) {
      char* const p = free_list_.back();
      free_list_.pop_back();
      return p;
    } else if ( alloc_initially_ ) {
      int64 now = timer::TicksMsec();
      if ( now > last_error_log_ts_ + 10000 ) {
        last_error_log_ts_ = now;
        LOG_ERROR << "Freepool exhausted";
      }
      return NULL;
    } else {
#if defined(HAVE_MEMALIGN)
      void* ret = memalign(size_ * alignment_, alignment_);
#else
#if defined(HAVE_POSIX_MEMALIGN)
      void* ret = NULL;
      const int err = posix_memalign(&ret,
                                     alignment_,
                                     size_ * alignment_);
      if ( err ) {
        LOG_ERROR << " Error in posix_memalign: " << errno
                  << " : " << GetSystemErrorDescription(errno);
        return NULL;
      }
#else
      void* ret = aligned_malloc(size_ * alignment_, alignment_);
#endif  // defined(HAVE_POSIX_MEMALIGN)
#endif  // defined(HAVE_MEMALIGN)
      return reinterpret_cast<char*>(ret);
    }
  }
  bool DisposeInternal(char* p) {
    if ( free_list_.size() < max_size_ ) {
      free_list_.push_back(p);
      return false;
    }
    CHECK(!alloc_initially_);
#if defined(HAVE_MEMALIGN) || defined(HAVE_POSIX_MEMALIGN)
    free(p);
#else
    aligned_free(p);
#endif
    return true;
  }
 private:
  const size_t alignment_;
  bool alloc_initially_;
  // prevent printing error messages too often
  int64 last_error_log_ts_;
};

}

#endif  // __COMMON_BASE_FREE_LIST_H__
