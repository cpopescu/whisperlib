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
// * Neither the name of Whispersoft s.r.l nor the names of its
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

#ifndef __WHISPERLIB_BASE_TYPES_H
#define __WHISPERLIB_BASE_TYPES_H
#pragma once

#include <whisperlib/base/core_config.h>

#if !defined(USE_WAF) && defined(__has_include)
#if __has_include("ext/have_includes.h")
#include "ext/have_includes.h"
#endif
#endif

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if defined(HAVE_NAMESER8_COMPAT_H)
#include <nameser8_compat.h>
#elif defined(HAVE_ENDIAN_H)
#include <endian.h>
#elif defined(HAVE_MACHINE_ENDIAN_H)
#include <machine/endian.h>
#endif

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR '/'
#endif

namespace whisper {
namespace common {
// considering int value 0x01020304
enum ByteOrder {
  BIGENDIAN,  // written as: 0x01, 0x02, 0x03, 0x04 ;
              //                     PowerPC, SGI Origin, Sun Sparc
  LILENDIAN,  // written as: 0x04, 0x03, 0x02, 0x01 ; Intel x86, Alpha AXP
  PDPENDIAN,  // written as: 0x03, 0x04, 0x01, 0x02 ; ARM -- NO SUPPORTED
  UNKNOWNENDIAN
};

// returns:
//  the name of the given byte order
const char* ByteOrderName(ByteOrder order);

//////////////////////////////////////////////////////////////////////
//
// Determine byte order at compile time
//
#ifdef BYTE_ORDER
# if defined BYTE_ORDER && defined LITTLE_ENDIAN && defined BIG_ENDIAN
#   if BYTE_ORDER == LITTLE_ENDIAN
#      define LOCAL_MACHINE_BYTE_ORDER_DEFINED
const ByteOrder kByteOrder = LILENDIAN;
#   elif BYTE_ORDER == BIG_ENDIAN
#     define LOCAL_MACHINE_BYTE_ORDER_DEFINED
const ByteOrder kByteOrder = BIGENDIAN;
#   endif
# endif
#else
# if defined __BYTE_ORDER && defined __LITTLE_ENDIAN && defined __BIG_ENDIAN
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#      define LOCAL_MACHINE_BYTE_ORDER_DEFINED
const ByteOrder kByteOrder = LILENDIAN;
#   elif __BYTE_ORDER == __BIG_ENDIAN
#     define LOCAL_MACHINE_BYTE_ORDER_DEFINED
const ByteOrder kByteOrder = BIGENDIAN;
#   endif
# endif
#endif

# if !defined LOCAL_MACHINE_BYTE_ORDER_DEFINED
#   error "We should have the byteorder defined at compile time"
# endif
}  // namespace common
}  // namespace whisper

#ifdef __GNUC__
  #define GCC_VER (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif
#ifdef __clang__
  #define CLANG_VER (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif


#ifndef PRId64
#define PRId64 "lld"
#endif
#ifndef PRIu64
#define PRIu64 "llu"
#endif
#ifndef PRIx64
#define PRIx64 "llx"
#endif

#include <limits.h>         // So we can set the bounds of our types
#include <stddef.h>         // For size_t
#include <stdint.h>
// #include <string.h>         // for memcpy

#ifdef COMPILER_MSVC
#define GG_LONGLONG(x) x##I64
#define GG_ULONGLONG(x) x##UI64
#else
#define GG_LONGLONG(x) x##LL
#define GG_ULONGLONG(x) x##ULL
#endif

#ifdef HAVE_STDINT_H

// for PRId64 , PRIu64 , ...
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

//////////////////////////////////////////////////////////////////////


// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdarg.h>

// Per C99 7.8.14, define __STDC_CONSTANT_MACROS before including <stdint.h>
// to get the INTn_C and UINTn_C macros for integer constants.  It's difficult
// to guarantee any specific ordering of header includes, so it's difficult to
// guarantee that the INTn_C macros can be defined by including <stdint.h> at
// any specific point.  Provide GG_INTn_C macros instead.

