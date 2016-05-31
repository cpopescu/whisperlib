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

#include <unistd.h>
#include <ostream>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#include "whisperlib/base/types.h"
#include "whisperlib/base/gflags.h"

#ifndef NDEBUG
#define DCHECK_IS_FATAL
#endif

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
struct NullBuffer : std::streambuf {
    int overflow(int c) { return c; }
};
static NullBuffer null_buffer;
static std::ostream null_stream(&null_buffer);
}  // namespace whisper_base

#if defined(USE_GLOG_LOGGING) && defined(HAVE_GLOG)  // && defined(_LOGGING_H_)
#include <glog/logging.h>
#define LOG_INFO LOG(INFO)
#define LOG_WARN LOG(WARNING)
#define LOG_WARNING LOG(WARNING)
#define LOG_ERROR LOG(ERROR)
#define LOG_FATAL LOG(FATAL)
#define LG LOG_INFO
#ifdef DEBUG
#define LOG_DEBUG LOG_INFO
#else
#define LOG_DEBUG if (false) whisper_base::null_stream
#endif

#else  // not USE_GLOG_LOGGING _______________________________________________

// These may be defined in other packages we include - make sure
// we take our definitions.
#undef LOG
#undef VLOG_IS_ON
#undef CHECK
#undef CHECK_OP_LOG
#undef CHECK_OP
#undef CHECK_EQ
#undef CHECK_NE
#undef CHECK_LE
#undef CHECK_LT
#undef CHECK_GE
#undef CHECK_GT
#undef CHECK_NOTNULL
#undef DCHECK
#undef DCHECK_OP_LOG
#undef DCHECK_OP
#undef DCHECK_EQ
#undef DCHECK_NE
#undef DCHECK_LE
#undef DCHECK_LT
#undef DCHECK_GE
#undef DCHECK_GT
#undef DCHECK_NOTNULL
#undef VLOG

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>              // ostringstream
#include <string.h>

#ifndef GLOG_NAMESPACE
#define GLOG_NAMESPACE google
#endif

namespace GLOG_NAMESPACE {
    void InitGoogleLogging(const char* arg0);
    void InstallFailureSignalHandler();
}

/// These flags are declared to match GLOG library, however have not much effect
// We should leave these here ase the glog defines this in the h file - do not remove
DECLARE_int32(v);    // Allow .cc filw to use VLOG without DECLARE_int32(v)
DECLARE_int32(log_level);
DECLARE_bool(logtostderr);
DECLARE_bool(alsologtostderr);

enum LogLevel {
    LogLevel_FATAL = 0,
    LogLevel_ERROR = 1,
    LogLevel_WARNING = 2,
    LogLevel_INFO = 3,
    LogLevel_DEBUG = 4,
};
inline void SetGlobalLogLevel(int level) { FLAGS_log_level = level; }


namespace whisper_base {

#define LOG_PREFIX_INFO 'I'
#define LOG_PREFIX_WARNING 'W'
#define LOG_PREFIX_ERROR 'E'
#define LOG_PREFIX_FATAL 'F'
#define LOG_PREFIX_DEBUG 'D'

class LogPrefix {
  public:
    LogPrefix(const char* file, int line, char prefix)
        : prefix_(prefix)
#ifdef ANDROID
        , file_(file)
#endif
    {
        oss << prefix << " [";
        pretty_date_.HumanDate(oss) << "] " << std::dec
#ifdef ANDROID
            << "[tid:" << gettid() << "] "
#else
            << "[tid:" << pthread_self() << "] "
#endif
            << file << ":" << line << ": ";
    }
    ~LogPrefix() {
        oss << "\n";
#ifdef ANDROID
        __android_log_print(GetAndroidLogPriority(),  "Urban Engines", oss.str().c_str());
#else
        const std::string s(oss.str());
        fwrite(s.c_str(), 1, s.length(), stderr);
        // For some reason, the libc++ has some issues w/ this (as opposed to libstdc++
        // Keep it to fprintf - the safe thing.
        // std::cerr << oss.str();
        if (LIKELY_F(prefix_ == LOG_PREFIX_FATAL)) {
            abort();
        }
#endif
    }
    std::ostream& stream() {
        return oss;
    }

#ifdef ANDROID
    android_LogPriority GetAndroidLogPriority() {
      switch (prefix_) {
      case LOG_PREFIX_INFO: return ANDROID_LOG_INFO;
      case LOG_PREFIX_WARNING: return ANDROID_LOG_WARN;
      case LOG_PREFIX_ERROR: return ANDROID_LOG_ERROR;
      case LOG_PREFIX_FATAL: return ANDROID_LOG_FATAL;
      case LOG_PREFIX_DEBUG: return ANDROID_LOG_INFO;
      }
      return ANDROID_LOG_INFO;
    }
#endif

  private:
    struct DateLogger {

        DateLogger() {
#if defined(_MSC_VER)
            _tzset();
#endif
        }
        std::ostream& HumanDate(std::ostream& str) const;
    };

