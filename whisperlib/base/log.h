// -*- Mode:c++; c-basic-offset:4; indent-tabs-mode:nil; coding:utf-8 -*-
// Copyright 2010 Google
//
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
// NOTE: contains modifications by giao and cp@urbanengines.com
//
// Copyright: Urban Engines, Inc. 2011 onwards
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
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
#ifndef _WHISPERLIB_BASE_LOG_H
#define _WHISPERLIB_BASE_LOG_H

#include "whisperlib/base/types.h"

#if defined(HAVE_GLOG)
#include <glog/logging.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#include <ostream>

#ifndef LIKELY_T   // EXPECT_TRUE clash with glog-3.3 fatal EXPECT_TRUE/FALSE
// Branch-prediction hint.  ONLY use these macros when absolutely sure.
#if defined(__GNUC__) || defined(__clang__) || defined(__has_builtin)
  #define LIKELY_T(cond)  (__builtin_expect(!!(cond), 1))  // predicts true
  #define LIKELY_F(cond)  (__builtin_expect(!!(cond), 0))  // predicts false
#else
  #define LIKELY_T(cond)  (cond)
  #define LIKELY_F(cond)  (cond)
#endif
#endif  // !def LIKELY_T


namespace whisper_base {
std::ostream& null_stream();
}  // namespace whisper_base


////////////////////////////////////////////////////////////////////////////////

// These may be defined in other packages we include - make sure
// we take our definitions.
#if defined(HAVE_GLOG)

#define LOG_INFO LOG(INFO)
#define LOG_WARN LOG(WARNING)
#define LOG_WARNING LOG(WARNING)
#define LOG_ERROR LOG(ERROR)
#define LOG_FATAL LOG(FATAL)
#define LG LOG_INFO

#ifdef DEBUG
#define LOG_DEBUG LOG_INFO
#else
#define LOG_DEBUG if (false) whisper_base::null_stream()
#endif

#undef  DCHECK_IS_FATAL
#ifndef NDEBUG
#define DCHECK_IS_FATAL
#endif

#define CHECK_NOT_NULL(p) CHECK_NOTNULL((p))

#else  // not HAVE_GLOG _______________________________________________

#include "whisperlib/base/log_whisper.h"
#ifndef NDEBUG
#define DCHECK_IS_FATAL
#endif

#include "whisperlib/base/log_checks.h"

#define CHECK_NOT_NULL(p) whisper_base::CheckNotNull( \
        __FILE__, __LINE__, "'" #p "' is NULL", (p))

#endif  // HAVE_GLOG


////////////////////////////////////////////////////////////////////////////////


//____________________ EXTRA definitions outside of glog _____________________//

#if !defined(DCHECK_IS_FATAL)

#define DLOG_INFO     if (false) LOG_INFO
#define DLOG_WARN     if (false) LOG_WARNING
#define DLOG_WARNING  if (false) LOG_WARNING
#define DLOG_ERROR    if (false) LOG_ERROR
#define LOG_DFATAL    LOG_ERROR

#else  // debug or DCHECK_IS_FATAL

#define DLOG_INFO    LOG_INFO
#define DLOG_WARN    LOG_WARNING
#define DLOG_WARNING LOG_WARNING
#define DLOG_ERROR   LOG_ERROR
#define LOG_DFATAL   LOG_FATAL

#endif  // DCHECK_IS_FATAL

// Conditional logging
#define LOG_INFO_IF(cond)     if (cond) LOG_INFO
#define LOG_WARN_IF(cond)     if (cond) LOG_WARNING
#define LOG_WARNING_IF(cond)  if (cond) LOG_WARNING
#define LOG_ERROR_IF(cond)    if (cond) LOG_ERROR
#define LOG_FATAL_IF(cond)    if (LIKELY_F(cond)) LOG_FATAL
#define LOG_DFATAL_IF(cond)   if (LIKELY_F(cond)) LOG_DFATAL

#define DLOG_INFO_IF(cond)    if (cond) DLOG_INFO
#define DLOG_WARN_IF(cond)    if (cond) DLOG_WARNING
#define DLOG_WARNING_IF(cond) if (cond) DLOG_WARNING
#define DLOG_ERROR_IF(cond)   if (cond) DLOG_ERROR
#define DLOG_FATAL_IF(cond)   if (LIKELY_F(cond)) DLOG_FATAL
#ifndef DVLOG
  #define DVLOG(x)            if ((x) <= FLAGS_v) DLOG_INFO
#endif

// Log the given value in hexadecimal with 0x prefix
#define LHEX(v) std::showbase << std::hex << (v) << std::dec << std::noshowbase

#define CHECK_SYS_FUN(a, b)                                             \
    do { const int __tmp_res__ = (a);                                   \
        if (LIKELY_F( __tmp_res__ != (b) )) {                           \
            LOG_FATAL << #a << " Failed w/ result: " << __tmp_res__     \
                      << " expected: " << #b;                           \
        }                                                               \
    } while ( false )


#define CHECK_NULL(p)                                                   \
    if (LIKELY_F((p) != nullptr))                                       \
        LOG_FATAL << " Pointer " << #p << "(" << LHEX(p) << ") is not NULL. "

#if !defined(DCHECK_IS_FATAL)
#define DCHECK_NOT_NULL(p)   DCHECK(p)
#define DCHECK_NULL(p)       DCHECK(!(p))
#else   // DEBUG
#define DCHECK_NOT_NULL(p)   CHECK_NOT_NULL(p)
#define DCHECK_NULL(p)       CHECK_NULL(p)
#endif


#endif  // _WHISPERLIB_BASE_LOG_H
