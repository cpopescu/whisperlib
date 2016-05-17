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

#ifndef __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK3_H__
#define __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK3_H__

namespace whisper {

template<typename R, typename X0, typename X1, typename X2>
class ResultCallback3 {
public:
  ResultCallback3(bool is_permanent)
    : is_permanent_(is_permanent) {
  }
  virtual ~ResultCallback3() {
  }
  R Run(X0 x0, X1 x1, X2 x2) {
    R ret = RunInternal(x0, x1, x2);
    if ( !is_permanent_ )
      delete this;
    return ret;
  }
  bool is_permanent() const { return is_permanent_; }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) = 0;
private:
  bool is_permanent_;
};

//////////////////////////////////////////////////////////////////////

template<typename R, typename X0, typename X1, typename X2>
class ResultCallback3_0 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(X0, X1, X2);
  ResultCallback3_0(bool is_permanent, Fun fun)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(x0, x1, x2);
  }
private:
  Fun fun_;
};
template<typename R, typename X0, typename X1, typename X2>
ResultCallback3_0<R, X0, X1, X2>* NewCallback(R (*fun)(X0, X1, X2)) {
  return new ResultCallback3_0<R, X0, X1, X2>(false, fun);
}
template<typename R, typename X0, typename X1, typename X2>
ResultCallback3_0<R, X0, X1, X2>* NewPermanentCallback(R (*fun)(X0, X1, X2)) {
  return new ResultCallback3_0<R, X0, X1, X2>(true, fun);
}


template<typename C, typename R, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_0 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(X0, X1, X2) const;
  ResultConstMemberCallback3_0 (bool is_permanent, const C* c, Fun fun)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(x0, x1, x2);
  }
private:
  const C* c_;
  Fun fun_;
};

template<typename C, typename R, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_0<C, R, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(X0, X1, X2) const) {
  return new ResultConstMemberCallback3_0<C, R, X0, X1, X2>(false, c, fun);
}
template<typename C, typename R, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_0<C, R, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(X0, X1, X2) const) {
  return new ResultConstMemberCallback3_0<C, R, X0, X1, X2>(true, c, fun);
}



template<typename C, typename R, typename X0, typename X1, typename X2>
class ResultMemberCallback3_0 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(X0, X1, X2);
  ResultMemberCallback3_0 (bool is_permanent, C* c, Fun fun)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(x0, x1, x2);
  }
private:
  C* c_;
  Fun fun_;
};

template<typename C, typename R, typename X0, typename X1, typename X2>
ResultMemberCallback3_0<C, R, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(X0, X1, X2)) {
  return new ResultMemberCallback3_0<C, R, X0, X1, X2>(false, c, fun);
}
template<typename C, typename R, typename X0, typename X1, typename X2>
ResultMemberCallback3_0<C, R, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(X0, X1, X2)) {
  return new ResultMemberCallback3_0<C, R, X0, X1, X2>(true, c, fun);
}

//////////////////////////////////////////////////////////////////////
//
// Autogenerated callback classes (w/ print_callback.py)
//

template<typename R, typename T0, typename X0, typename X1, typename X2>
class ResultCallback3_1 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, X0, X1, X2);
  ResultCallback3_1(bool is_permanent, Fun fun, T0 p0)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, x0, x1, x2);
  }
private:
  T0 p0_;
  Fun fun_;
};
template<typename R, typename T0, typename X0, typename X1, typename X2>
ResultCallback3_1<R, T0, X0, X1, X2>* NewCallback(R (*fun)(T0, X0, X1, X2), T0 p0) {
  return new ResultCallback3_1<R, T0, X0, X1, X2>(false, fun, p0);
}
template<typename R, typename T0, typename X0, typename X1, typename X2>
ResultCallback3_1<R, T0, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, X0, X1, X2), T0 p0) {
  return new ResultCallback3_1<R, T0, X0, X1, X2>(true, fun, p0);
}


template<typename C, typename R, typename T0, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_1 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, X0, X1, X2) const;
  ResultConstMemberCallback3_1 (bool is_permanent, const C* c, Fun fun, T0 p0)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, x0, x1, x2);
  }
private:
  const C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_1<C, R, T0, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, X0, X1, X2) const, T0 p0) {
  return new ResultConstMemberCallback3_1<C, R, T0, X0, X1, X2>(false, c, fun, p0);
}
template<typename C, typename R, typename T0, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_1<C, R, T0, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, X0, X1, X2) const, T0 p0) {
  return new ResultConstMemberCallback3_1<C, R, T0, X0, X1, X2>(true, c, fun, p0);
}



template<typename C, typename R, typename T0, typename X0, typename X1, typename X2>
class ResultMemberCallback3_1 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, X0, X1, X2);
  ResultMemberCallback3_1 (bool is_permanent, C* c, Fun fun, T0 p0)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, x0, x1, x2);
  }