#define GG_INT8_C(x)    (x)
#define GG_INT16_C(x)   (x)
#define GG_INT32_C(x)   (x)
#define GG_INT64_C(x)   GG_LONGLONG(x)

#define GG_UINT8_C(x)   (x ## U)
#define GG_UINT16_C(x)  (x ## U)
#define GG_UINT32_C(x)  (x ## U)
#define GG_UINT64_C(x)  GG_ULONGLONG(x)

// It's possible for functions that use a va_list, such as StringPrintf, to
// invalidate the data in it upon use.  The fix is to make a copy of the
// structure before using it and use that copy instead.  va_copy is provided
// for this purpose.  MSVC does not provide va_copy, so define an
// implementation here.  It is not guaranteed that assignment is a copy, so the
// StringUtil.VariableArgsFunc unit test tests this capability.
#if defined(COMPILER_GCC)
#define GG_VA_COPY(a, b) (va_copy(a, b))
#elif defined(COMPILER_MSVC)
#define GG_VA_COPY(a, b) (a = b)
#endif

// Define an OS-neutral wrapper for shared library entry points
#if defined(OS_WIN)
#define API_CALL __stdcall
#else
#define API_CALL
#endif

//////////////////////////////////////////////////////////////////////
//
// Copyright 2010 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8         schar;
typedef int32        char32;

#else

typedef signed char         schar;
typedef signed char         int8;
typedef short               int16;
// TODO: Remove these type guards.  These are to avoid conflicts with
// obsolete/protypes.h in the Gecko SDK.
#ifndef _INT32
#define _INT32
typedef int                 int32;
#endif

// The NSPR system headers define 64-bit as |long| when possible, except on
// Mac OS X.  In order to not have typedef mismatches, we do the same on LP64.
//
// On Mac OS X, |long long| is used for 64-bit types for compatibility with
// <inttypes.h> format macros even in the LP64 model.
#if defined(__LP64__) && !defined(OS_MACOSX) && !defined(OS_OPENBSD)
typedef long                int64;
#else
typedef long long           int64;
#endif

// NOTE: unsigned types are DANGEROUS in loops and other arithmetical
// places.  Use the signed types unless your variable represents a bit
// pattern (eg a hash value) or you really need the extra bit.  Do NOT
// use 'unsigned' to express "this value should always be positive";
// use assertions for this.

typedef unsigned char      uint8;
typedef unsigned short     uint16;
// TODO: Remove these type guards.  These are to avoid conflicts with
// obsolete/protypes.h in the Gecko SDK.
#ifndef _UINT32
#define _UINT32
typedef unsigned int       uint32;
#endif

// See the comment above about NSPR and 64-bit.
#if defined(__LP64__) && !defined(OS_MACOSX) && !defined(OS_OPENBSD)
typedef unsigned long uint64;
#else
typedef unsigned long long uint64;
#endif

// A type to represent a Unicode code-point value. As of Unicode 4.0,
// such values require up to 21 bits.
// (For type-checking on pointers, make this explicitly signed,
// and it should always be the signed version of whatever int32 is.)
typedef signed int         char32;

#endif  // HAVE_STDINT_H


const uint8  kuint8max  = (( uint8) 0xFF);
const uint16 kuint16max = ((uint16) 0xFFFF);
const uint32 kuint32max = ((uint32) 0xFFFFFFFF);
const uint64 kuint64max = ((uint64) GG_LONGLONG(0xFFFFFFFFFFFFFFFF));
const  int8  kint8min   = ((  int8) 0x80);
const  int8  kint8max   = ((  int8) 0x7F);
const  int16 kint16min  = (( int16) 0x8000);
const  int16 kint16max  = (( int16) 0x7FFF);
const  int32 kint32min  = (( int32) 0x80000000);
const  int32 kint32max  = (( int32) 0x7FFFFFFF);
const  int64 kint64min  = (( int64) GG_LONGLONG(0x8000000000000000));
const  int64 kint64max  = (( int64) GG_LONGLONG(0x7FFFFFFFFFFFFFFF));

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

