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

#ifndef __WHISPERLIB_BASE_HASH_H
#define __WHISPERLIB_BASE_HASH_H
#pragma once

#include "whisperlib/base/types.h"

//////////////////////////////////////////////////////////////////////

// we accept 'std' and '__gnu_cxx' namespaces without full specification
// to reduce the clutter
namespace std { namespace tr1 { } }
// using namespace std::tr1;
namespace  __gnu_cxx { }
using namespace __gnu_cxx;
// namespace std {}
// using namespace std;

//////////////////////////////////////////////////////////////////////

// determine hash map includes

// For C++11: >= 201103;  For C98: == 199711
#if __cplusplus >= 201103       // c++11
 #if GCC_VER > 40500 || CLANG_VER > 30000
   #undef HAVE_TR1_UNORDERED_SET
   #undef HAVE_TR1_UNORDERED_MAP
   #undef HAVE_UNORDERED_SET
   #undef HAVE_UNORDERED_MAP
   #undef HAVE_FUNCTIONAL
   #define HAVE_UNORDERED_SET
   #define HAVE_UNORDERED_MAP
   #define HAVE_FUNCTIONAL
 #endif
#endif // c++11

#if defined(HAVE_UNORDERED_SET)
#  define WHISPER_HASH_SET_HEADER <unordered_set>
#  define hash_set std::unordered_set
#elif defined(HAVE_TR1_UNORDERED_SET)
#  define WHISPER_HASH_SET_HEADER <tr1/unordered_set>
#  define hash_set tr1::unordered_set
#elif defined(HAVE_EXT_HASH_SET)
#  define WHISPER_HASH_SET_HEADER <ext/hash_set>
#else
#  error "Cannot find hash_set include"
#endif

#if defined(HAVE_UNORDERED_MAP)
#  define WHISPER_HASH_MAP_HEADER <unordered_map>
#  define hash_map std::unordered_map
#  define hash_multimap std::unordered_multimap

#  if defined(HAVE_FUNCTIONAL_HASH_H)
#    define WHISPER_HASH_FUN_HEADER <functional_hash.h>
#  elif defined(HAVE_FUNCTIONAL)
#    define WHISPER_HASH_FUN_HEADER <functional>
#  else
#    error "Unknown functional hash header"
#  endif
#  define WHISPER_HASH_FUN_NAMESPACE_BEGIN namespace std {
#  define WHISPER_HASH_FUN_NAMESPACE_END }
#  define WHISPER_HASH_FUN_NAMESPACE std

#elif defined(HAVE_TR1_UNORDERED_MAP)
#  define WHISPER_HASH_MAP_HEADER <tr1/unordered_map>
#  define hash_map tr1::unordered_map
#  define hash_multimap tr1::unordered_multimap
#  define WHISPER_HASH_FUN_HEADER <tr1/functional_hash.h>
#  define WHISPER_HASH_FUN_NAMESPACE_BEGIN namespace std { namespace tr1 {
#  define WHISPER_HASH_FUN_NAMESPACE_END } }
#  define WHISPER_HASH_FUN_NAMESPACE std::tr1

#elif defined(HAVE_EXT_HASH_MAP)
#  define WHISPER_HASH_MAP_HEADER <ext/hash_map>
#  ifdef (HAVE_EXT_HASH_FUN_H)
#     define WHISPER_HASH_FUN_HEADER <ext/hash_fun.h>
#  else
#     error "Unknown hash function header"
#  endif
#  define WHISPER_HASH_FUN_NAMESPACE_BEGIN namespace __gnu_cxx {
#  define WHISPER_HASH_FUN_NAMESPACE_END }
#  define WHISPER_HASH_FUN_NAMESPACE __gnu_cxx

#else
#  error "Cannot find hash_map include"
#endif

#endif // __WHISPERLIB_BASE_HASH_H
