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

// NOTE: contains modifications by giao and cp@1618labs.com

#ifndef _WHISPERLIB_BASE_LOG_H
#define _WHISPERLIB_BASE_LOG_H

#include <stdio.h>

#ifdef ANDROID
#include <android/log.h>
#include <sstream>
#include <unistd.h>
#else
#include <pthread.h>
#endif

#include <whisperlib/base/gflags.h>

#if defined(_LOGGING_H_) || defined(HAVE_GLOG)

#include <glog/logging.h>

#define LOG_INFO LOG(INFO)
#define LOG_WARNING LOG(WARNING)
#define LOG_ERROR LOG(ERROR)
#define LOG_FATAL LOG(FATAL)
#define LG LOG(INFO)


#else  // not HAVE_GLOG

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>  // NOLINT

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

namespace ext_base {

#define LOG_PREFIX_INFO 'I'
#define LOG_PREFIX_WARNING 'W'
#define LOG_PREFIX_ERROR 'E'
#define LOG_PREFIX_FATAL 'F'

class LogMessage {
  public:
    LogMessage(const char* file, int line, char prefix)
        : prefix_(prefix)
#ifdef ANDROID
        , file_(file)
#endif
        {
#ifdef ANDROID
        oss << "1618labs: "
#else
        std::cerr
#endif
            << prefix << " [" << pretty_date_.HumanDate() << "] " << std::dec
#ifdef ANDROID
            << "[pid:" << getpid() << " / "
            << "tid:" << gettid() << "]"
#else
            << "[tid:" << pthread_self() << "]"
#endif
                  << file << ":" << line << ": ";
    }
    ~LogMessage() {
#ifdef ANDROID
        oss << "\n";
        __android_log_print(GetAndroidLogPriority(), file_, oss.str().c_str());
#else
        std::cerr << "\n";
        if (prefix_ == LOG_PREFIX_FATAL) {
            abort();
        }
#endif
    }
    std::ostream& stream() {
#ifdef ANDROID
        return oss;
#else
        return std::cerr;
#endif
    }

#ifdef ANDROID
    android_LogPriority GetAndroidLogPriority() {
      switch (prefix_) {
      case LOG_PREFIX_INFO: return ANDROID_LOG_INFO;
      case LOG_PREFIX_WARNING: return ANDROID_LOG_WARN;
      case LOG_PREFIX_ERROR: return ANDROID_LOG_ERROR;
      case LOG_PREFIX_FATAL: return ANDROID_LOG_FATAL;
      }
      return ANDROID_LOG_INFO;
    }
#endif

  private:
    struct DateLogger {
        char buffer_[9];
        DateLogger() {
#if defined(_MSC_VER)
            _tzset();
#endif
        }
        char* const HumanDate();
    };

    DateLogger pretty_date_;

 private:
    char prefix_;
#ifdef ANDROID
    const char* file_;
    std::ostringstream oss;
#endif

    // Disable unsafe copy/assign
    explicit LogMessage(const LogMessage&);
    void operator=(const LogMessage&);
};

}  // namespace ext_base

enum LogLevel {
    LogLevel_FATAL = 0,
    LogLevel_ERROR = 1,
    LogLevel_WARNING = 2,
    LogLevel_INFO = 3,
    LogLevel_DEBUG = 4,
};

inline void SetGlobalLogLevel(int level) {
    FLAGS_log_level = level;
}

#define LOG_INFO if (FLAGS_log_level >= LogLevel_INFO) ext_base::LogMessage(__FILE__, __LINE__, LOG_PREFIX_INFO).stream()
#define LG LOG_INFO
#define LOG_ERROR if (FLAGS_log_level >= LogLevel_ERROR) ext_base::LogMessage(__FILE__, __LINE__, LOG_PREFIX_ERROR).stream()
#define LOG_WARNING if (FLAGS_log_level >= LogLevel_WARNING) ext_base::LogMessage(__FILE__, __LINE__, LOG_PREFIX_WARNING).stream()
#define LOG_FATAL if (FLAGS_log_level >= LogLevel_FATAL) ext_base::LogMessage(__FILE__, __LINE__, LOG_PREFIX_FATAL).stream()
#define LOG_QFATAL LOG_FATAL

#ifndef VLOG
#define LOG(severity) LOG_ ## severity
#define LOG_EVERY_N(severity, n) LOG_ ## severity
#define VLOG(x) if ((x) <= FLAGS_v) LOG_INFO
#define VLOG_IS_ON(x) (0)
#endif  // VLOG


//////////////////////////////////////////////////////////////////////
// Conditional macros - a little better then ASSERTs

#define CHECK(cond)                                                     \
    if ( cond );                                                        \
    else                                                                \
        LOG_FATAL << #cond << " failed. "

#define DEFINE_CHECK_OP(name, op)                                       \
    template <class T1, class T2>                                       \
    inline void name(const T1& a, const T2& b, const char* _fn, int _line) { \
        if ( a op b );                                                  \
        else                                                            \
            ext_base::LogMessage(_fn, _line, LOG_PREFIX_FATAL).stream() \
                << "Check failed: [" << (a) << "] " #op " [" << (b) << "]"; \
  }

