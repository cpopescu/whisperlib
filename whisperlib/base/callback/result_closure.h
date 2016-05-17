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

#ifndef __WHISPERLIB_CALLBACK_BASE_RESULT_CLOSURE_H__
#define __WHISPERLIB_CALLBACK_BASE_RESULT_CLOSURE_H__

namespace whisper {

template<typename R>
class ResultClosure {
public:
  ResultClosure(bool is_permanent)
    : is_permanent_(is_permanent) {
  }
  virtual ~ResultClosure() {
  }
  R Run() {
    R ret = RunInternal();
    if ( !is_permanent_ )
      delete this;
    return ret;
  }
  bool is_permanent() const { return is_permanent_; }
protected:
  virtual R RunInternal() = 0;
private:
  bool is_permanent_;
};


/////////////////////////////////////////////////////////////////////

template<typename R>
class ResultClosure_0 : public ResultClosure<R> {
public:
  typedef R (*Fun)();
  ResultClosure_0(bool is_permanent, Fun fun)
    : ResultClosure<R>(is_permanent),
      fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (*fun_)();
  }
private:
  Fun fun_;
};
template<typename R>
ResultClosure_0<R>* NewCallback(R (*fun)()) {
  return new ResultClosure_0<R>(false, fun);
}
template<typename R>
ResultClosure_0<R>* NewPermanentCallback(R (*fun)()) {
  return new ResultClosure_0<R>(true, fun);
}


template<typename C, typename R>
class ResultMemberClosure_0 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)();
  ResultMemberClosure_0 (bool is_permanent, C* c, Fun fun)
    : ResultClosure<R>(is_permanent),
      c_(c),
      fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)();
  }
private:
  C* c_;
  Fun fun_;
};

template<typename C, typename R>
ResultMemberClosure_0<C, R>* NewCallback(C* c, R (C::*fun)()) {
  return new ResultMemberClosure_0<C, R>(false, c, fun);
}
template<typename C, typename R>
ResultMemberClosure_0<C, R>* NewPermanentCallback(C* c, R (C::*fun)()) {
  return new ResultMemberClosure_0<C, R>(true, c, fun);
}

template<typename C, typename R>
class ResultConstMemberClosure_0 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)() const;
  ResultConstMemberClosure_0 (bool is_permanent, const C* c, Fun fun)
    : ResultClosure<R>(is_permanent),
      c_(c),
      fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)();
  }
private:
  const C* c_;
  Fun fun_;
};

template<typename C, typename R>
ResultConstMemberClosure_0<C, R>* NewCallback(const C* c, R (C::*fun)() const) {
  return new ResultConstMemberClosure_0<C, R>(false, c, fun);
}
template<typename C, typename R>
ResultConstMemberClosure_0<C, R>* NewPermanentCallback(const C* c, R (C::*fun)() const) {
  return new ResultConstMemberClosure_0<C, R>(true, c, fun);
}

////////////////////////////////////////////////////////////////////////////////

template<typename R, typename T0>
class ResultClosure_1 : public ResultClosure<R> {
public:
  typedef R (*Fun)(T0);
  ResultClosure_1(bool is_permanent, Fun fun, T0 p0)
    : ResultClosure<R>(is_permanent),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (*fun_)(p0_);
  }
private:
  T0 p0_;
  Fun fun_;
};
template<typename R, typename T0>
ResultClosure_1<R, T0>* NewCallback(R (*fun)(T0), T0 p0) {
  return new ResultClosure_1<R, T0>(false, fun, p0);
}
template<typename R, typename T0>
ResultClosure_1<R, T0>* NewPermanentCallback(R (*fun)(T0), T0 p0) {
  return new ResultClosure_1<R, T0>(true, fun, p0);
}


template<typename C, typename R, typename T0>
class ResultConstMemberClosure_1 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0) const;
  ResultConstMemberClosure_1 (bool is_permanent, const C* c, Fun fun, T0 p0)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_);
  }
private:
  const C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0>