    DateLogger pretty_date_;

 private:
    char prefix_;
    std::ostringstream oss;
#ifdef ANDROID
    const char* file_;
#endif

    // Disable unsafe copy/assign
    explicit LogPrefix(const LogPrefix&);
    void operator=(const LogPrefix&);
};

// Helper for CHECK_NOT_NULL
template<class T> T CheckNotNull(const char* file, int line, const char* msg, T p) {
    if (LIKELY_F(p == nullptr)) {
        LogPrefix(file, line, LOG_PREFIX_INFO).stream() << msg;
    }
    return p;
}
}  // namespace whisper_base


#ifdef DISABLE_LOGGING   // RC build
#define LOG_INFO    if (false) whisper_base::null_stream
#define LOG_WARN    LOG_INFO
#define LOG_WARNING LOG_INFO
#define LOG_ERROR   LOG_INFO
#define LOG_DEBUG   LOG_INFO

#else   // ! DISABLE_LOGGING
#define LOG_INFO    if (LIKELY_T(FLAGS_log_level >= LogLevel_INFO))     \
        whisper_base::LogPrefix(__FILE__, __LINE__, LOG_PREFIX_INFO).stream()
#define LOG_WARN    if (LIKELY_T(FLAGS_log_level >= LogLevel_WARNING))  \
    whisper_base::LogPrefix(__FILE__, __LINE__, LOG_PREFIX_WARNING).stream()
#define LOG_WARNING if (LIKELY_T(FLAGS_log_level >= LogLevel_WARNING))  \
    whisper_base::LogPrefix(__FILE__, __LINE__, LOG_PREFIX_WARNING).stream()
#define LOG_ERROR   if (LIKELY_T(FLAGS_log_level >= LogLevel_ERROR))    \
    whisper_base::LogPrefix(__FILE__, __LINE__, LOG_PREFIX_ERROR).stream()
#define LOG_DEBUG   if (FLAGS_log_level >= LogLevel_DEBUG))             \
    whisper_base::LogPrefix(__FILE__, __LINE__, LOG_PREFIX_DEBUG).stream()
#endif  // DISABLE_LOGGING

#define LOG_FATAL   if (LIKELY_T(FLAGS_log_level >= LogLevel_FATAL))    \
    whisper_base::LogPrefix(__FILE__, __LINE__, LOG_PREFIX_FATAL).stream()
#define LOG_QFATAL  LOG_FATAL

#define LG LOG_INFO
#define LOG(severity)  LOG_ ## severity
#define LOG_IF(severity, cond)   if (cond) LOG_ ## severity
#define DLOG_IF(severity, cond)  if (cond) DLOG_ ## severity
#define LOG_EVERY_N(severity, n) LOG_ ## severity

#ifndef VLOG
#define VLOG_IS_ON(x)  ((x) <= FLAGS_v)
#define VLOG(x)        if (VLOG_IS_ON(x)) LOG_INFO
#endif  // VLOG


//////////////////////////////////////////////////////////////////////
// Conditional macros - a little better then ASSERTs

#define CHECK(cond)  if (!LIKELY_T(cond)) LOG_FATAL << #cond << " FAILED! "

#define DEFINE_CHECK_OP(name, op)                                       \
    template <class T1, class T2>                                       \
    inline bool name(const T1& a, const T2& b) {                        \
        if (LIKELY_T( a op b )) return true;                            \
        return false;                                                   \
    }

#define DEFINE_CHECK_FUN(name, fun, res)                                \
    template <class T1, class T2>                                       \
    inline bool name(const T1& a, const T2& b) {                        \
        if (LIKELY_T( fun(a, b) == res )) return true;                  \
        return false;                                                   \
    }

DEFINE_CHECK_OP(CHECK_EQ_FUN, ==)
DEFINE_CHECK_OP(CHECK_LT_FUN, <)
DEFINE_CHECK_OP(CHECK_LE_FUN, <=)
DEFINE_CHECK_OP(CHECK_GT_FUN, >)
DEFINE_CHECK_OP(CHECK_GE_FUN, >=)
DEFINE_CHECK_OP(CHECK_NE_FUN, !=)

#define PERFORM_CHECK_OP(name, op, a, b)                                \
    if (!LIKELY_T(name((a), (b))))                                      \
        LOG_FATAL << "Check failed: [" << (a) << "] " #op " [" << (b) << "]"
#define CHECK_EQ(a,b) PERFORM_CHECK_OP(CHECK_EQ_FUN, ==, (a),(b))
#define CHECK_LT(a,b) PERFORM_CHECK_OP(CHECK_LT_FUN, <, (a),(b))
#define CHECK_LE(a,b) PERFORM_CHECK_OP(CHECK_LE_FUN, <=, (a),(b))
#define CHECK_GT(a,b) PERFORM_CHECK_OP(CHECK_GT_FUN, >, (a),(b))
#define CHECK_GE(a,b) PERFORM_CHECK_OP(CHECK_GE_FUN, >=, (a),(b))
#define CHECK_NE(a,b) PERFORM_CHECK_OP(CHECK_NE_FUN, !=, (a),(b))

