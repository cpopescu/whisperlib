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

#ifndef __WHISPERLIB_BASE_TYPES_H
#define __WHISPERLIB_BASE_TYPES_H

// for PRId64 , PRIu64 , ...
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <whisperlib/base/basictypes.h>

//////////////////////////////////////////////////////////////////////

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

#define CONSIDER(cond) case cond: return #cond;
#define NUMBEROF(things) arraysize(things)

#define INVALID_FD_VALUE (-1)

//////////////////////////////////////////////////////////////////////

// we accept 'std' and '__gnu_cxx' namespaces without full specification
// to reduce the clutter
namespace std {
namespace tr1 { }
}
namespace  __gnu_cxx { }
using namespace std;
using namespace std::tr1;
using namespace __gnu_cxx;

//////////////////////////////////////////////////////////////////////

// determine hash map includes

#if defined(HAVE_UNORDERED_SET)
#  define WHISPER_HASH_SET_HEADER <unordered_set>
#  define hash_set unordered_set
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
#  define hash_map unordered_map
#  define WHISPER_HASH_FUN_HEADER <functional_hash.h>
#  define WHISPER_HASH_FUN_NAMESPACE_BEGIN namespace std {
#  define WHISPER_HASH_FUN_NAMESPACE_END }
#  define WHISPER_HASH_FUN_NAMESPACE std
#elif defined(HAVE_TR1_UNORDERED_MAP)
#  define WHISPER_HASH_MAP_HEADER <tr1/unordered_map>
#  define hash_map tr1::unordered_map
#  define WHISPER_HASH_FUN_HEADER <tr1/functional_hash.h>
#  define WHISPER_HASH_FUN_NAMESPACE_BEGIN namespace std { namespace tr1 {
#  define WHISPER_HASH_FUN_NAMESPACE_END } }
#  define WHISPER_HASH_FUN_NAMESPACE std::tr1
#elif defined(HAVE_EXT_HASH_MAP)
#  define WHISPER_HASH_MAP_HEADER <ext/hash_map>
#  define WHISPER_HASH_FUN_HEADER <ext/hash_fun.h>
#  define WHISPER_HASH_FUN_NAMESPACE_BEGIN namespace __gnu_cxx {
#  define WHISPER_HASH_FUN_NAMESPACE_END }
#  define WHISPER_HASH_FUN_NAMESPACE __gnu_cxx
#else
#  error "Cannot find hash_map include"
#endif

#endif   // __WHISPERLIB_BASE_TYPES_H
