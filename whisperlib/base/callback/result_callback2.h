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

#ifndef __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK2_H__
#define __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK2_H__

namespace whisper {

template<typename R, typename X0, typename X1>
class ResultCallback2 {
public:
  ResultCallback2(bool is_permanent)
    : is_permanent_(is_permanent) {
  }
  virtual ~ResultCallback2() {
  }
  R Run(X0 x0, X1 x1) {
    R ret = RunInternal(x0, x1);
    if ( !is_permanent_ )
      delete this;
    return ret;
  }
  bool is_permanent() const { return is_permanent_; }
protected:
  virtual R RunInternal(X0 x0, X1 x1) = 0;
private:
  bool is_permanent_;
};

//////////////////////////////////////////////////////////////////////

template<typename R, typename X0, typename X1>
class ResultCallback2_0 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (*Fun)(X0, X1);
  ResultCallback2_0(bool is_permanent, Fun fun)
    : ResultCallback2<R, X0, X1>(is_permanent),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (*fun_)(x0, x1);
  }
private:
  Fun fun_;
};
template<typename R, typename X0, typename X1>
ResultCallback2_0<R, X0, X1>* NewCallback(R (*fun)(X0, X1)) {
  return new ResultCallback2_0<R, X0, X1>(false, fun);
}
template<typename R, typename X0, typename X1>
ResultCallback2_0<R, X0, X1>* NewPermanentCallback(R (*fun)(X0, X1)) {
  return new ResultCallback2_0<R, X0, X1>(true, fun);
}


template<typename C, typename R, typename X0, typename X1>
class ResultConstMemberCallback2_0 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(X0, X1) const;
  ResultConstMemberCallback2_0 (bool is_permanent, const C* c, Fun fun)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(x0, x1);
  }
private:
  const C* c_;
  Fun fun_;
};

template<typename C, typename R, typename X0, typename X1>
ResultConstMemberCallback2_0<C, R, X0, X1>* NewCallback(const C* c, R (C::*fun)(X0, X1) const) {
  return new ResultConstMemberCallback2_0<C, R, X0, X1>(false, c, fun);
}
template<typename C, typename R, typename X0, typename X1>
ResultConstMemberCallback2_0<C, R, X0, X1>* NewPermanentCallback(const C* c, R (C::*fun)(X0, X1) const) {
  return new ResultConstMemberCallback2_0<C, R, X0, X1>(true, c, fun);
}



template<typename C, typename R, typename X0, typename X1>
class ResultMemberCallback2_0 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(X0, X1);
  ResultMemberCallback2_0 (bool is_permanent, C* c, Fun fun)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(x0, x1);
  }
private:
  C* c_;
  Fun fun_;
};

template<typename C, typename R, typename X0, typename X1>
ResultMemberCallback2_0<C, R, X0, X1>* NewCallback(C* c, R (C::*fun)(X0, X1)) {
  return new ResultMemberCallback2_0<C, R, X0, X1>(false, c, fun);
}
template<typename C, typename R, typename X0, typename X1>
ResultMemberCallback2_0<C, R, X0, X1>* NewPermanentCallback(C* c, R (C::*fun)(X0, X1)) {
  return new ResultMemberCallback2_0<C, R, X0, X1>(true, c, fun);
}

//////////////////////////////////////////////////////////////////////

template<typename R, typename T0, typename X0, typename X1>
class ResultCallback2_1 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (*Fun)(T0, X0, X1);
  ResultCallback2_1(bool is_permanent, Fun fun, T0 p0)
    : ResultCallback2<R, X0, X1>(is_permanent),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (*fun_)(p0_, x0, x1);
  }
private:
  T0 p0_;
  Fun fun_;
};
template<typename R, typename T0, typename X0, typename X1>
ResultCallback2_1<R, T0, X0, X1>* NewCallback(R (*fun)(T0, X0, X1), T0 p0) {
  return new ResultCallback2_1<R, T0, X0, X1>(false, fun, p0);
}
template<typename R, typename T0, typename X0, typename X1>
ResultCallback2_1<R, T0, X0, X1>* NewPermanentCallback(R (*fun)(T0, X0, X1), T0 p0) {
  return new ResultCallback2_1<R, T0, X0, X1>(true, fun, p0);
}


template<typename C, typename R, typename T0, typename X0, typename X1>
class ResultConstMemberCallback2_1 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, X0, X1) const;
  ResultConstMemberCallback2_1 (bool is_permanent, const C* c, Fun fun, T0 p0)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, x0, x1);
  }
private:
  const C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename X0, typename X1>