DEFINE_CHECK_FUN(CHECK_STREQ_FUN, strcmp, 0)
DEFINE_CHECK_FUN(CHECK_STRLT_FUN, strcmp, -1)
DEFINE_CHECK_FUN(CHECK_STRGT_FUN, strcmp, 1)

#define PERFORM_CHECK_FUN(name, a, b)                                   \
    if (!LIKELY_T(name((a), (b))))                                      \
        LOG_FATAL << "Check failed: " << #name << "(" << (a) << ", " << (b) << ")"
#define CHECK_STREQ(a,b) PERFORM_CHECK_FUN(CHECK_STREQ_FUN, (a), (b))
#define CHECK_STRLT(a,b) PERFORM_CHECK_FUN(CHECK_STRLT_FUN, (a), (b))
#define CHECK_STRGT(a,b) PERFORM_CHECK_FUN(CHECK_STRGT_FUN, (a), (b))


#if !defined(DCHECK_IS_FATAL)
#define DCHECK(cond)    if (false) LOG_INFO
#define DCHECK_EQ(a, b) if (false) LOG_INFO
#define DCHECK_LT(a, b) if (false) LOG_INFO
#define DCHECK_LE(a, b) if (false) LOG_INFO
#define DCHECK_GT(a, b) if (false) LOG_INFO
#define DCHECK_GE(a, b) if (false) LOG_INFO
#define DCHECK_NE(a, b) if (false) LOG_INFO
#else
#define DCHECK(cond)    CHECK(cond)
#define DCHECK_EQ(a, b) CHECK_EQ(a, b)
#define DCHECK_LT(a, b) CHECK_LT(a, b)
#define DCHECK_LE(a, b) CHECK_LE(a, b)
#define DCHECK_GT(a, b) CHECK_GT(a, b)
#define DCHECK_GE(a, b) CHECK_GE(a, b)
#define DCHECK_NE(a, b) CHECK_NE(a, b)
#endif

#endif  // USE_GLOG_LOGGING



//____________________ EXTRA definitions outside of glog _____________________//

#if !defined(DCHECK_IS_FATAL)
#define DLOG_INFO  if (false) LOG_INFO
#define DLOG_WARN  if (false) LOG_WARNING
#define DLOG_WARNING  if (false) LOG_WARNING
#define DLOG_ERROR  if (false) LOG_ERROR
#define LOG_DFATAL LOG_ERROR
#else  // debug or DCHECK_IS_FATAL
#define DLOG_INFO LOG_INFO
#define DLOG_WARN LOG_WARNING
#define DLOG_WARNING LOG_WARNING
#define DLOG_ERROR LOG_ERROR
#define LOG_DFATAL LOG_FATAL
#endif  // DCHECK_IS_FATAL

// Conditional logging
#define LOG_INFO_IF(cond)     if (cond) LOG_INFO
#define LOG_WARN_IF(cond)  if (cond) LOG_WARNING
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


#define CHECK_SYS_FUN(a, b)                                             \
    do { const int __tmp_res__ = (a);                                   \
        if (LIKELY_F( __tmp_res__ != (b) )) {                           \
            LOG_FATAL << #a << " Failed w/ result: " << __tmp_res__     \
                      << " expected: " << #b;                           \
        }                                                               \
    } while ( false )

// Log the given value in hexadecimal with 0x prefix
#define LHEX(v) std::showbase << std::hex << (v) << std::dec << std::noshowbase

#define CHECK_NULL(p)                                                   \
    if (LIKELY_F(p)) LOG_FATAL << " Pointer " << #p << "(" << LHEX(p) << ") is not NULL. "

//#ifdef CHECK_NOTNULL
// #define CHECK_NOT_NULL(p) CHECK_NOTNULL(p)
#if defined(LOG_PREFIX_INFO)
 #define CHECK_NOT_NULL(p) whisper_base::CheckNotNull(__FILE__, __LINE__, "'" #p "' is NULL", (p))
#else
 #define CHECK_NOT_NULL(p) (CHECK(p != NULL), p)
#endif


#if !defined(DCHECK_IS_FATAL)
#define DCHECK_NOT_NULL(p)   DCHECK(p)
#define DCHECK_NULL(p)       DCHECK(!(p))
#define DCHECK_SYS_FUN(a, b) (a, b)   // TODO(giao): support stream
#else   // DEBUG
#define DCHECK_NOT_NULL(p)   CHECK_NOT_NULL(p)
#define DCHECK_NULL(p)       CHECK_NULL(p)
#define DCHECK_SYS_FUN(a, b) CHECK_SYS_FUN(a, b)
#endif


#endif  // _WHISPERLIB_BASE_LOG_H
