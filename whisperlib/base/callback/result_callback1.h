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

#ifndef __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK1_H__
#define __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK1_H__

namespace whisper {

template<typename R, typename X0>
class ResultCallback1 {
public:
  ResultCallback1(bool is_permanent)
    : is_permanent_(is_permanent) {
  }
  virtual ~ResultCallback1() {
  }
  R Run(X0 x0) {
    R ret = RunInternal(x0);
    if ( !is_permanent_ )
      delete this;
    return ret;
  }
  bool is_permanent() const { return is_permanent_; }
protected:
  virtual R RunInternal(X0 x0) = 0;
private:
  bool is_permanent_;
};


/////////////////////////////////////////////////////////////////////

template<typename R, typename X0>
class ResultCallback1_0 : public ResultCallback1<R, X0> {
public:
  typedef R (*Fun)(X0);
  ResultCallback1_0(bool is_permanent, Fun fun)
    : ResultCallback1<R, X0>(is_permanent),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (*fun_)(x0);
  }
private:
  Fun fun_;
};
template<typename R, typename X0>
ResultCallback1_0<R, X0>* NewCallback(R (*fun)(X0)) {
  return new ResultCallback1_0<R, X0>(false, fun);
}
template<typename R, typename X0>
ResultCallback1_0<R, X0>* NewPermanentCallback(R (*fun)(X0)) {
  return new ResultCallback1_0<R, X0>(true, fun);
}


template<typename C, typename R, typename X0>
class ResultMemberCallback1_0 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(X0);
  ResultMemberCallback1_0 (bool is_permanent, C* c, Fun fun)
    : ResultCallback1<R, X0>(is_permanent),
      c_(c),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(x0);
  }
private:
  C* c_;
  Fun fun_;
};

template<typename C, typename R, typename X0>
ResultMemberCallback1_0<C, R, X0>* NewCallback(C* c, R (C::*fun)(X0)) {
  return new ResultMemberCallback1_0<C, R, X0>(false, c, fun);
}
template<typename C, typename R, typename X0>
ResultMemberCallback1_0<C, R, X0>* NewPermanentCallback(C* c, R (C::*fun)(X0)) {
  return new ResultMemberCallback1_0<C, R, X0>(true, c, fun);
}

template<typename C, typename R, typename X0>
class ResultConstMemberCallback1_0 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(X0) const;
  ResultConstMemberCallback1_0 (bool is_permanent, const C* c, Fun fun)
    : ResultCallback1<R, X0>(is_permanent),
      c_(c),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(x0);
  }
private:
  const C* c_;
  Fun fun_;
};

template<typename C, typename R, typename X0>
ResultConstMemberCallback1_0<C, R, X0>* NewCallback(const C* c, R (C::*fun)(X0) const) {
  return new ResultConstMemberCallback1_0<C, R, X0>(false, c, fun);
}
template<typename C, typename R, typename X0>
ResultConstMemberCallback1_0<C, R, X0>* NewPermanentCallback(const C* c, R (C::*fun)(X0) const) {
  return new ResultConstMemberCallback1_0<C, R, X0>(true, c, fun);
}

////////////////////////////////////////////////////////////////////////////////

template<typename R, typename T0, typename X0>
class ResultCallback1_1 : public ResultCallback1<R, X0> {
public:
  typedef R (*Fun)(T0, X0);
  ResultCallback1_1(bool is_permanent, Fun fun, T0 p0)
    : ResultCallback1<R, X0>(is_permanent),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (*fun_)(p0_, x0);
  }
private:
  T0 p0_;
  Fun fun_;
};
template<typename R, typename T0, typename X0>
ResultCallback1_1<R, T0, X0>* NewCallback(R (*fun)(T0, X0), T0 p0) {
  return new ResultCallback1_1<R, T0, X0>(false, fun, p0);
}
template<typename R, typename T0, typename X0>
ResultCallback1_1<R, T0, X0>* NewPermanentCallback(R (*fun)(T0, X0), T0 p0) {
  return new ResultCallback1_1<R, T0, X0>(true, fun, p0);
}


template<typename C, typename R, typename T0, typename X0>
class ResultConstMemberCallback1_1 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, X0) const;
  ResultConstMemberCallback1_1 (bool is_permanent, const C* c, Fun fun, T0 p0)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, x0);
  }
private:
  const C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename X0>