ResultConstMemberCallback2_1<C, R, T0, X0, X1>* NewCallback(const C* c, R (C::*fun)(T0, X0, X1) const, T0 p0) {
  return new ResultConstMemberCallback2_1<C, R, T0, X0, X1>(false, c, fun, p0);
}
template<typename C, typename R, typename T0, typename X0, typename X1>
ResultConstMemberCallback2_1<C, R, T0, X0, X1>* NewPermanentCallback(const C* c, R (C::*fun)(T0, X0, X1) const, T0 p0) {
  return new ResultConstMemberCallback2_1<C, R, T0, X0, X1>(true, c, fun, p0);
}

template<typename C, typename R, typename T0, typename X0, typename X1>
class ResultMemberCallback2_1 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, X0, X1);
  ResultMemberCallback2_1 (bool is_permanent, C* c, Fun fun, T0 p0)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, x0, x1);
  }
private:
  C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename X0, typename X1>
ResultMemberCallback2_1<C, R, T0, X0, X1>* NewCallback(C* c, R (C::*fun)(T0, X0, X1), T0 p0) {
  return new ResultMemberCallback2_1<C, R, T0, X0, X1>(false, c, fun, p0);
}
template<typename C, typename R, typename T0, typename X0, typename X1>
ResultMemberCallback2_1<C, R, T0, X0, X1>* NewPermanentCallback(C* c, R (C::*fun)(T0, X0, X1), T0 p0) {
  return new ResultMemberCallback2_1<C, R, T0, X0, X1>(true, c, fun, p0);
}

//////////////////////////////////////////////////////////////////////

template<typename R, typename T0, typename T1, typename X0, typename X1>
class ResultCallback2_2 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (*Fun)(T0, T1, X0, X1);
  ResultCallback2_2(bool is_permanent, Fun fun, T0 p0, T1 p1)
    : ResultCallback2<R, X0, X1>(is_permanent),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (*fun_)(p0_, p1_, x0, x1);
  }
private:
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename X0, typename X1>
ResultCallback2_2<R, T0, T1, X0, X1>* NewCallback(R (*fun)(T0, T1, X0, X1), T0 p0, T1 p1) {
  return new ResultCallback2_2<R, T0, T1, X0, X1>(false, fun, p0, p1);
}
template<typename R, typename T0, typename T1, typename X0, typename X1>
ResultCallback2_2<R, T0, T1, X0, X1>* NewPermanentCallback(R (*fun)(T0, T1, X0, X1), T0 p0, T1 p1) {
  return new ResultCallback2_2<R, T0, T1, X0, X1>(true, fun, p0, p1);
}


template<typename C, typename R, typename T0, typename T1, typename X0, typename X1>
class ResultConstMemberCallback2_2 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, T1, X0, X1) const;
  ResultConstMemberCallback2_2 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, p1_, x0, x1);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename X0, typename X1>
ResultConstMemberCallback2_2<C, R, T0, T1, X0, X1>* NewCallback(const C* c, R (C::*fun)(T0, T1, X0, X1) const, T0 p0, T1 p1) {
  return new ResultConstMemberCallback2_2<C, R, T0, T1, X0, X1>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1, typename X0, typename X1>
ResultConstMemberCallback2_2<C, R, T0, T1, X0, X1>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, X0, X1) const, T0 p0, T1 p1) {
  return new ResultConstMemberCallback2_2<C, R, T0, T1, X0, X1>(true, c, fun, p0, p1);
}



template<typename C, typename R, typename T0, typename T1, typename X0, typename X1>
class ResultMemberCallback2_2 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, T1, X0, X1);
  ResultMemberCallback2_2 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, p1_, x0, x1);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename X0, typename X1>
ResultMemberCallback2_2<C, R, T0, T1, X0, X1>* NewCallback(C* c, R (C::*fun)(T0, T1, X0, X1), T0 p0, T1 p1) {
  return new ResultMemberCallback2_2<C, R, T0, T1, X0, X1>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1, typename X0, typename X1>
ResultMemberCallback2_2<C, R, T0, T1, X0, X1>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, X0, X1), T0 p0, T1 p1) {
  return new ResultMemberCallback2_2<C, R, T0, T1, X0, X1>(true, c, fun, p0, p1);
}

//////////////////////////////////////////////////////////////////////

template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
class ResultCallback2_3 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (*Fun)(T0, T1, T2, X0, X1);
  ResultCallback2_3(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback2<R, X0, X1>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (*fun_)(p0_, p1_, p2_, x0, x1);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
ResultCallback2_3<R, T0, T1, T2, X0, X1>* NewCallback(R (*fun)(T0, T1, T2, X0, X1), T0 p0, T1 p1, T2 p2) {
  return new ResultCallback2_3<R, T0, T1, T2, X0, X1>(false, fun, p0, p1, p2);
}
template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
ResultCallback2_3<R, T0, T1, T2, X0, X1>* NewPermanentCallback(R (*fun)(T0, T1, T2, X0, X1), T0 p0, T1 p1, T2 p2) {
  return new ResultCallback2_3<R, T0, T1, T2, X0, X1>(true, fun, p0, p1, p2);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
class ResultConstMemberCallback2_3 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, T1, T2, X0, X1) const;
  ResultConstMemberCallback2_3 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, p1_, p2_, x0, x1);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
