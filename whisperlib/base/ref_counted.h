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
// Author: Cosmin Tudorache & Catalin Popescu

#ifndef __WHISPERLIB_BASE_REF_COUNTED_H__
#define __WHISPERLIB_BASE_REF_COUNTED_H__

#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/sync/mutex.h>

// Reference counted through encapsulation.
// The "ref_counted" contains the object C
template<class C>
class ref_counted {
 public:
  explicit ref_counted(C* data, synch::Mutex* mutex)
      : data_(data),
        mutex_(mutex),
        ref_count_(0) {
    CHECK_NOT_NULL(mutex_);
  }
  ~ref_counted() {
#ifndef NDEBUG
    mutex_->Lock();
    DCHECK_EQ(ref_count_, 0);
    mutex_->Unlock();
#endif
    delete data_;
  }
  void IncRef() {
    mutex_->Lock();
    ++ref_count_;
    mutex_->Unlock();
  }
  void DecRef() {
    mutex_->Lock();
    DCHECK_GT(ref_count_, 0);
    --ref_count_;
    const bool do_delete = (ref_count_ == 0);
    mutex_->Unlock();
    if ( do_delete ) {
      delete this;
    }
  }
  int ref_count() const {
    return ref_count_;
  }
  C& operator*() const {
    DCHECK(data_ != NULL);
    return *data_;
  }
  C* operator->() const  {
    DCHECK(data_ != NULL);
    return data_;
  }
  C* get() const {
    return data_;
  }
  void clear() {
      data_ = NULL;
  }
 private:
  C* data_;
  synch::Mutex* const mutex_;
  int ref_count_;
  DISALLOW_EVIL_CONSTRUCTORS(ref_counted);
};

// Reference counted through inheritance.
// The object C derives from RefCounted.
class RefCounted {
 public:
  // If you provide a 'mutex', IncRef() and DecRef() will be synchronized
  // by the given 'mutex'.
  RefCounted(synch::Mutex* mutex)
    : ref_count_(0),
      ref_mutex_(mutex) {
  }
  virtual ~RefCounted() {
    DCHECK_EQ(ref_count_, 0);
  }
  int ref_count() const {
    return ref_count_;
  }
  void IncRef() const {
    ref_mutex_->Lock();
    IncRefLocked();
    ref_mutex_->Unlock();
  }
  // returns: true = this object was deleted.
  bool DecRef() const {
    ref_mutex_->Lock();
    const bool do_delete = DecRefLocked();
    ref_mutex_->Unlock();

    if ( do_delete ) {
      delete this;
    }
    return do_delete;
  }

 protected:
  void IncRefLocked() const {
    ref_count_++;
  }
  bool DecRefLocked() const {
    DCHECK_GT(ref_count_, 0);
    ref_count_--;
    return ref_count_ == 0;
  }


  mutable int ref_count_;
  synch::Mutex* const ref_mutex_;

  DISALLOW_EVIL_CONSTRUCTORS(RefCounted);
};

template <typename REF_COUNTED>
class scoped_ref {
public:
  scoped_ref()
    : p_(NULL) {}
  scoped_ref(REF_COUNTED* p)
    : p_(p) {
    if ( p_ != NULL ) {
      p_->IncRef();
    }
  }
  scoped_ref(const scoped_ref<REF_COUNTED>& other)
    : p_(other.p_) {
    if ( p_ != NULL ) {
      p_->IncRef();
    }
  }
  ~scoped_ref() {
    if ( p_ != NULL ) {
      p_->DecRef();
      p_ = NULL;
    }
  }
  REF_COUNTED* get() const {
    return p_;
  }
  void reset(REF_COUNTED* other) {
    if (p_ != other) {
      if ( p_ != NULL ) {
        p_->DecRef();
      }
      p_ = other;
      if ( p_ != NULL ) {
        p_->IncRef();
      }
    }
  }
  // utility: moves the pointer from here to 'out'
  template <typename TT>
  void MoveTo(scoped_ref<TT>* out) {
    out->reset(get());
    reset(NULL);
  }
  REF_COUNTED& operator*() const {
    CHECK_NOT_NULL(p_);
    return *p_;
  }
  REF_COUNTED* operator->() const  {
    CHECK_NOT_NULL(p_);
    return p_;
  }
  void operator=(REF_COUNTED* other) {
    reset(other);
  }
  scoped_ref<REF_COUNTED> operator=(const scoped_ref<REF_COUNTED>& other) {
    operator=(other.p_);
    return *this;
  }
  REF_COUNTED* release() {
    REF_COUNTED* ret = p_;
    p_ = NULL;
    return ret;
  }
  string ToString() const {
    return p_ == NULL ? "NULL" : p_->ToString();
  }

private:
  REF_COUNTED* p_;
};

#endif  // __WHISPERLIB_BASE_REF_COUNTED_H__