private:
  C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename X0, typename X1, typename X2>
ResultMemberCallback3_1<C, R, T0, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, X0, X1, X2), T0 p0) {
  return new ResultMemberCallback3_1<C, R, T0, X0, X1, X2>(false, c, fun, p0);
}
template<typename C, typename R, typename T0, typename X0, typename X1, typename X2>
ResultMemberCallback3_1<C, R, T0, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, X0, X1, X2), T0 p0) {
  return new ResultMemberCallback3_1<C, R, T0, X0, X1, X2>(true, c, fun, p0);
}



template<typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
class ResultCallback3_2 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, T1, X0, X1, X2);
  ResultCallback3_2(bool is_permanent, Fun fun, T0 p0, T1 p1)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, p1_, x0, x1, x2);
  }
private:
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
ResultCallback3_2<R, T0, T1, X0, X1, X2>* NewCallback(R (*fun)(T0, T1, X0, X1, X2), T0 p0, T1 p1) {
  return new ResultCallback3_2<R, T0, T1, X0, X1, X2>(false, fun, p0, p1);
}
template<typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
ResultCallback3_2<R, T0, T1, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, T1, X0, X1, X2), T0 p0, T1 p1) {
  return new ResultCallback3_2<R, T0, T1, X0, X1, X2>(true, fun, p0, p1);
}


template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_2 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, X0, X1, X2) const;
  ResultConstMemberCallback3_2 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, x0, x1, x2);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_2<C, R, T0, T1, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, T1, X0, X1, X2) const, T0 p0, T1 p1) {
  return new ResultConstMemberCallback3_2<C, R, T0, T1, X0, X1, X2>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_2<C, R, T0, T1, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, X0, X1, X2) const, T0 p0, T1 p1) {
  return new ResultConstMemberCallback3_2<C, R, T0, T1, X0, X1, X2>(true, c, fun, p0, p1);
}



template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
class ResultMemberCallback3_2 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, X0, X1, X2);
  ResultMemberCallback3_2 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, x0, x1, x2);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
ResultMemberCallback3_2<C, R, T0, T1, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, T1, X0, X1, X2), T0 p0, T1 p1) {
  return new ResultMemberCallback3_2<C, R, T0, T1, X0, X1, X2>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2>
ResultMemberCallback3_2<C, R, T0, T1, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, X0, X1, X2), T0 p0, T1 p1) {
  return new ResultMemberCallback3_2<C, R, T0, T1, X0, X1, X2>(true, c, fun, p0, p1);
}



template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
class ResultCallback3_3 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, T1, T2, X0, X1, X2);
  ResultCallback3_3(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, p1_, p2_, x0, x1, x2);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
ResultCallback3_3<R, T0, T1, T2, X0, X1, X2>* NewCallback(R (*fun)(T0, T1, T2, X0, X1, X2), T0 p0, T1 p1, T2 p2) {
  return new ResultCallback3_3<R, T0, T1, T2, X0, X1, X2>(false, fun, p0, p1, p2);
}
template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
ResultCallback3_3<R, T0, T1, T2, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, T1, T2, X0, X1, X2), T0 p0, T1 p1, T2 p2) {
  return new ResultCallback3_3<R, T0, T1, T2, X0, X1, X2>(true, fun, p0, p1, p2);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_3 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, X0, X1, X2) const;
  ResultConstMemberCallback3_3 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, x0, x1, x2);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_3<C, R, T0, T1, T2, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, X0, X1, X2) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberCallback3_3<C, R, T0, T1, T2, X0, X1, X2>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_3<C, R, T0, T1, T2, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, X0, X1, X2) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberCallback3_3<C, R, T0, T1, T2, X0, X1, X2>(true, c, fun, p0, p1, p2);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
class ResultMemberCallback3_3 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, X0, X1, X2);
  ResultMemberCallback3_3 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, x0, x1, x2);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
