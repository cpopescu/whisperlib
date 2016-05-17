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

#ifndef __WHISPERLIB_BASE_CALLBACK_CALLBACK4_H__
#define __WHISPERLIB_BASE_CALLBACK_CALLBACK4_H__

#include "whisperlib/base/callback/callback.h"

namespace whisper {

template<typename X0, typename X1, typename X2, typename X3>
class Callback4 : public Callback {
public:
  Callback4(bool is_permanent)
    : Callback(),
      is_permanent_(is_permanent) {
  }
  virtual ~Callback4() {
  }
  void Run(X0 x0, X1 x1, X2 x2, X3 x3) {
    const bool permanent = is_permanent();
    RunInternal(x0, x1, x2, x3);
    if ( !permanent ) {
      delete this;
    }
  }
  bool is_permanent() const { return is_permanent_; }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) = 0;
private:
  const bool is_permanent_;
};

template<typename R, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4 {
public:
  ResultCallback4(bool is_permanent)
    : is_permanent_(is_permanent) {
  }
  virtual ~ResultCallback4() {
  }
  R Run(X0 x0, X1 x1, X2 x2, X3 x3) {
    R ret = RunInternal(x0, x1, x2, x3);
    if ( !is_permanent_ )
      delete this;
    return ret;
  }
  bool is_permanent() const { return is_permanent_; }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) = 0;
private:
  bool is_permanent_;
};

//////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
template<typename X0, typename X1, typename X2, typename X3>
class Callback4_0 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(X0, X1, X2, X3);
  Callback4_0(bool is_permanent, Fun fun)
    : Callback4<X0, X1, X2, X3>(is_permanent),

      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(x0, x1, x2, x3);
  }
private:

  Fun fun_;
};
template<typename X0, typename X1, typename X2, typename X3>
Callback4_0<X0, X1, X2, X3>* NewCallback(void (*fun)(X0, X1, X2, X3)) {
  return new Callback4_0<X0, X1, X2, X3>(false, fun);
}
template<typename X0, typename X1, typename X2, typename X3>
Callback4_0<X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(X0, X1, X2, X3)) {
  return new Callback4_0<X0, X1, X2, X3>(true, fun);
}


template<typename C, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_0 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(X0, X1, X2, X3) const;
  ConstMemberCallback4_0 (bool is_permanent, const C* c, Fun fun)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),

      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(x0, x1, x2, x3);
  }
private:
  const C* c_;

  Fun fun_;
};

template<typename C, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_0<C, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(X0, X1, X2, X3) const) {
  return new ConstMemberCallback4_0<C, X0, X1, X2, X3>(false, c, fun);
}
template<typename C, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_0<C, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(X0, X1, X2, X3) const) {
  return new ConstMemberCallback4_0<C, X0, X1, X2, X3>(true, c, fun);
}



template<typename C, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_0 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(X0, X1, X2, X3);
  MemberCallback4_0 (bool is_permanent, C* c, Fun fun)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),

      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(x0, x1, x2, x3);
  }
private:
  C* c_;

  Fun fun_;
};

template<typename C, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_0<C, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(X0, X1, X2, X3)) {
  return new MemberCallback4_0<C, X0, X1, X2, X3>(false, c, fun);
}
template<typename C, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_0<C, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(X0, X1, X2, X3)) {
  return new MemberCallback4_0<C, X0, X1, X2, X3>(true, c, fun);
}



template<typename T0, typename X0, typename X1, typename X2, typename X3>
class Callback4_1 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, X0, X1, X2, X3);
  Callback4_1(bool is_permanent, Fun fun, T0 p0)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
  Fun fun_;
};
template<typename T0, typename X0, typename X1, typename X2, typename X3>
Callback4_1<T0, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, X0, X1, X2, X3), T0 p0) {
  return new Callback4_1<T0, X0, X1, X2, X3>(false, fun, p0);
}
template<typename T0, typename X0, typename X1, typename X2, typename X3>
Callback4_1<T0, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, X0, X1, X2, X3), T0 p0) {
  return new Callback4_1<T0, X0, X1, X2, X3>(true, fun, p0);
}