ResultConstMemberCallback1_1<C, R, T0, X0>* NewCallback(const C* c, R (C::*fun)(T0, X0) const, T0 p0) {
  return new ResultConstMemberCallback1_1<C, R, T0, X0>(false, c, fun, p0);
}
template<typename C, typename R, typename T0, typename X0>
ResultConstMemberCallback1_1<C, R, T0, X0>* NewPermanentCallback(const C* c, R (C::*fun)(T0, X0) const, T0 p0) {
  return new ResultConstMemberCallback1_1<C, R, T0, X0>(true, c, fun, p0);
}



template<typename C, typename R, typename T0, typename X0>
class ResultMemberCallback1_1 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, X0);
  ResultMemberCallback1_1 (bool is_permanent, C* c, Fun fun, T0 p0)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, x0);
  }
private:
  C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename X0>
ResultMemberCallback1_1<C, R, T0, X0>* NewCallback(C* c, R (C::*fun)(T0, X0), T0 p0) {
  return new ResultMemberCallback1_1<C, R, T0, X0>(false, c, fun, p0);
}
template<typename C, typename R, typename T0, typename X0>
ResultMemberCallback1_1<C, R, T0, X0>* NewPermanentCallback(C* c, R (C::*fun)(T0, X0), T0 p0) {
  return new ResultMemberCallback1_1<C, R, T0, X0>(true, c, fun, p0);
}



template<typename R, typename T0, typename T1, typename X0>
class ResultCallback1_2 : public ResultCallback1<R, X0> {
public:
  typedef R (*Fun)(T0, T1, X0);
  ResultCallback1_2(bool is_permanent, Fun fun, T0 p0, T1 p1)
    : ResultCallback1<R, X0>(is_permanent),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (*fun_)(p0_, p1_, x0);
  }
private:
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename X0>
ResultCallback1_2<R, T0, T1, X0>* NewCallback(R (*fun)(T0, T1, X0), T0 p0, T1 p1) {
  return new ResultCallback1_2<R, T0, T1, X0>(false, fun, p0, p1);
}
template<typename R, typename T0, typename T1, typename X0>
ResultCallback1_2<R, T0, T1, X0>* NewPermanentCallback(R (*fun)(T0, T1, X0), T0 p0, T1 p1) {
  return new ResultCallback1_2<R, T0, T1, X0>(true, fun, p0, p1);
}


template<typename C, typename R, typename T0, typename T1, typename X0>
class ResultConstMemberCallback1_2 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, T1, X0) const;
  ResultConstMemberCallback1_2 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, p1_, x0);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename X0>
ResultConstMemberCallback1_2<C, R, T0, T1, X0>* NewCallback(const C* c, R (C::*fun)(T0, T1, X0) const, T0 p0, T1 p1) {
  return new ResultConstMemberCallback1_2<C, R, T0, T1, X0>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1, typename X0>
ResultConstMemberCallback1_2<C, R, T0, T1, X0>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, X0) const, T0 p0, T1 p1) {
  return new ResultConstMemberCallback1_2<C, R, T0, T1, X0>(true, c, fun, p0, p1);
}



template<typename C, typename R, typename T0, typename T1, typename X0>
class ResultMemberCallback1_2 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, T1, X0);
  ResultMemberCallback1_2 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, p1_, x0);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename X0>
ResultMemberCallback1_2<C, R, T0, T1, X0>* NewCallback(C* c, R (C::*fun)(T0, T1, X0), T0 p0, T1 p1) {
  return new ResultMemberCallback1_2<C, R, T0, T1, X0>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1, typename X0>
ResultMemberCallback1_2<C, R, T0, T1, X0>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, X0), T0 p0, T1 p1) {
  return new ResultMemberCallback1_2<C, R, T0, T1, X0>(true, c, fun, p0, p1);
}



template<typename R, typename T0, typename T1, typename T2, typename X0>
class ResultCallback1_3 : public ResultCallback1<R, X0> {
public:
  typedef R (*Fun)(T0, T1, T2, X0);
  ResultCallback1_3(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback1<R, X0>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (*fun_)(p0_, p1_, p2_, x0);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename X0>
ResultCallback1_3<R, T0, T1, T2, X0>* NewCallback(R (*fun)(T0, T1, T2, X0), T0 p0, T1 p1, T2 p2) {
  return new ResultCallback1_3<R, T0, T1, T2, X0>(false, fun, p0, p1, p2);
}
template<typename R, typename T0, typename T1, typename T2, typename X0>
ResultCallback1_3<R, T0, T1, T2, X0>* NewPermanentCallback(R (*fun)(T0, T1, T2, X0), T0 p0, T1 p1, T2 p2) {
  return new ResultCallback1_3<R, T0, T1, T2, X0>(true, fun, p0, p1, p2);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename X0>
class ResultConstMemberCallback1_3 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, T1, T2, X0) const;
  ResultConstMemberCallback1_3 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, p1_, p2_, x0);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename X0>
ResultConstMemberCallback1_3<C, R, T0, T1, T2, X0>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, X0) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberCallback1_3<C, R, T0, T1, T2, X0>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename X0>
ResultConstMemberCallback1_3<C, R, T0, T1, T2, X0>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, X0) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberCallback1_3<C, R, T0, T1, T2, X0>(true, c, fun, p0, p1, p2);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename X0>
class ResultMemberCallback1_3 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, T1, T2, X0);
  ResultMemberCallback1_3 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, p1_, p2_, x0);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename X0>