ResultConstMemberClosure_1<C, R, T0>* NewCallback(const C* c, R (C::*fun)(T0) const, T0 p0) {
  return new ResultConstMemberClosure_1<C, R, T0>(false, c, fun, p0);
}
template<typename C, typename R, typename T0>
ResultConstMemberClosure_1<C, R, T0>* NewPermanentCallback(const C* c, R (C::*fun)(T0) const, T0 p0) {
  return new ResultConstMemberClosure_1<C, R, T0>(true, c, fun, p0);
}



template<typename C, typename R, typename T0>
class ResultMemberClosure_1 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0);
  ResultMemberClosure_1 (bool is_permanent, C* c, Fun fun, T0 p0)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_);
  }
private:
  C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0>
ResultMemberClosure_1<C, R, T0>* NewCallback(C* c, R (C::*fun)(T0), T0 p0) {
  return new ResultMemberClosure_1<C, R, T0>(false, c, fun, p0);
}
template<typename C, typename R, typename T0>
ResultMemberClosure_1<C, R, T0>* NewPermanentCallback(C* c, R (C::*fun)(T0), T0 p0) {
  return new ResultMemberClosure_1<C, R, T0>(true, c, fun, p0);
}



template<typename R, typename T0, typename T1>
class ResultClosure_2 : public ResultClosure<R> {
public:
  typedef R (*Fun)(T0, T1);
  ResultClosure_2(bool is_permanent, Fun fun, T0 p0, T1 p1)
    : ResultClosure<R>(is_permanent),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (*fun_)(p0_, p1_);
  }
private:
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename R, typename T0, typename T1>
ResultClosure_2<R, T0, T1>* NewCallback(R (*fun)(T0, T1), T0 p0, T1 p1) {
  return new ResultClosure_2<R, T0, T1>(false, fun, p0, p1);
}
template<typename R, typename T0, typename T1>
ResultClosure_2<R, T0, T1>* NewPermanentCallback(R (*fun)(T0, T1), T0 p0, T1 p1) {
  return new ResultClosure_2<R, T0, T1>(true, fun, p0, p1);
}


template<typename C, typename R, typename T0, typename T1>
class ResultConstMemberClosure_2 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0, T1) const;
  ResultConstMemberClosure_2 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_, p1_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1>
ResultConstMemberClosure_2<C, R, T0, T1>* NewCallback(const C* c, R (C::*fun)(T0, T1) const, T0 p0, T1 p1) {
  return new ResultConstMemberClosure_2<C, R, T0, T1>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1>
ResultConstMemberClosure_2<C, R, T0, T1>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1) const, T0 p0, T1 p1) {
  return new ResultConstMemberClosure_2<C, R, T0, T1>(true, c, fun, p0, p1);
}



template<typename C, typename R, typename T0, typename T1>
class ResultMemberClosure_2 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0, T1);
  ResultMemberClosure_2 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_, p1_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1>
ResultMemberClosure_2<C, R, T0, T1>* NewCallback(C* c, R (C::*fun)(T0, T1), T0 p0, T1 p1) {
  return new ResultMemberClosure_2<C, R, T0, T1>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1>
ResultMemberClosure_2<C, R, T0, T1>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1), T0 p0, T1 p1) {
  return new ResultMemberClosure_2<C, R, T0, T1>(true, c, fun, p0, p1);
}



template<typename R, typename T0, typename T1, typename T2>
class ResultClosure_3 : public ResultClosure<R> {
public:
  typedef R (*Fun)(T0, T1, T2);
  ResultClosure_3(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultClosure<R>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (*fun_)(p0_, p1_, p2_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2>
ResultClosure_3<R, T0, T1, T2>* NewCallback(R (*fun)(T0, T1, T2), T0 p0, T1 p1, T2 p2) {
  return new ResultClosure_3<R, T0, T1, T2>(false, fun, p0, p1, p2);
}
template<typename R, typename T0, typename T1, typename T2>
ResultClosure_3<R, T0, T1, T2>* NewPermanentCallback(R (*fun)(T0, T1, T2), T0 p0, T1 p1, T2 p2) {
  return new ResultClosure_3<R, T0, T1, T2>(true, fun, p0, p1, p2);
}


template<typename C, typename R, typename T0, typename T1, typename T2>
class ResultConstMemberClosure_3 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0, T1, T2) const;
  ResultConstMemberClosure_3 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_, p1_, p2_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2>
ResultConstMemberClosure_3<C, R, T0, T1, T2>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberClosure_3<C, R, T0, T1, T2>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2>
ResultConstMemberClosure_3<C, R, T0, T1, T2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberClosure_3<C, R, T0, T1, T2>(true, c, fun, p0, p1, p2);
}



template<typename C, typename R, typename T0, typename T1, typename T2>
class ResultMemberClosure_3 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0, T1, T2);
  ResultMemberClosure_3 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_, p1_, p2_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2>