template<typename C, typename T0, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_1 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, X0, X1, X2, X3) const;
  ConstMemberCallback4_1 (bool is_permanent, const C* c, Fun fun, T0 p0)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, x0, x1, x2, x3);
  }
private:
  const C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename T0, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_1<C, T0, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, X0, X1, X2, X3) const, T0 p0) {
  return new ConstMemberCallback4_1<C, T0, X0, X1, X2, X3>(false, c, fun, p0);
}
template<typename C, typename T0, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_1<C, T0, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, X0, X1, X2, X3) const, T0 p0) {
  return new ConstMemberCallback4_1<C, T0, X0, X1, X2, X3>(true, c, fun, p0);
}



template<typename C, typename T0, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_1 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, X0, X1, X2, X3);
  MemberCallback4_1 (bool is_permanent, C* c, Fun fun, T0 p0)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, x0, x1, x2, x3);
  }
private:
  C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename T0, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_1<C, T0, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, X0, X1, X2, X3), T0 p0) {
  return new MemberCallback4_1<C, T0, X0, X1, X2, X3>(false, c, fun, p0);
}
template<typename C, typename T0, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_1<C, T0, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, X0, X1, X2, X3), T0 p0) {
  return new MemberCallback4_1<C, T0, X0, X1, X2, X3>(true, c, fun, p0);
}



template<typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
class Callback4_2 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, T1, X0, X1, X2, X3);
  Callback4_2(bool is_permanent, Fun fun, T0 p0, T1 p1)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, p1_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
Callback4_2<T0, T1, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, T1, X0, X1, X2, X3), T0 p0, T1 p1) {
  return new Callback4_2<T0, T1, X0, X1, X2, X3>(false, fun, p0, p1);
}
template<typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
Callback4_2<T0, T1, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, T1, X0, X1, X2, X3), T0 p0, T1 p1) {
  return new Callback4_2<T0, T1, X0, X1, X2, X3>(true, fun, p0, p1);
}


template<typename C, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_2 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, X0, X1, X2, X3) const;
  ConstMemberCallback4_2 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, x0, x1, x2, x3);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_2<C, T0, T1, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, T1, X0, X1, X2, X3) const, T0 p0, T1 p1) {
  return new ConstMemberCallback4_2<C, T0, T1, X0, X1, X2, X3>(false, c, fun, p0, p1);
}
template<typename C, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_2<C, T0, T1, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, T1, X0, X1, X2, X3) const, T0 p0, T1 p1) {
  return new ConstMemberCallback4_2<C, T0, T1, X0, X1, X2, X3>(true, c, fun, p0, p1);
}



template<typename C, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_2 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, X0, X1, X2, X3);
  MemberCallback4_2 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, x0, x1, x2, x3);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_2<C, T0, T1, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, T1, X0, X1, X2, X3), T0 p0, T1 p1) {
  return new MemberCallback4_2<C, T0, T1, X0, X1, X2, X3>(false, c, fun, p0, p1);
}
template<typename C, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_2<C, T0, T1, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, X0, X1, X2, X3), T0 p0, T1 p1) {
  return new MemberCallback4_2<C, T0, T1, X0, X1, X2, X3>(true, c, fun, p0, p1);
}



template<typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
class Callback4_3 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, T1, T2, X0, X1, X2, X3);
  Callback4_3(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, p1_, p2_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
Callback4_3<T0, T1, T2, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, T1, T2, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2) {
  return new Callback4_3<T0, T1, T2, X0, X1, X2, X3>(false, fun, p0, p1, p2);
}
template<typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
Callback4_3<T0, T1, T2, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, T1, T2, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2) {
  return new Callback4_3<T0, T1, T2, X0, X1, X2, X3>(true, fun, p0, p1, p2);
}