// An older, deprecated, politically incorrect name for the above.
// NOTE: The usage of this macro was baned from our code base, but some
// third_party libraries are yet using it.
// TODO(tfarina): Figure out how to fix the usage of this macro in the
// third_party libraries and get rid of it.
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName) DISALLOW_COPY_AND_ASSIGN(TypeName)

// A macro to disallow all the implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
//
// This should be used in the private: declarations for a class
// that wants to prevent anyone from instantiating it. This is
// especially useful for classes containing only static methods.
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

// The arraysize(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.  If you use arraysize on
// a pointer by mistake, you will get a compile-time error.
//
// One caveat is that arraysize() doesn't accept any array of an
// anonymous type or a type defined inside a function.  In these rare
// cases, you have to use the unsafe ARRAYSIZE_UNSAFE() macro below.  This is
// due to a limitation in C++'s template system.  The limitation might
// eventually be removed, but it hasn't happened yet.

// This template function declaration is used in defining arraysize.
// Note that the function doesn't need an implementation, as we only
// use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

// That gcc wants both of these prototypes seems mysterious. VC, for
// its part, can't decide which to use (another mystery). Matching of
// template overloads: the final frontier.
#ifndef _MSC_VER
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];
#endif

#define arraysize(array) (sizeof(ArraySizeHelper(array)))

// ARRAYSIZE_UNSAFE performs essentially the same calculation as arraysize,
// but can be used on anonymous types or types defined inside
// functions.  It's less safe than arraysize as it accepts some
// (although not all) pointers.  Therefore, you should use arraysize
// whenever possible.
//
// The expression ARRAYSIZE_UNSAFE(a) is a compile-time constant of type
// size_t.
//
// ARRAYSIZE_UNSAFE catches a few type errors.  If you see a compiler error
//
//   "warning: division by zero in ..."
//
// when using ARRAYSIZE_UNSAFE, you are (wrongfully) giving it a pointer.
// You should only use ARRAYSIZE_UNSAFE on statically allocated arrays.
//
// The following comments are on the implementation details, and can
// be ignored by the users.
//
// ARRAYSIZE_UNSAFE(arr) works by inspecting sizeof(arr) (the # of bytes in
// the array) and sizeof(*(arr)) (the # of bytes in one array
// element).  If the former is divisible by the latter, perhaps arr is
// indeed an array, in which case the division result is the # of
// elements in the array.  Otherwise, arr cannot possibly be an array,
// and we generate a compiler error to prevent the code from
// compiling.
//
// Since the size of bool is implementation-defined, we need to cast
// !(sizeof(a) & sizeof(*(a))) to size_t in order to ensure the final
// result has type size_t.
//
// This macro is not perfect as it wrongfully accepts certain
// pointers, namely where the pointer size is divisible by the pointee
// size.  Since all our code has to go through a 32-bit compiler,
// where a pointer is 4 bytes, this means all pointers to a type whose
// size is 3 or greater than 4 will be (righteously) rejected.

#define ARRAYSIZE_UNSAFE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))


// Use implicit_cast as a safe version of static_cast or const_cast
// for upcasting in the type hierarchy (i.e. casting a pointer to Foo
// to a pointer to SuperclassOfFoo or casting a pointer to Foo to
// a const pointer to Foo).
// When you use implicit_cast, the compiler checks that the cast is safe.
// Such explicit implicit_casts are necessary in surprisingly many
// situations where C++ demands an exact type match instead of an
// argument type convertable to a target type.
//
// The From type can be inferred, so the preferred syntax for using
// implicit_cast is the same as for static_cast etc.:
//
//   implicit_cast<ToType>(expr)
//
// implicit_cast would have been part of the C++ standard library,
// but the proposal was submitted too late.  It will probably make
// its way into the language in the future.
template<typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}

// The COMPILE_ASSERT macro can be used to verify that a compile time
// expression is true. For example, you could use it to verify the
// size of a static array:
//
//   COMPILE_ASSERT(ARRAYSIZE_UNSAFE(content_type_names) == CONTENT_NUM_TYPES,
//                  content_type_names_incorrect_size);
//
// or to make sure a struct is smaller than a certain size:
//
//   COMPILE_ASSERT(sizeof(foo) < 128, foo_too_large);
//
// The second argument to the macro is the name of the variable. If
// the expression is false, most compilers will issue a warning/error
// containing the name of the variable.