ResultConstMemberCallback2_3<C, R, T0, T1, T2, X0, X1>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, X0, X1) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberCallback2_3<C, R, T0, T1, T2, X0, X1>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
ResultConstMemberCallback2_3<C, R, T0, T1, T2, X0, X1>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, X0, X1) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberCallback2_3<C, R, T0, T1, T2, X0, X1>(true, c, fun, p0, p1, p2);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
class ResultMemberCallback2_3 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, T1, T2, X0, X1);
  ResultMemberCallback2_3 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, p1_, p2_, x0, x1);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
ResultMemberCallback2_3<C, R, T0, T1, T2, X0, X1>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, X0, X1), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberCallback2_3<C, R, T0, T1, T2, X0, X1>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1>
ResultMemberCallback2_3<C, R, T0, T1, T2, X0, X1>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, X0, X1), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberCallback2_3<C, R, T0, T1, T2, X0, X1>(true, c, fun, p0, p1, p2);
}

//////////////////////////////////////////////////////////////////////

template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
class ResultCallback2_4 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, X0, X1);
  ResultCallback2_4(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback2<R, X0, X1>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (*fun_)(p0_, p1_, p2_, p3_, x0, x1);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
ResultCallback2_4<R, T0, T1, T2, T3, X0, X1>* NewCallback(R (*fun)(T0, T1, T2, T3, X0, X1), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultCallback2_4<R, T0, T1, T2, T3, X0, X1>(false, fun, p0, p1, p2, p3);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
ResultCallback2_4<R, T0, T1, T2, T3, X0, X1>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, X0, X1), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultCallback2_4<R, T0, T1, T2, T3, X0, X1>(true, fun, p0, p1, p2, p3);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
class ResultConstMemberCallback2_4 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, X0, X1) const;
  ResultConstMemberCallback2_4 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, x0, x1);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
ResultConstMemberCallback2_4<C, R, T0, T1, T2, T3, X0, X1>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberCallback2_4<C, R, T0, T1, T2, T3, X0, X1>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
ResultConstMemberCallback2_4<C, R, T0, T1, T2, T3, X0, X1>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberCallback2_4<C, R, T0, T1, T2, T3, X0, X1>(true, c, fun, p0, p1, p2, p3);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
class ResultMemberCallback2_4 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, X0, X1);
  ResultMemberCallback2_4 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, x0, x1);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
ResultMemberCallback2_4<C, R, T0, T1, T2, T3, X0, X1>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberCallback2_4<C, R, T0, T1, T2, T3, X0, X1>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1>
ResultMemberCallback2_4<C, R, T0, T1, T2, T3, X0, X1>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberCallback2_4<C, R, T0, T1, T2, T3, X0, X1>(true, c, fun, p0, p1, p2, p3);
}

//////////////////////////////////////////////////////////////////////

template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
class ResultCallback2_5 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, X0, X1);
  ResultCallback2_5(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback2<R, X0, X1>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
ResultCallback2_5<R, T0, T1, T2, T3, T4, X0, X1>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, X0, X1), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultCallback2_5<R, T0, T1, T2, T3, T4, X0, X1>(false, fun, p0, p1, p2, p3, p4);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
ResultCallback2_5<R, T0, T1, T2, T3, T4, X0, X1>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, X0, X1), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultCallback2_5<R, T0, T1, T2, T3, T4, X0, X1>(true, fun, p0, p1, p2, p3, p4);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
class ResultConstMemberCallback2_5 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, X0, X1) const;
  ResultConstMemberCallback2_5 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
ResultConstMemberCallback2_5<C, R, T0, T1, T2, T3, T4, X0, X1>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberCallback2_5<C, R, T0, T1, T2, T3, T4, X0, X1>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
ResultConstMemberCallback2_5<C, R, T0, T1, T2, T3, T4, X0, X1>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberCallback2_5<C, R, T0, T1, T2, T3, T4, X0, X1>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
class ResultMemberCallback2_5 : public ResultCallback2<R, X0, X1> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, X0, X1);
  ResultMemberCallback2_5 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback2<R, X0, X1>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
ResultMemberCallback2_5<C, R, T0, T1, T2, T3, T4, X0, X1>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberCallback2_5<C, R, T0, T1, T2, T3, T4, X0, X1>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1>
ResultMemberCallback2_5<C, R, T0, T1, T2, T3, T4, X0, X1>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberCallback2_5<C, R, T0, T1, T2, T3, T4, X0, X1>(true, c, fun, p0, p1, p2, p3, p4);
}

}

#endif   // __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK2_H__
