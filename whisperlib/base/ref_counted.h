// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
//
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

#include <atomic>
#include <string>
#include "whisperlib/base/log.h"
#include "whisperlib/base/callback.h"
#include "whisperlib/base/types.h"

// Reference counted through encapsulation.
// The "ref_counted" contains the object C
template<class C>
class ref_counted {
public:
  explicit ref_counted(C* data)
    : ref_count_(0),
      data_(data),
      release_cb_(NULL) {
  }

  ~ref_counted() {
    if (release_cb_ != NULL) {
      release_cb_->Run(data_);
    } else {
      delete data_;
    }
  }

  void IncRef() const {
    ref_count_.fetch_add(1);
  }

  // returns: true = this object was deleted.
  bool DecRef() const {
    const bool do_delete = 1 == ref_count_.fetch_sub(1);
    DCHECK_GE(ref_count_.load(), 0);
    if (do_delete) {
      delete this;
      return true;
    }
    return false;
  }

  int ref_count() const {
    return ref_count_.load();
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
  void set_release_cb(whisper::Callback1<C*>* release_cb) {
    release_cb_ = release_cb;
  }
  C* reset(C* data) {
    C* const d = data_;
    data_ = data;
    return d;
  }

 private:
  mutable std::atomic_int ref_count_;
  C* data_;
  whisper::Callback1<C*>* release_cb_;
  DISALLOW_EVIL_CONSTRUCTORS(ref_counted);
};

// Reference counted through inheritance.
// The object C derives from RefCounted.
class RefCounted {
 public:
  RefCounted() : ref_count_(0) {
  }
  virtual ~RefCounted() {
    DCHECK_EQ(ref_count_.load(), 0);
  }

  int ref_count() const {
    return ref_count_.load();
  }

  void IncRef() const {
    ref_count_.fetch_add(1);
  }

  // returns: true = this object was deleted.
  bool DecRef() const {
    const bool do_delete = 1 == ref_count_.fetch_sub(1);
    if (do_delete) {
      delete this;
      return true;
    }
    return false;
  }

 private:
  mutable std::atomic_int ref_count_;
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
  scoped_ref<REF_COUNTED>& operator=(REF_COUNTED* other) {
    reset(other);
    return *this;
  }
  scoped_ref<REF_COUNTED>& operator=(const scoped_ref<REF_COUNTED>& other) {
    return operator=(other.p_);
  }

  REF_COUNTED* release() {
    REF_COUNTED* ret = p_;
    p_ = NULL;
    return ret;
  }
  std::string ToString() const {
    return p_ == NULL ? "NULL" : p_->ToString();
  }

private:
  REF_COUNTED* p_;
};

//////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
template<typename T>
class InstanceCounterT {
public:
    explicit InstanceCounterT(const char* token) :
        token_(token),
        instances_(0) {
    }
    ~InstanceCounterT() {
        LOG_WARNING_IF(instances_ != 0) << token_ << " instances leaked " << instances_;
    }

public:
    void inc() {
        instances_.fetch_add(1);
    }
    void dec() {
        instances_.fetch_sub(1);
    }

private:
    const std::string token_;
    std::atomic_int instances_;
};
#else // NDEBUG
template<typename T>
class InstanceCounterT {
public:
    InstanceCounterT(const char* /*token*/)  {
    }
    ~InstanceCounterT() {
    }

public:
    void inc() {}
    void dec() {}
};
#endif // NDEBUG

#endif  // __WHISPERLIB_BASE_REF_COUNTED_H__