#undef COMPILE_ASSERT

#if __cplusplus > 199711L
#define COMPILE_ASSERT(expr, msg)            \
    static_assert(expr, #msg)
#else
template <bool>
struct CompileAssert {
};
#define COMPILE_ASSERT(expr, msg)                               \
    typedef CompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]
#endif

// Implementation details of COMPILE_ASSERT:
//
// - COMPILE_ASSERT works by defining an array type that has -1
//   elements (and thus is invalid) when the expression is false.
//
// - The simpler definition
//
//     #define COMPILE_ASSERT(expr, msg) typedef char msg[(expr) ? 1 : -1]
//
//   does not work, as gcc supports variable-length arrays whose sizes
//   are determined at run-time (this is gcc's extension and not part
//   of the C++ standard).  As a result, gcc fails to reject the
//   following code with the simple definition:
//
//     int foo;
//     COMPILE_ASSERT(foo, msg); // not supposed to compile as foo is
//                               // not a compile-time constant.
//
// - By using the type CompileAssert<(bool(expr))>, we ensures that
//   expr is a compile-time constant.  (Template arguments must be
//   determined at compile-time.)
//
// - The outer parentheses in CompileAssert<(bool(expr))> are necessary
//   to work around a bug in gcc 3.4.4 and 4.0.1.  If we had written
//
//     CompileAssert<bool(expr)>
//
//   instead, these compilers will refuse to compile
//
//     COMPILE_ASSERT(5 > 0, some_message);
//
//   (They seem to think the ">" in "5 > 0" marks the end of the
//   template argument list.)
//
// - The array size is (bool(expr) ? 1 : -1), instead of simply
//
//     ((expr) ? 1 : -1).
//
//   This is to avoid running into a bug in MS VC 7.1, which
//   causes ((0.0) ? 1 : -1) to incorrectly evaluate to 1.


// MetatagId refers to metatag-id that we assign to
// each metatag <name, value> pair..
typedef uint32 MetatagId;

// Argument type used in interfaces that can optionally take ownership
// of a passed in argument.  If TAKE_OWNERSHIP is passed, the called
// object takes ownership of the argument.  Otherwise it does not.
enum Ownership {
  DO_NOT_TAKE_OWNERSHIP,
  TAKE_OWNERSHIP
};

// bit_cast<Dest,Source> is a template function that implements the
// equivalent of "*reinterpret_cast<Dest*>(&source)".  We need this in
// very low-level functions like the protobuf library and fast math
// support.
//
//   float f = 3.14159265358979;
//   int i = bit_cast<int32>(f);
//   // i = 0x40490fdb
//
// The classical address-casting method is:
//
//   // WRONG
//   float f = 3.14159265358979;            // WRONG
//   int i = * reinterpret_cast<int*>(&f);  // WRONG
//
// The address-casting method actually produces undefined behavior
// according to ISO C++ specification section 3.10 -15 -.  Roughly, this
// section says: if an object in memory has one type, and a program
// accesses it with a different type, then the result is undefined
// behavior for most values of "different type".
//
// This is true for any cast syntax, either *(int*)&f or
// *reinterpret_cast<int*>(&f).  And it is particularly true for
// conversions betweeen integral lvalues and floating-point lvalues.
//
// The purpose of 3.10 -15- is to allow optimizing compilers to assume
// that expressions with different types refer to different memory.  gcc
// 4.0.1 has an optimizer that takes advantage of this.  So a
// non-conforming program quietly produces wildly incorrect output.
//
// The problem is not the use of reinterpret_cast.  The problem is type
// punning: holding an object in memory of one type and reading its bits
// back using a different type.
//
// The C++ standard is more subtle and complex than this, but that
// is the basic idea.
//
// Anyways ...
//
// bit_cast<> calls memcpy() which is blessed by the standard,
// especially by the example in section 3.9 .  Also, of course,
// bit_cast<> wraps up the nasty logic in one place.
//
// Fortunately memcpy() is very fast.  In optimized mode, with a
// constant size, gcc 2.95.3, gcc 4.0.1, and msvc 7.1 produce inline
// code with the minimal amount of data movement.  On a 32-bit system,
// memcpy(d,s,4) compiles to one load and one store, and memcpy(d,s,8)
// compiles to two loads and two stores.
//
// I tested this code with gcc 2.95.3, gcc 4.0.1, icc 8.1, and msvc 7.1.
//
// WARNING: if Dest or Source is a non-POD type, the result of the memcpy
// is likely to surprise you.