#define DEFINE_CHECK_FUN(name, fun, res)                                \
    template <class T1, class T2>                                       \
    inline void name(const T1& a, const T2& b, const char* _fn, int _line) { \
        if ( fun(a, b) == res );                                        \
        else                                                            \
            ext_base::LogMessage(_fn, _line, LOG_PREFIX_FATAL).stream() \
                << "Check failed: " << #fun "("                         \
                << a << "," << (b) << ") != " << res;                   \
  }

DEFINE_CHECK_OP(CHECK_EQ_FUN, ==)
DEFINE_CHECK_OP(CHECK_LT_FUN, <)
DEFINE_CHECK_OP(CHECK_LE_FUN, <=)
DEFINE_CHECK_OP(CHECK_GT_FUN, >)
DEFINE_CHECK_OP(CHECK_GE_FUN, >=)
DEFINE_CHECK_OP(CHECK_NE_FUN, !=)

#define CHECK_EQ(a,b) CHECK_EQ_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_LT(a,b) CHECK_LT_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_LE(a,b) CHECK_LE_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_GT(a,b) CHECK_GT_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_GE(a,b) CHECK_GE_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_NE(a,b) CHECK_NE_FUN((a),(b), __FILE__, __LINE__)

DEFINE_CHECK_FUN(CHECK_STREQ_FUN, strcmp, 0)
DEFINE_CHECK_FUN(CHECK_STRLT_FUN, strcmp, -1)
DEFINE_CHECK_FUN(CHECK_STRGT_FUN, strcmp, 1)
#define CHECK_STREQ(a,b) CHECK_STREQ_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_STRLT(a,b) CHECK_STRLT_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_STRGT(a,b) CHECK_STRGT_FUN((a),(b), __FILE__, __LINE__)

#ifdef NDEBUG
#define DCHECK(cond)    if ( false ) LOG_INFO
#define DCHECK_EQ(a, b) if ( false ) LOG_INFO
#define DCHECK_LT(a, b) if ( false ) LOG_INFO
#define DCHECK_LE(a, b) if ( false ) LOG_INFO
#define DCHECK_GT(a, b) if ( false ) LOG_INFO
#define DCHECK_GE(a, b) if ( false ) LOG_INFO
#define DCHECK_NE(a, b) if ( false ) LOG_INFO
#else
#define DCHECK(cond)   CHECK(cond)
#define DCHECK_EQ(a, b) CHECK_EQ(a, b)
#define DCHECK_LT(a, b) CHECK_LT(a, b)
#define DCHECK_LE(a, b) CHECK_LE(a, b)
#define DCHECK_GT(a, b) CHECK_GT(a, b)
#define DCHECK_GE(a, b) CHECK_GE(a, b)
#define DCHECK_NE(a, b) CHECK_NE(a, b)
#endif

// Defined here to break the CHECK macro dependency
namespace ext_base {
inline char* const LogMessage::DateLogger::HumanDate() {
#if defined(_MSC_VER)
     _strtime_s(buffer_, sizeof(buffer_));
#else
     time_t time_value = time(NULL);
     struct tm now;
     localtime_r(&time_value, &now);
     int n = snprintf(buffer_, sizeof(buffer_), "%02d:%02d:%02d",
                      now.tm_hour, now.tm_min, now.tm_sec);
     DCHECK_LT(n, sizeof(buffer_));
     buffer_[n] = 0;
#endif
     return buffer_;
}
}  // namespace ext_base

#endif  // not HAVE_GLOG


// Extra definitions outside of glog
#ifdef NDEBUG
#define LOG_DFATAL LOG_ERROR
#define DLOG_INFO  if (false) LOG_INFO
#define DLOG_ERROR  if (false) LOG_ERROR
#define DLOG_WARNING  if (false) LOG_WARNING
#ifndef DVLOG
  #define DVLOG(x) if (false) LOG_INFO
#endif

#else  // debug
#define LOG_DFATAL LOG_FATAL
#define DLOG_INFO LOG_INFO
#define DLOG_ERROR LOG_ERROR
#define DLOG_WARNING LOG_WARNING
#ifndef DVLOG
  #define DVLOG(x) if ((x) <= FLAGS_v) LOG_INFO
#endif
#endif


#define LOG_FATAL_IF(cond) if ( cond ) LOG_FATAL
#define LOG_ERROR_IF(cond) if ( cond ) LOG_ERROR
#define LOG_WARNING_IF(cond) if ( cond ) LOG_WARNING
#define LOG_INFO_IF(cond) if ( cond ) LOG_INFO

#define CHECK_SYS_FUN(a, b)                                             \
  do { const int __tmp_res__ = (a);                                     \
    if ( __tmp_res__ != (b) ) {                                         \
      LOG_FATAL << #a << " Failed w/ result: " << __tmp_res__           \
                << " expected: " << #b;                                 \
    }                                                                   \
  } while ( false )

// Log the given value in hexadecimal with 0x prefix
#define LHEX(v) showbase << hex << (v) << dec << noshowbase

#define CHECK_NOT_NULL(p)                                               \
    if ( (p) != NULL );                                                 \
    else                                                                \
        LOG_FATAL << " Pointer is NULL. "

#define CHECK_NULL(p)                                                   \
    if ( (p) == NULL );                                                 \
    else                                                                \
        LOG_FATAL << " Pointer " << #p << "(" << LHEX(p) << ") is not NULL. "

#endif  // _WHISPERLIB_BASE_LOG_H