ResultMemberClosure_3<C, R, T0, T1, T2>* NewCallback(C* c, R (C::*fun)(T0, T1, T2), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberClosure_3<C, R, T0, T1, T2>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2>
ResultMemberClosure_3<C, R, T0, T1, T2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberClosure_3<C, R, T0, T1, T2>(true, c, fun, p0, p1, p2);
}



template<typename R, typename T0, typename T1, typename T2, typename T3>
class ResultClosure_4 : public ResultClosure<R> {
public:
  typedef R (*Fun)(T0, T1, T2, T3);
  ResultClosure_4(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultClosure<R>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (*fun_)(p0_, p1_, p2_, p3_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3>
ResultClosure_4<R, T0, T1, T2, T3>* NewCallback(R (*fun)(T0, T1, T2, T3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultClosure_4<R, T0, T1, T2, T3>(false, fun, p0, p1, p2, p3);
}
template<typename R, typename T0, typename T1, typename T2, typename T3>
ResultClosure_4<R, T0, T1, T2, T3>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultClosure_4<R, T0, T1, T2, T3>(true, fun, p0, p1, p2, p3);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3>
class ResultConstMemberClosure_4 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3) const;
  ResultConstMemberClosure_4 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_, p1_, p2_, p3_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3>
ResultConstMemberClosure_4<C, R, T0, T1, T2, T3>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberClosure_4<C, R, T0, T1, T2, T3>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3>
ResultConstMemberClosure_4<C, R, T0, T1, T2, T3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberClosure_4<C, R, T0, T1, T2, T3>(true, c, fun, p0, p1, p2, p3);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3>
class ResultMemberClosure_4 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3);
  ResultMemberClosure_4 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_, p1_, p2_, p3_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3>
ResultMemberClosure_4<C, R, T0, T1, T2, T3>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberClosure_4<C, R, T0, T1, T2, T3>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3>
ResultMemberClosure_4<C, R, T0, T1, T2, T3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberClosure_4<C, R, T0, T1, T2, T3>(true, c, fun, p0, p1, p2, p3);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
class ResultClosure_5 : public ResultClosure<R> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4);
  ResultClosure_5(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultClosure<R>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
ResultClosure_5<R, T0, T1, T2, T3, T4>* NewCallback(R (*fun)(T0, T1, T2, T3, T4), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultClosure_5<R, T0, T1, T2, T3, T4>(false, fun, p0, p1, p2, p3, p4);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
ResultClosure_5<R, T0, T1, T2, T3, T4>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultClosure_5<R, T0, T1, T2, T3, T4>(true, fun, p0, p1, p2, p3, p4);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
class ResultConstMemberClosure_5 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4) const;
  ResultConstMemberClosure_5 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
ResultConstMemberClosure_5<C, R, T0, T1, T2, T3, T4>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberClosure_5<C, R, T0, T1, T2, T3, T4>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
ResultConstMemberClosure_5<C, R, T0, T1, T2, T3, T4>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberClosure_5<C, R, T0, T1, T2, T3, T4>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
class ResultMemberClosure_5 : public ResultClosure<R> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4);
  ResultMemberClosure_5 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultClosure<R>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal() {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
ResultMemberClosure_5<C, R, T0, T1, T2, T3, T4>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberClosure_5<C, R, T0, T1, T2, T3, T4>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
ResultMemberClosure_5<C, R, T0, T1, T2, T3, T4>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberClosure_5<C, R, T0, T1, T2, T3, T4>(true, c, fun, p0, p1, p2, p3, p4);
}

}

#endif  // __WHISPERLIB_CALLBACK_BASE_RESULT_CLOSURE_H__