template<typename C, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_3 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, X0, X1, X2, X3) const;
  ConstMemberCallback4_3 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, x0, x1, x2, x3);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_3<C, T0, T1, T2, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2) {
  return new ConstMemberCallback4_3<C, T0, T1, T2, X0, X1, X2, X3>(false, c, fun, p0, p1, p2);
}
template<typename C, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_3<C, T0, T1, T2, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, T1, T2, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2) {
  return new ConstMemberCallback4_3<C, T0, T1, T2, X0, X1, X2, X3>(true, c, fun, p0, p1, p2);
}



template<typename C, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_3 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, X0, X1, X2, X3);
  MemberCallback4_3 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, x0, x1, x2, x3);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_3<C, T0, T1, T2, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2) {
  return new MemberCallback4_3<C, T0, T1, T2, X0, X1, X2, X3>(false, c, fun, p0, p1, p2);
}
template<typename C, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_3<C, T0, T1, T2, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2) {
  return new MemberCallback4_3<C, T0, T1, T2, X0, X1, X2, X3>(true, c, fun, p0, p1, p2);
}



template<typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
class Callback4_4 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, T1, T2, T3, X0, X1, X2, X3);
  Callback4_4(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
Callback4_4<T0, T1, T2, T3, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, T1, T2, T3, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new Callback4_4<T0, T1, T2, T3, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3);
}
template<typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
Callback4_4<T0, T1, T2, T3, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new Callback4_4<T0, T1, T2, T3, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_4 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, X0, X1, X2, X3) const;
  ConstMemberCallback4_4 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2, x3);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_4<C, T0, T1, T2, T3, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ConstMemberCallback4_4<C, T0, T1, T2, T3, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_4<C, T0, T1, T2, T3, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ConstMemberCallback4_4<C, T0, T1, T2, T3, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3);
}



template<typename C, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_4 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, X0, X1, X2, X3);
  MemberCallback4_4 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2, x3);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_4<C, T0, T1, T2, T3, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new MemberCallback4_4<C, T0, T1, T2, T3, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_4<C, T0, T1, T2, T3, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new MemberCallback4_4<C, T0, T1, T2, T3, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3);
}



template<typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
class Callback4_5 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3);
  Callback4_5(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
Callback4_5<T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new Callback4_5<T0, T1, T2, T3, T4, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
Callback4_5<T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new Callback4_5<T0, T1, T2, T3, T4, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_5 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3) const;
  ConstMemberCallback4_5 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_5<C, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ConstMemberCallback4_5<C, T0, T1, T2, T3, T4, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_5<C, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ConstMemberCallback4_5<C, T0, T1, T2, T3, T4, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_5 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3);
  MemberCallback4_5 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_5<C, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new MemberCallback4_5<C, T0, T1, T2, T3, T4, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_5<C, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new MemberCallback4_5<C, T0, T1, T2, T3, T4, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
class Callback4_6 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3);
  Callback4_6(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : Callback4<X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
      fun_(fun) {
  }
protected:
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2, x3);
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
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
Callback4_6<T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new Callback4_6<T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4, p5);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
Callback4_6<T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new Callback4_6<T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4, p5);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_6 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3) const;
  ConstMemberCallback4_6 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_6<C, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ConstMemberCallback4_6<C, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_6<C, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ConstMemberCallback4_6<C, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5);
}



template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_6 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3);
  MemberCallback4_6 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_6<C, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new MemberCallback4_6<C, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_6<C, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new MemberCallback4_6<C, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5);
}



template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
class Callback4_7 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3);
  Callback4_7(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2, x3);
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
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
Callback4_7<T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new Callback4_7<T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
Callback4_7<T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new Callback4_7<T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4, p5, p6);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_7 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3) const;
  ConstMemberCallback4_7 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_7<C, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ConstMemberCallback4_7<C, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_7<C, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ConstMemberCallback4_7<C, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6);
}



template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_7 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3);
  MemberCallback4_7 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_7<C, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new MemberCallback4_7<C, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_7<C, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new MemberCallback4_7<C, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6);
}