ResultMemberCallback1_3<C, R, T0, T1, T2, X0>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, X0), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberCallback1_3<C, R, T0, T1, T2, X0>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename X0>
ResultMemberCallback1_3<C, R, T0, T1, T2, X0>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, X0), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberCallback1_3<C, R, T0, T1, T2, X0>(true, c, fun, p0, p1, p2);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
class ResultCallback1_4 : public ResultCallback1<R, X0> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, X0);
  ResultCallback1_4(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback1<R, X0>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (*fun_)(p0_, p1_, p2_, p3_, x0);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
ResultCallback1_4<R, T0, T1, T2, T3, X0>* NewCallback(R (*fun)(T0, T1, T2, T3, X0), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultCallback1_4<R, T0, T1, T2, T3, X0>(false, fun, p0, p1, p2, p3);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
ResultCallback1_4<R, T0, T1, T2, T3, X0>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, X0), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultCallback1_4<R, T0, T1, T2, T3, X0>(true, fun, p0, p1, p2, p3);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
class ResultConstMemberCallback1_4 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, X0) const;
  ResultConstMemberCallback1_4 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, x0);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
ResultConstMemberCallback1_4<C, R, T0, T1, T2, T3, X0>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, X0) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberCallback1_4<C, R, T0, T1, T2, T3, X0>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
ResultConstMemberCallback1_4<C, R, T0, T1, T2, T3, X0>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, X0) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberCallback1_4<C, R, T0, T1, T2, T3, X0>(true, c, fun, p0, p1, p2, p3);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
class ResultMemberCallback1_4 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, X0);
  ResultMemberCallback1_4 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, x0);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
ResultMemberCallback1_4<C, R, T0, T1, T2, T3, X0>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, X0), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberCallback1_4<C, R, T0, T1, T2, T3, X0>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0>
ResultMemberCallback1_4<C, R, T0, T1, T2, T3, X0>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, X0), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberCallback1_4<C, R, T0, T1, T2, T3, X0>(true, c, fun, p0, p1, p2, p3);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
class ResultCallback1_5 : public ResultCallback1<R, X0> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, X0);
  ResultCallback1_5(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback1<R, X0>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, x0);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
ResultCallback1_5<R, T0, T1, T2, T3, T4, X0>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, X0), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultCallback1_5<R, T0, T1, T2, T3, T4, X0>(false, fun, p0, p1, p2, p3, p4);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
ResultCallback1_5<R, T0, T1, T2, T3, T4, X0>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, X0), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultCallback1_5<R, T0, T1, T2, T3, T4, X0>(true, fun, p0, p1, p2, p3, p4);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
class ResultConstMemberCallback1_5 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, X0) const;
  ResultConstMemberCallback1_5 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
ResultConstMemberCallback1_5<C, R, T0, T1, T2, T3, T4, X0>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberCallback1_5<C, R, T0, T1, T2, T3, T4, X0>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
ResultConstMemberCallback1_5<C, R, T0, T1, T2, T3, T4, X0>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberCallback1_5<C, R, T0, T1, T2, T3, T4, X0>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
class ResultMemberCallback1_5 : public ResultCallback1<R, X0> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, X0);
  ResultMemberCallback1_5 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback1<R, X0>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
ResultMemberCallback1_5<C, R, T0, T1, T2, T3, T4, X0>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberCallback1_5<C, R, T0, T1, T2, T3, T4, X0>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0>
ResultMemberCallback1_5<C, R, T0, T1, T2, T3, T4, X0>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberCallback1_5<C, R, T0, T1, T2, T3, T4, X0>(true, c, fun, p0, p1, p2, p3, p4);
}

}

#endif   // __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK1_H__
