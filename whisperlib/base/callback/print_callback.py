#!/usr/bin/python
#
# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Catalin Popescu
#
# This handy script generates functor classes (see callback.h, callback1.h
# for examples..)
#
######################################################################

def ClosureN(N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<%s>
class Closure%d : public Closure {
public:
  typedef void (*Fun)(%s);
  Closure%d(bool is_permanent, Fun fun, %s)
    : Closure(is_permanent),
    %s
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(%s);
  }
private:
  %s
  Fun fun_;
};
template<%s>
Closure%d<%s>* NewCallback(void (*fun)(%s), %s) {
  return new Closure%d<%s>(false, fun, %s);
}
template<%s>
Closure%d<%s>* NewPermanentCallback(void (*fun)(%s), %s) {
  return new Closure%d<%s>(true, fun, %s);
}
""" % (typenames,
N,
typeargs,
N,
params,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
', '.join(['p%d_' % (i) for i in range(N)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)]),
typenames,
N, typeargs, typeargs,
params, N, typeargs, paramnames,
typenames,
N, typeargs, typeargs,
params, N, typeargs, paramnames)

######################################################################

def ConstMemberClosureN(N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<typename C, %s>
class ConstMemberClosure%d : public Closure {
public:
  typedef void (C::*Fun)(%s) const;
  ConstMemberClosure%d(bool is_permanent, const C* c, Fun fun, %s)
    : Closure(is_permanent),
    c_(c),
    %s
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(%s);
  }
private:
  const C* c_;
  %s
  Fun fun_;
};
template<typename C, %s>
ConstMemberClosure%d<C, %s>* NewCallback(const C* c, void (C::*fun)(%s) const, %s) {
  return new ConstMemberClosure%d<C, %s>(false, c, fun, %s);
}
template<typename C, %s>
ConstMemberClosure%d<C, %s>* NewPermanentCallback(C* c, void (C::*fun)(%s) const, %s) {
  return new ConstMemberClosure%d<C, %s>(true, c, fun, %s);
}
""" % (typenames,
N,
typeargs,
N,
params,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
', '.join(['p%d_' % (i) for i in range(N)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)]),
typenames,
N, typeargs, typeargs,
params, N, typeargs, paramnames,
typenames,
N, typeargs, typeargs,
params, N, typeargs, paramnames)

######################################################################

def MemberClosureN(N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<typename C, %s>
class MemberClosure%d : public Closure {
public:
  typedef void (C::*Fun)(%s);
  MemberClosure%d(bool is_permanent, C* c, Fun fun, %s)
    : Closure(is_permanent),
    c_(c),
    %s
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(%s);
  }
private:
  C* c_;
  %s
  Fun fun_;
};
template<typename C, %s>
MemberClosure%d<C, %s>* NewCallback(C* c, void (C::*fun)(%s), %s) {
  return new MemberClosure%d<C, %s>(false, c, fun, %s);
}
template<typename C, %s>
MemberClosure%d<C, %s>* NewPermanentCallback(C* c, void (C::*fun)(%s), %s) {
  return new MemberClosure%d<C, %s>(true, c, fun, %s);
}
""" % (typenames,
N,
typeargs,
N,
params,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
', '.join(['p%d_' % (i) for i in range(N)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)]),
typenames,
N, typeargs, typeargs,
params, N, typeargs, paramnames,
typenames,
N, typeargs, typeargs,
params, N, typeargs, paramnames)

######################################################################

## Callback X

######################################################################

def CallbackX(M, N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    typenamesX = ', '.join(['typename X%d' % i for i in range(M)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    paramsX = ', '.join(['X%d x%d' % (i, i) for i in range(M)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    typeargsX = ', '.join(['X%d' % i for i in range(M)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<%s, %s>
class Callback%d_%d : public Callback%d<%s> {
public:
  typedef void (*Fun)(%s, %s);
  Callback%d_%d(bool is_permanent, Fun fun, %s)
    : Callback%d<%s>(is_permanent),
    %s
      fun_(fun) {
  }
protected:
  virtual void RunInternal(%s) {
    (*fun_)(%s, %s);
  }
private:
  %s
  Fun fun_;
};
template<%s, %s>
Callback%d_%d<%s, %s>* NewCallback(void (*fun)(%s, %s), %s) {
  return new Callback%d_%d<%s, %s>(false, fun, %s);
}
template<%s, %s>
Callback%d_%d<%s, %s>* NewPermanentCallback(void (*fun)(%s, %s), %s) {
  return new Callback%d_%d<%s, %s>(true, fun, %s);
}
""" % (typenames, typenamesX,
M, N,
M, typeargsX,
typeargs, typeargsX,
M, N, params,
M, typeargsX,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
paramsX,
', '.join(['p%d_' % (i) for i in range(N)]),
', '.join(['x%d' % (i) for i in range(M)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)]),
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames,
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames)