template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
class Callback4_8 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3);
  Callback4_8(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2, x3);
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
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
Callback4_8<T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new Callback4_8<T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
Callback4_8<T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new Callback4_8<T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_8 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3) const;
  ConstMemberCallback4_8 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_8<C, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ConstMemberCallback4_8<C, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_8<C, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ConstMemberCallback4_8<C, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}



template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_8 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3);
  MemberCallback4_8 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_8<C, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new MemberCallback4_8<C, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_8<C, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new MemberCallback4_8<C, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}



template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
class Callback4_9 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3);
  Callback4_9(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2, x3);
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
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
Callback4_9<T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new Callback4_9<T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
Callback4_9<T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new Callback4_9<T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
class ConstMemberCallback4_9 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3) const;
  ConstMemberCallback4_9 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ConstMemberCallback4_9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
ConstMemberCallback4_9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewPermanentCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ConstMemberCallback4_9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}



template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
class MemberCallback4_9 : public Callback4<X0, X1, X2, X3> {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3);
  MemberCallback4_9 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : Callback4<X0, X1, X2, X3>(is_permanent),
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
  virtual void RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2, x3);
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

template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new MemberCallback4_9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
MemberCallback4_9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new MemberCallback4_9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}



template<typename R, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_0 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(X0, X1, X2, X3);
  ResultCallback4_0(bool is_permanent, Fun fun)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),

      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(x0, x1, x2, x3);
  }
private:

  Fun fun_;
};
template<typename R, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_0<R, X0, X1, X2, X3>* NewCallback(R (*fun)(X0, X1, X2, X3)) {
  return new ResultCallback4_0<R, X0, X1, X2, X3>(false, fun);
}
template<typename R, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_0<R, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(X0, X1, X2, X3)) {
  return new ResultCallback4_0<R, X0, X1, X2, X3>(true, fun);
}


template<typename C, typename R, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_0 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(X0, X1, X2, X3) const;
  ResultConstMemberCallback4_0 (bool is_permanent, const C* c, Fun fun)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),

    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(x0, x1, x2, x3);
  }
private:
  const C* c_;

  Fun fun_;
};

template<typename C, typename R, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_0<C, R, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(X0, X1, X2, X3) const) {
  return new ResultConstMemberCallback4_0<C, R, X0, X1, X2, X3>(false, c, fun);
}
template<typename C, typename R, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_0<C, R, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(X0, X1, X2, X3) const) {
  return new ResultConstMemberCallback4_0<C, R, X0, X1, X2, X3>(true, c, fun);
}



template<typename C, typename R, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_0 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(X0, X1, X2, X3);
  ResultMemberCallback4_0 (bool is_permanent, C* c, Fun fun)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),

    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(x0, x1, x2, x3);
  }
private:
  C* c_;

  Fun fun_;
};

template<typename C, typename R, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_0<C, R, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(X0, X1, X2, X3)) {
  return new ResultMemberCallback4_0<C, R, X0, X1, X2, X3>(false, c, fun);
}
template<typename C, typename R, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_0<C, R, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(X0, X1, X2, X3)) {
  return new ResultMemberCallback4_0<C, R, X0, X1, X2, X3>(true, c, fun);
}



template<typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_1 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, X0, X1, X2, X3);
  ResultCallback4_1(bool is_permanent, Fun fun, T0 p0)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
  Fun fun_;
};
template<typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_1<R, T0, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, X0, X1, X2, X3), T0 p0) {
  return new ResultCallback4_1<R, T0, X0, X1, X2, X3>(false, fun, p0);
}
template<typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_1<R, T0, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, X0, X1, X2, X3), T0 p0) {
  return new ResultCallback4_1<R, T0, X0, X1, X2, X3>(true, fun, p0);
}


template<typename C, typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_1 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_1 (bool is_permanent, const C* c, Fun fun, T0 p0)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, x0, x1, x2, x3);
  }
private:
  const C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_1<C, R, T0, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, X0, X1, X2, X3) const, T0 p0) {
  return new ResultConstMemberCallback4_1<C, R, T0, X0, X1, X2, X3>(false, c, fun, p0);
}
template<typename C, typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_1<C, R, T0, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, X0, X1, X2, X3) const, T0 p0) {
  return new ResultConstMemberCallback4_1<C, R, T0, X0, X1, X2, X3>(true, c, fun, p0);
}



