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

#include "whisperlib/base/types.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/base/core_errno.h"
#include "whisperlib/sync/mutex.h"

#if defined(HAVE_MEMALIGN)
#include <malloc.h>
#endif

#if defined(HAVE_POSIX_MEMALIGN)
#include <stdlib.h>
#endif

namespace whisper {
namespace util {

template <typename T>
class Disposer {
 public:
    Disposer() {}
    virtual ~Disposer() {}
    virtual bool Dispose(T* p) {
        delete p;
        return true;
    }
    virtual void MultiDispose(const std::vector<T*>& p, size_t start = 0) {
        for (int i = start; i < p.size(); ++i) {
            Dispose(p[i]);
        }
    }
};

template <class C>
class scoped_dispose {
public:
    explicit scoped_dispose(Disposer<C>* disp, C* p = NULL) : disp_(disp), ptr_(p) { }
    ~scoped_dispose() { if (ptr_) { disp_->Dispose(ptr_); } }
    C& operator*() const { return *ptr_; }
    C* operator->() const  { return ptr_; }
    C* get() const { return ptr_; }
    C* release() { C* p = ptr_; ptr_ = NULL; return p; }
private:
    Disposer<C>* const disp_;
    C* ptr_;
};


template <typename T>
class FreeList : public Disposer<T> {
 public:
  explicit FreeList(size_t max_size)
    : outstanding_(0), max_size_(max_size) {
  }
  virtual ~FreeList() {
    while ( !free_list_.empty() ) {
      T* const p = free_list_.back();
      free_list_.pop_back();
      delete p;
    }
  }
  virtual T* Allocate() const = 0;
  virtual T* New() {
    ++outstanding_;
    if ( !free_list_.empty() ) {
      T* const p = free_list_.back();
      free_list_.pop_back();
      return p;
    } else {
      return Allocate();
    }
  }
  virtual bool Dispose(T* p) {
    --outstanding_;
    if ( free_list_.size() < max_size_ ) {
      free_list_.push_back(p);
      return false;
    }
    delete p;
    return true;
  }
  virtual void DisposeAll(std::vector<T*>* p) {
    outstanding_ -= p->size();
    free_list_.insert(free_list_.end(), p->begin(), p->end());
    if ( free_list_.size() > max_size_ ) {
        for ( size_t i = max_size_; i < free_list_.size(); ++i ) {
            delete free_list_[i];
        }
        free_list_.resize(max_size_);
    }
  }
  size_t max_size() const {
    return max_size_;
  }
  virtual size_t outstanding() const {
    return outstanding_;
  }

 private:
  size_t outstanding_;
  const size_t max_size_;
  std::vector<T*> free_list_;
};

template <typename T>
class SimpleFreeList : public FreeList<T> {
public:
  explicit SimpleFreeList(size_t max_size)
      : FreeList<T>(max_size) {
  }
  virtual T* Allocate() const {
    return new T;
  }
};

template <typename T, class M, class ML>
class ThreadSafeFreeList : public FreeList<T> {
 public:
  explicit ThreadSafeFreeList(size_t max_size, M* m)
      : FreeList<T>(max_size), mutex_(m)  {
  }
  virtual ~ThreadSafeFreeList() {
    delete mutex_;
  }
  virtual T* New() {
    ML ml(mutex_);
    return FreeList<T>::New();
  }
  virtual bool Dispose(T* p) {
    ML ml(mutex_);
    return FreeList<T>::Dispose(p);
  }
  virtual void DisposeAll(std::vector<T*>* p) {
    ML ml(mutex_);
    FreeList<T>::DisposeAll(p);
  }
  virtual size_t outstanding() const {
    ML ml(mutex_);
    return FreeList<T>::outstanding();
  }
 private:
  M* mutex_;
};

template <typename T>
class SimpleThreadSafeFreeList
    : public ThreadSafeFreeList<T, synch::Mutex, synch::MutexLocker> {
public:
  explicit SimpleThreadSafeFreeList(size_t max_size)
      : ThreadSafeFreeList<T, synch::Mutex, synch::MutexLocker>(max_size, new synch::Mutex()) {
  }
  virtual T* Allocate() const {
    return new T;
  }
};

template <typename T>
class SpinThreadSafeFreeList
    : public ThreadSafeFreeList<T, synch::Spin, synch::SpinLocker> {
public:
  explicit SpinThreadSafeFreeList(size_t max_size)
      : ThreadSafeFreeList<T, synch::Spin, synch::SpinLocker>(
          max_size, new synch::Spin()) {
  }
  virtual T* Allocate() const {
    return new T;
  }
};

//////////////////////////////////////////////////////////////////////

// a FreeList that stores objects. Each object is an array of T: T[]
// The size of the arrays are fixed, estabilished by constructor.
template <typename T>
class FreeArrayList : public Disposer<T> {
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
    ++outstanding_;
    if ( !free_list_.empty() ) {
      T* const p = free_list_.back();
      free_list_.pop_back();
      return p;
    } else {
      return new T[size_];
    }
  }
  virtual bool Dispose(T* p) {
    --outstanding_;
    if ( free_list_.size() < max_size_ ) {
      free_list_.push_back(p);
      return false;
    }
    delete [] p;
    return true;
  }
  virtual size_t GetOutstanding() const {
    return outstanding_;
  }
  size_t size() const {
    return size_;
  }
 protected:
  const size_t size_;
  const size_t max_size_;
  std::vector<T*> free_list_;
  size_t outstanding_;
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
  virtual size_t GetOutstanding() const {
    synch::MutexLocker ml(&mutex_);
    return FreeArrayList<T>::GetOutstanding();
  }
 protected:
  mutable synch::Mutex mutex_;
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
      std::vector<char*> p;
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
        LOG_WARNING << "Freepool exhausted";
      }
      return NULL;
    } else {
#if defined(HAVE_MEMALIGN)
        void* ret = memalign(size_ * alignment_, alignment_);
#elif defined(HAVE_POSIX_MEMALIGN)
        void* ret = NULL;
        const int err = posix_memalign(&ret, alignment_,
                                       size_ * alignment_);
        if ( err ) {
            LOG_WARNING << " Error in posix_memalign: " << errno
                        << " : " << GetSystemErrorDescription(errno);
            return NULL;
        }
#else
        void* ret = aligned_malloc(size_ * alignment_, alignment_);
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

}  // namespace util
}  // namespace whisper

#endif  // __COMMON_BASE_FREE_LIST_H__
