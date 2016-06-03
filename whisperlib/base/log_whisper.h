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

//
// Whisperlib specifify log implementation.
//

#ifndef _WHISPERLIB_BASE_LOG_WHISPER_H
#define _WHISPERLIB_BASE_LOG_WHISPER_H

#if defined(USE_GLOG_LOGGING) && defined(HAVE_GLOG)
#error "Don't include this file if using google log !"
#endif

#include <unistd.h>
#include <ostream>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include "whisperlib/base/types.h"
#include "whisperlib/base/gflags.h"

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
#define LOG_INFO    if (false) whisper_base::null_stream()
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


#endif  //  _WHISPERLIB_BASE_LOG_WHISPER_H