template<typename C, typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_1 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, X0, X1, X2, X3);
  ResultMemberCallback4_1 (bool is_permanent, C* c, Fun fun, T0 p0)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, x0, x1, x2, x3);
  }
private:
  C* c_;
  T0 p0_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_1<C, R, T0, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, X0, X1, X2, X3), T0 p0) {
  return new ResultMemberCallback4_1<C, R, T0, X0, X1, X2, X3>(false, c, fun, p0);
}
template<typename C, typename R, typename T0, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_1<C, R, T0, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, X0, X1, X2, X3), T0 p0) {
  return new ResultMemberCallback4_1<C, R, T0, X0, X1, X2, X3>(true, c, fun, p0);
}



template<typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_2 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, T1, X0, X1, X2, X3);
  ResultCallback4_2(bool is_permanent, Fun fun, T0 p0, T1 p1)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, p1_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_2<R, T0, T1, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, T1, X0, X1, X2, X3), T0 p0, T1 p1) {
  return new ResultCallback4_2<R, T0, T1, X0, X1, X2, X3>(false, fun, p0, p1);
}
template<typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_2<R, T0, T1, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, T1, X0, X1, X2, X3), T0 p0, T1 p1) {
  return new ResultCallback4_2<R, T0, T1, X0, X1, X2, X3>(true, fun, p0, p1);
}


template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_2 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_2 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, x0, x1, x2, x3);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_2<C, R, T0, T1, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, T1, X0, X1, X2, X3) const, T0 p0, T1 p1) {
  return new ResultConstMemberCallback4_2<C, R, T0, T1, X0, X1, X2, X3>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_2<C, R, T0, T1, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, X0, X1, X2, X3) const, T0 p0, T1 p1) {
  return new ResultConstMemberCallback4_2<C, R, T0, T1, X0, X1, X2, X3>(true, c, fun, p0, p1);
}



template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_2 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, X0, X1, X2, X3);
  ResultMemberCallback4_2 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, x0, x1, x2, x3);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_2<C, R, T0, T1, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, T1, X0, X1, X2, X3), T0 p0, T1 p1) {
  return new ResultMemberCallback4_2<C, R, T0, T1, X0, X1, X2, X3>(false, c, fun, p0, p1);
}
template<typename C, typename R, typename T0, typename T1, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_2<C, R, T0, T1, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, X0, X1, X2, X3), T0 p0, T1 p1) {
  return new ResultMemberCallback4_2<C, R, T0, T1, X0, X1, X2, X3>(true, c, fun, p0, p1);
}