######################################################################

def ConstMemberCallbackX(M, N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    typenamesX = ', '.join(['typename X%d' % i for i in range(M)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    paramsX = ', '.join(['X%d x%d' % (i, i) for i in range(M)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    typeargsX = ', '.join(['X%d' % i for i in range(M)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<typename C, %s, %s>
class ConstMemberCallback%d_%d : public Callback%d<%s> {
public:
  typedef void (C::*Fun)(%s, %s) const;
  ConstMemberCallback%d_%d (bool is_permanent, const C* c, Fun fun, %s)
    : Callback%d<%s>(is_permanent),
    c_(c),
    %s
      fun_(fun) {
  }
protected:
  virtual void RunInternal(%s) {
      (c_->*fun_)(%s, %s);
  }
private:
  const C* c_;
  %s
  Fun fun_;
}; """ % ( typenames, typenamesX,
M, N,
M, typeargsX,
typeargs, typeargsX,
M, N, params,
M, typeargsX,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
paramsX,
', '.join(['p%d_' % (i) for i in range(N)]),
', '.join(['x%d' % (i) for i in range(M)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)])
)
    print """
template<typename C, %s, %s>
ConstMemberCallback%d_%d<C, %s, %s>* NewCallback(const C* c, void (C::*fun)(%s, %s) const, %s) {
  return new ConstMemberCallback%d_%d<C, %s, %s>(false, c, fun, %s);
}
template<typename C, %s, %s>
ConstMemberCallback%d_%d<C, %s, %s>* NewPermanentCallback(const C* c, void (C::*fun)(%s, %s) const, %s) {
  return new ConstMemberCallback%d_%d<C, %s, %s>(true, c, fun, %s);
}

""" % (
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames,
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames)

######################################################################

def MemberCallbackX(M, N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    typenamesX = ', '.join(['typename X%d' % i for i in range(M)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    paramsX = ', '.join(['X%d x%d' % (i, i) for i in range(M)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    typeargsX = ', '.join(['X%d' % i for i in range(M)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<typename C, %s, %s>
class MemberCallback%d_%d : public Callback%d<%s> {
public:
  typedef void (C::*Fun)(%s, %s);
  MemberCallback%d_%d (bool is_permanent, C* c, Fun fun, %s)
    : Callback%d<%s>(is_permanent),
    c_(c),
    %s
      fun_(fun) {
  }
protected:
  virtual void RunInternal(%s) {
      (c_->*fun_)(%s, %s);
  }
private:
  C* c_;
  %s
  Fun fun_;
}; """ % ( typenames, typenamesX,
M, N,
M, typeargsX,
typeargs, typeargsX,
M, N, params,
M, typeargsX,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
paramsX,
', '.join(['p%d_' % (i) for i in range(N)]),
', '.join(['x%d' % (i) for i in range(M)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)])
)
    print """
template<typename C, %s, %s>
MemberCallback%d_%d<C, %s, %s>* NewCallback(C* c, void (C::*fun)(%s, %s), %s) {
  return new MemberCallback%d_%d<C, %s, %s>(false, c, fun, %s);
}
template<typename C, %s, %s>
MemberCallback%d_%d<C, %s, %s>* NewPermanentCallback(C* c, void (C::*fun)(%s, %s), %s) {
  return new MemberCallback%d_%d<C, %s, %s>(true, c, fun, %s);
}

""" % (
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames,
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames)


######################################################################

## ResultCallback X

######################################################################

def ResultCallbackX(M, N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    typenamesX = ', '.join(['typename X%d' % i for i in range(M)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    paramsX = ', '.join(['X%d x%d' % (i, i) for i in range(M)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    typeargsX = ', '.join(['X%d' % i for i in range(M)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<typename R, %s, %s>
class ResultCallback%d_%d : public ResultCallback%d<R, %s> {
public:
  typedef R (*Fun)(%s, %s);
  ResultCallback%d_%d(bool is_permanent, Fun fun, %s)
    : ResultCallback%d<R, %s>(is_permanent),
    %s
      fun_(fun) {
  }
protected:
  virtual R RunInternal(%s) {
    return (*fun_)(%s, %s);
  }
private:
  %s
  Fun fun_;
};
template<typename R, %s, %s>
ResultCallback%d_%d<R, %s, %s>* NewCallback(R (*fun)(%s, %s), %s) {
  return new ResultCallback%d_%d<R, %s, %s>(false, fun, %s);
}
template<typename R, %s, %s>
ResultCallback%d_%d<R, %s, %s>* NewPermanentCallback(R (*fun)(%s, %s), %s) {
  return new ResultCallback%d_%d<R, %s, %s>(true, fun, %s);
}
""" % (typenames, typenamesX,
M, N,
M, typeargsX,
typeargs, typeargsX,
M, N, params,
M, typeargsX,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
paramsX,
', '.join(['p%d_' % (i) for i in range(N)]),
', '.join(['x%d' % (i) for i in range(M)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)]),
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames,
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames)

######################################################################

def ResultConstMemberCallbackX(M, N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    typenamesX = ', '.join(['typename X%d' % i for i in range(M)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    paramsX = ', '.join(['X%d x%d' % (i, i) for i in range(M)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    typeargsX = ', '.join(['X%d' % i for i in range(M)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<typename C, typename R, %s, %s>
class ResultConstMemberCallback%d_%d : public ResultCallback%d<R, %s> {
public:
  typedef R (C::*Fun)(%s, %s) const;
  ResultConstMemberCallback%d_%d (bool is_permanent, const C* c, Fun fun, %s)
    : ResultCallback%d<R, %s>(is_permanent),
    c_(c),
    %s
    fun_(fun) {
  }
protected:
  virtual R RunInternal(%s) {
    return (c_->*fun_)(%s, %s);
  }
private:
  const C* c_;
  %s
  Fun fun_;
}; """ % ( typenames, typenamesX,
M, N,
M, typeargsX,
typeargs, typeargsX,
M, N, params,
M, typeargsX,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
paramsX,
', '.join(['p%d_' % (i) for i in range(N)]),
', '.join(['x%d' % (i) for i in range(M)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)])
)
    print """
template<typename C, typename R, %s, %s>
ResultConstMemberCallback%d_%d<C, R, %s, %s>* NewCallback(const C* c, R (C::*fun)(%s, %s) const, %s) {
  return new ResultConstMemberCallback%d_%d<C, R, %s, %s>(false, c, fun, %s);
}
template<typename C, typename R, %s, %s>
ResultConstMemberCallback%d_%d<C, R, %s, %s>* NewPermanentCallback(const C* c, R (C::*fun)(%s, %s) const, %s) {
  return new ResultConstMemberCallback%d_%d<C, R, %s, %s>(true, c, fun, %s);
}

""" % (
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames,
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames)

######################################################################

def ResultMemberCallbackX(M, N):
    typenames = ', '.join(['typename T%d' % i for i in range(N)])
    typenamesX = ', '.join(['typename X%d' % i for i in range(M)])
    params = ', '.join(['T%d p%d' % (i, i) for i in range(N)])
    paramsX = ', '.join(['X%d x%d' % (i, i) for i in range(M)])
    typeargs = ', '.join(['T%d' % i for i in range(N)])
    typeargsX = ', '.join(['X%d' % i for i in range(M)])
    paramnames = ', '.join(['p%d' % (i) for i in range(N)])
    print """
template<typename C, typename R, %s, %s>
class ResultMemberCallback%d_%d : public ResultCallback%d<R, %s> {
public:
  typedef R (C::*Fun)(%s, %s);
  ResultMemberCallback%d_%d (bool is_permanent, C* c, Fun fun, %s)
    : ResultCallback%d<R, %s>(is_permanent),
    c_(c),
    %s
    fun_(fun) {
  }
protected:
  virtual R RunInternal(%s) {
    return (c_->*fun_)(%s, %s);
  }
private:
  C* c_;
  %s
  Fun fun_;
}; """ % ( typenames, typenamesX,
M, N,
M, typeargsX,
typeargs, typeargsX,
M, N, params,
M, typeargsX,
'\n'.join(['p%d_(p%d),' % (i, i) for i in range(N)]),
paramsX,
', '.join(['p%d_' % (i) for i in range(N)]),
', '.join(['x%d' % (i) for i in range(M)]),
'\n'.join(['T%d p%d_;' % (i, i) for i in range(N)])
)
    print """
template<typename C, typename R, %s, %s>
ResultMemberCallback%d_%d<C, R, %s, %s>* NewCallback(C* c, R (C::*fun)(%s, %s), %s) {
  return new ResultMemberCallback%d_%d<C, R, %s, %s>(false, c, fun, %s);
}
template<typename C, typename R, %s, %s>
ResultMemberCallback%d_%d<C, R, %s, %s>* NewPermanentCallback(C* c, R (C::*fun)(%s, %s), %s) {
  return new ResultMemberCallback%d_%d<C, R, %s, %s>(true, c, fun, %s);
}

""" % (
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames,
typenames, typenamesX,
M, N, typeargs, typeargsX, typeargs, typeargsX,
params, M, N, typeargs, typeargsX, paramnames)


for i in range(1, 10):
  ClosureN(i)
  MemberClosureN(i)
  ConstMemberClosureN(i)

#for i in range(1, 10):
#  CallbackX(1,i)
#  ConstMemberCallbackX(1, i)
#  MemberCallbackX(1, i)

#for i in range(1, 10):
#  ResultCallbackX(1,i)
#  ResultConstMemberCallbackX(1, i)
#  ResultMemberCallbackX(1, i)