template <class Dest, class Source>
inline Dest bit_cast(const Source& source) {
  // Compile time assertion: sizeof(Dest) == sizeof(Source)
  // A compile error here means your Dest and Source have different sizes.

#ifndef __has_feature
#define __has_feature(x) 0  // Compatibility with non-clang compilers.
#endif

#if __has_feature(cxx_static_assert) ||  __cplusplus > 199711L
  static_assert(sizeof(Dest) == sizeof(Source), "sizeof(Source) and Dest differs");
#else
  typedef char VerifySizesAreEqual [sizeof(Dest) == sizeof(Source) ? 1 : -1];
#endif

  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}

// Used to explicitly mark the return value of a function as unused. If you are
// really sure you don't want to do anything with the return value of a function
// that has been marked WARN_UNUSED_RESULT, wrap it with this. Example:
//
//   scoped_ptr<MyType> my_var = ...;
//   if (TakeOwnership(my_var.get()) == SUCCESS)
//     ignore_result(my_var.release());
//
template<typename T>
inline void ignore_result(const T&) {
}

// The following enum should be used only as a constructor argument to indicate
// that the variable has static storage class, and that the constructor should
// do nothing to its state.  It indicates to the reader that it is legal to
// declare a static instance of the class, provided the constructor is given
// the base::LINKER_INITIALIZED argument.  Normally, it is unsafe to declare a
// static variable that has a constructor or a destructor because invocation
// order is undefined.  However, IF the type can be initialized by filling with
// zeroes (which the loader does for static variables), AND the destructor also
// does nothing to the storage, AND there are no virtual methods, then a
// constructor declared as
//       explicit MyClass(base::LinkerInitialized x) {}
// and invoked as
//       static MyClass my_variable_name(base::LINKER_INITIALIZED);
namespace base {
enum LinkerInitialized { LINKER_INITIALIZED };
}  // base

//////////////////////////////////////////////////////////////////////
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

static const int8  kMaxInt8  = (static_cast<int8>(0x7f));
static const int8  kMinInt8  = (static_cast<int8>(0x80));
static const int16 kMaxInt16 = (static_cast<int16>(0x7fff));
static const int16 kMinInt16 = (static_cast<int16>(0x8000));
static const int32 kMaxInt32 = (static_cast<int32>(0x7fffffff));
static const int32 kMinInt32 = (static_cast<int32>(0x80000000));
static const int64 kMaxInt64 = (static_cast<int64>(0x7fffffffffffffffLL));
static const int64 kMinInt64 = (static_cast<int64>(0x8000000000000000LL));

static const uint8  kMaxUInt8  = (static_cast<uint8>(0xff));
static const uint16 kMaxUInt16 = (static_cast<uint16>(0xffff));
static const uint32 kMaxUInt32 = (static_cast<uint32>(0xffffffff));
static const uint64 kMaxUInt64 = (static_cast<uint64>(0xffffffffffffffffLL));

//////////////////////////////////////////////////////////////////////

typedef uint32 fourcc_t;

#define FOURCC(a, b, c, d) \
  (((uint32)(a) << 24) | ((uint32)(b) << 16) | ((uint32)(c) << 8) | (uint32)(d))

static const fourcc_t kInvalidFourCC = FOURCC('\0','\0','\0','\0');

//////////////////////////////////////////////////////////////////////

#define CONSIDER(cond) case cond: return #cond;
#define NUMBEROF(things) arraysize(things)

#define INVALID_FD_VALUE (-1)


#endif   // __WHISPERLIB_BASE_TYPES_H