template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_3 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, T1, T2, X0, X1, X2, X3);
  ResultCallback4_3(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, p1_, p2_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_3<R, T0, T1, T2, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, T1, T2, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2) {
  return new ResultCallback4_3<R, T0, T1, T2, X0, X1, X2, X3>(false, fun, p0, p1, p2);
}
template<typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_3<R, T0, T1, T2, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, T1, T2, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2) {
  return new ResultCallback4_3<R, T0, T1, T2, X0, X1, X2, X3>(true, fun, p0, p1, p2);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_3 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_3 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, x0, x1, x2, x3);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_3<C, R, T0, T1, T2, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberCallback4_3<C, R, T0, T1, T2, X0, X1, X2, X3>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_3<C, R, T0, T1, T2, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2) {
  return new ResultConstMemberCallback4_3<C, R, T0, T1, T2, X0, X1, X2, X3>(true, c, fun, p0, p1, p2);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_3 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, X0, X1, X2, X3);
  ResultMemberCallback4_3 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, x0, x1, x2, x3);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_3<C, R, T0, T1, T2, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberCallback4_3<C, R, T0, T1, T2, X0, X1, X2, X3>(false, c, fun, p0, p1, p2);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_3<C, R, T0, T1, T2, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2) {
  return new ResultMemberCallback4_3<C, R, T0, T1, T2, X0, X1, X2, X3>(true, c, fun, p0, p1, p2);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_4 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, X0, X1, X2, X3);
  ResultCallback4_4(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_4<R, T0, T1, T2, T3, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, T1, T2, T3, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultCallback4_4<R, T0, T1, T2, T3, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_4<R, T0, T1, T2, T3, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultCallback4_4<R, T0, T1, T2, T3, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_4 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_4 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2, x3);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_4<C, R, T0, T1, T2, T3, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberCallback4_4<C, R, T0, T1, T2, T3, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_4<C, R, T0, T1, T2, T3, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultConstMemberCallback4_4<C, R, T0, T1, T2, T3, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_4 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, X0, X1, X2, X3);
  ResultMemberCallback4_4 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, x0, x1, x2, x3);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_4<C, R, T0, T1, T2, T3, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberCallback4_4<C, R, T0, T1, T2, T3, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_4<C, R, T0, T1, T2, T3, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ResultMemberCallback4_4<C, R, T0, T1, T2, T3, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_5 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3);
  ResultCallback4_5(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2, x3);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_5<R, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultCallback4_5<R, T0, T1, T2, T3, T4, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_5<R, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultCallback4_5<R, T0, T1, T2, T3, T4, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_5 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_5 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberCallback4_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultConstMemberCallback4_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_5 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3);
  ResultMemberCallback4_5 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
    fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberCallback4_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ResultMemberCallback4_5<C, R, T0, T1, T2, T3, T4, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_6 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3);
  ResultCallback4_6(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
      fun_(fun) {
  }
protected:
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2, x3);
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
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_6<R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultCallback4_6<R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4, p5);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_6<R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultCallback4_6<R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4, p5);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_6 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_6 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultConstMemberCallback4_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultConstMemberCallback4_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_6 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3);
  ResultMemberCallback4_6 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultMemberCallback4_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ResultMemberCallback4_6<C, R, T0, T1, T2, T3, T4, T5, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_7 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3);
  ResultCallback4_7(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2, x3);
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
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_7<R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultCallback4_7<R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_7<R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultCallback4_7<R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4, p5, p6);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_7 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_7 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultConstMemberCallback4_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultConstMemberCallback4_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_7 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3);
  ResultMemberCallback4_7 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultMemberCallback4_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ResultMemberCallback4_7<C, R, T0, T1, T2, T3, T4, T5, T6, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_8 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3);
  ResultCallback4_8(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2, x3);
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
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_8<R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultCallback4_8<R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_8<R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultCallback4_8<R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_8 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_8 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultConstMemberCallback4_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultConstMemberCallback4_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_8 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3);
  ResultMemberCallback4_8 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultMemberCallback4_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ResultMemberCallback4_8<C, R, T0, T1, T2, T3, T4, T5, T6, T7, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}



template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
class ResultCallback4_9 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3);
  ResultCallback4_9(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2, x3);
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
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultCallback4_9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(false, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
ResultCallback4_9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewPermanentCallback(R (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultCallback4_9<R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(true, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}


template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
class ResultConstMemberCallback4_9 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3) const;
  ResultConstMemberCallback4_9 (bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultConstMemberCallback4_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
ResultConstMemberCallback4_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewPermanentCallback(const C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultConstMemberCallback4_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}



template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
class ResultMemberCallback4_9 : public ResultCallback4<R, X0, X1, X2, X3> {
public:
  typedef R (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3);
  ResultMemberCallback4_9 (bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : ResultCallback4<R, X0, X1, X2, X3>(is_permanent),
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
  virtual R RunInternal(X0 x0, X1 x1, X2 x2, X3 x3) {
    return (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_, x0, x1, x2, x3);
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

template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultMemberCallback4_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename C, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename X0, typename X1, typename X2, typename X3>
ResultMemberCallback4_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>* NewPermanentCallback(C* c, R (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ResultMemberCallback4_9<C, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, X0, X1, X2, X3>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}

}

#endif