ResultMemberCallback3_3<C, R, T0, T1, T2, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, X0, X1, X2), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberCallback3_3<C, R, T0, T1, T2, X0, X1, X2>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2>
ResultMemberCallback3_3<C, R, T0, T1, T2, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, X0, X1, X2), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberCallback3_3<C, R, T0, T1, T2, X0, X1, X2>(true, c, fun, p0, p1, p2);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
class ResultCallback3_4 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, X0, X1, X2);
  ResultCallback3_4(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
ResultCallback3_4<R, T0, T1, T2, T3, X0, X1, X2>* NewCallback(R (*fun)(T0, T1, T2, T3, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultCallback3_4<R, T0, T1, T2, T3, X0, X1, X2>(false, fun, p0, p1, p2, p3);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
ResultCallback3_4<R, T0, T1, T2, T3, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultCallback3_4<R, T0, T1, T2, T3, X0, X1, X2>(true, fun, p0, p1, p2, p3);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_4 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, X0, X1, X2) const;
  ResultConstMemberCallback3_4 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_4<C, R, T0, T1, T2, T3, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberCallback3_4<C, R, T0, T1, T2, T3, X0, X1, X2>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_4<C, R, T0, T1, T2, T3, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberCallback3_4<C, R, T0, T1, T2, T3, X0, X1, X2>(true, c, fun, p0, p1, p2, p3);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
class ResultMemberCallback3_4 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, X0, X1, X2);
  ResultMemberCallback3_4 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
ResultMemberCallback3_4<C, R, T0, T1, T2, T3, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberCallback3_4<C, R, T0, T1, T2, T3, X0, X1, X2>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2>
ResultMemberCallback3_4<C, R, T0, T1, T2, T3, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberCallback3_4<C, R, T0, T1, T2, T3, X0, X1, X2>(true, c, fun, p0, p1, p2, p3);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
class ResultCallback3_5 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, X0, X1, X2);
  ResultCallback3_5(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
ResultCallback3_5<R, T0, T1, T2, T3, T4, X0, X1, X2>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultCallback3_5<R, T0, T1, T2, T3, T4, X0, X1, X2>(false, fun, p0, p1, p2, p3, p4);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
ResultCallback3_5<R, T0, T1, T2, T3, T4, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultCallback3_5<R, T0, T1, T2, T3, T4, X0, X1, X2>(true, fun, p0, p1, p2, p3, p4);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_5 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, X0, X1, X2) const;
  ResultConstMemberCallback3_5 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberCallback3_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberCallback3_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
class ResultMemberCallback3_5 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, X0, X1, X2);
  ResultMemberCallback3_5 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
ResultMemberCallback3_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberCallback3_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2>
ResultMemberCallback3_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberCallback3_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
class ResultCallback3_6 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2);
  ResultCallback3_6(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
ResultCallback3_6<R, T0, T1, T2, T3, T4, T5, X0, X1, X2>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultCallback3_6<R, T0, T1, T2, T3, T4, T5, X0, X1, X2>(false, fun, p0, p1, p2, p3, p4, p5);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
ResultCallback3_6<R, T0, T1, T2, T3, T4, T5, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultCallback3_6<R, T0, T1, T2, T3, T4, T5, X0, X1, X2>(true, fun, p0, p1, p2, p3, p4, p5);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_6 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2) const;
  ResultConstMemberCallback3_6 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultConstMemberCallback3_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4, p5);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultConstMemberCallback3_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4, p5);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
class ResultMemberCallback3_6 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2);
  ResultMemberCallback3_6 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
ResultMemberCallback3_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultMemberCallback3_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4, p5);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2>
ResultMemberCallback3_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultMemberCallback3_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4, p5);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
class ResultCallback3_7 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2);
  ResultCallback3_7(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
ResultCallback3_7<R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultCallback3_7<R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>(false, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
ResultCallback3_7<R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultCallback3_7<R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>(true, fun, p0, p1, p2, p3, p4, p5, p6);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_7 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2) const;
  ResultConstMemberCallback3_7 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultConstMemberCallback3_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultConstMemberCallback3_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4, p5, p6);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
class ResultMemberCallback3_7 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2);
  ResultMemberCallback3_7 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
ResultMemberCallback3_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultMemberCallback3_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2>
ResultMemberCallback3_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultMemberCallback3_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4, p5, p6);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
class ResultCallback3_8 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2);
  ResultCallback3_8(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
ResultCallback3_8<R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultCallback3_8<R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>(false, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
ResultCallback3_8<R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultCallback3_8<R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>(true, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_8 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2) const;
  ResultConstMemberCallback3_8 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultConstMemberCallback3_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultConstMemberCallback3_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
class ResultMemberCallback3_8 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2);
  ResultMemberCallback3_8 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
ResultMemberCallback3_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultMemberCallback3_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2>
ResultMemberCallback3_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultMemberCallback3_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
class ResultCallback3_9 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2);
  ResultCallback3_9(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
p8_(p8),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
T8 p8_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
ResultCallback3_9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultCallback3_9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>(false, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
ResultCallback3_9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultCallback3_9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>(true, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
class ResultConstMemberCallback3_9 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2) const;
  ResultConstMemberCallback3_9 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
p8_(p8),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
T8 p8_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultConstMemberCallback3_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
ResultConstMemberCallback3_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultConstMemberCallback3_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
class ResultMemberCallback3_9 : public ResultCallback3<R, X0, X1, X2> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2);
  ResultMemberCallback3_9 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : ResultCallback3<R, X0, X1, X2>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
p8_(p8),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
T8 p8_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
ResultMemberCallback3_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultMemberCallback3_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2>
ResultMemberCallback3_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultMemberCallback3_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}

}

#endif   // __WHISPERLIB_BASE_CALLBACK_RESULT_CALLBACK3_H__
