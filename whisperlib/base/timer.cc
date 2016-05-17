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
// Author: Cosmin Tudorache

#include <sys/time.h>
#include <assert.h>
#include <iomanip>
#include <pthread.h>
#include <sstream>
#include <unistd.h>

#include "whisperlib/base/types.h"
#include "whisperlib/base/timer.h"

#if defined(HAVE_MACH_TIMEBASE)
#include <mach/mach_time.h>
#endif

#ifndef CHECK_EQ
#define CHECK_EQ(a, b) assert(a == b)
#define CHECK_GE(a, b) assert(a >= b)
#endif

namespace whisper {
namespace timer {

#if defined(HAVE_MACH_TIMEBASE)

static mach_timebase_info_data_t timer_info = {0,0};
pthread_once_t  glb_once_control = PTHREAD_ONCE_INIT;
static void init_timer_info() {
    mach_timebase_info(&timer_info);
}

int64 TicksNsec() {
  uint64 m_time = mach_absolute_time();
  if (timer_info.denom == 0) {
    pthread_once(&glb_once_control, init_timer_info);
  }
  return m_time * (timer_info.numer / timer_info.denom);
}
int64 CpuNsec() {
    return TicksNsec();
}

#elif defined(HAVE_CLOCK_GETTIME)

int64 TicksNsec() {
  struct timespec ts = { 0, };
  const int result = clock_gettime(CLOCK_MONOTONIC, &ts);
  CHECK_EQ(result, 0);
  const int64 time_nsec = ts.tv_sec * 1000000000LL + ts.tv_nsec;
  return time_nsec;
}
int64 CpuNsec() {
    struct timespec ts = { 0, };
    const int result = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    CHECK_EQ(result, 0);
    const int64 time_nsec = ts.tv_sec * 1000000000LL + ts.tv_nsec;
    return time_nsec;
}

#elif defined(HAVE_GETTIMEOFDAY)

int64 TicksNsec() {
  struct timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec * 1000000000LL + now.tv_usec * 1000LL;
}
int64 CpuNsec() {
    return TicksNsec();
}

#else
#error "No way to implement TicksNsec"
#endif

void SleepMsec(int32 miliseconds) {
  while ( miliseconds > 0 ) {
    const int64 ms_start = TicksMsec();
    // usleep sleeps less then 1000000 usecs - need to split in
    // sleep and usleep.
    const int64 seconds = miliseconds / 1000;
    const int64 useconds = (miliseconds % 1000) * 1000;
    if ( seconds > 0 ) {
      ::sleep(seconds);
    } else {
      ::usleep(useconds);
    }
    const int64 ms_end = TicksMsec();
    CHECK_GE(ms_end, ms_start);
    const int64 elapsed = ms_end - ms_start;
    miliseconds -= elapsed;
  }
}

struct timespec TimespecFromMsec(int64 miliseconds) {
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(miliseconds / 1000LL);
  ts.tv_nsec = static_cast<long>((miliseconds % 1000LL) * 1000000LL);
  return ts;
}
int64 TimespecToMsec(struct timespec ts) {
  return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}
void SetTimespecAbsoluteTimespec(struct timespec* ts,
                                 const struct timespec& delta_ts) {
#if defined(HAVE_CLOCK_GETTIME)
  const int result = clock_gettime(CLOCK_REALTIME, ts);
  CHECK_EQ(result, 0);
#elif defined(HAVE_GETTIMEOFDAY)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  ts->tv_sec = tv.tv_sec;
  ts->tv_nsec = tv.tv_usec * 1000LL;
#else
  #error "No way to implement SetTimespecAbsoluteTimespec"
#endif

  ts->tv_sec += delta_ts.tv_sec;
  ts->tv_nsec += delta_ts.tv_nsec;

  while ( ts->tv_nsec > 999999999 ) {
    ts->tv_nsec -= 1000000000;
    ts->tv_sec++;
  }
}

void SetTimespecAbsoluteMsec(struct timespec *ts,
                             int64 shift_up_miliseconds) {
  struct timespec delta(TimespecFromMsec(shift_up_miliseconds));
  SetTimespecAbsoluteTimespec(ts, delta);
}

struct timespec
TimespecAbsoluteTimespec(const struct timespec& delta_ts) {
  struct timespec ts;
  SetTimespecAbsoluteTimespec(&ts, delta_ts);
  return ts;
}

struct timespec TimespecAbsoluteMsec(int64 shift_up_miliseconds) {
  struct timespec ts(TimespecFromMsec(shift_up_miliseconds));
  return TimespecAbsoluteTimespec(ts);
}

namespace {
std::string StrTimeTm(const struct tm& now, int64 ms) {
  std::ostringstream oss;
  oss << std::setfill('0')
      << (1900 + now.tm_year) << "/"
      << std::setw(2) << 1 + now.tm_mon << "/"
      << std::setw(2) << now.tm_mday
      << ' '
      << std::setw(2) << now.tm_hour  << ':'
      << std::setw(2) << now.tm_min   << ':'
      << std::setw(2) << now.tm_sec;
  if (ms >= 0) {
    oss << "." << std::setw(3) << ms;
  }
  return oss.str();
}

}  // namespace

std::string StrHumanTimeMs(int64 time_ms) {
  time_t val = time_ms / 1000;
  struct tm now;
  localtime_r(&val, &now);
  return StrTimeTm(now, time_ms % 1000);
}

std::string StrHumanUtcTimeMs(int64 time_ms) {
  time_t val = time_ms / 1000;
  struct tm now;
  gmtime_r(&val, &now);
  return StrTimeTm(now, time_ms % 1000);
}

std::string StrHumanTimeSec(time_t time_sec) {
  struct tm now;
  localtime_r(&time_sec, &now);
  return StrTimeTm(now, -1);
}

std::string StrHumanUtcTimeSec(time_t time_sec) {
  struct tm now;
  gmtime_r(&time_sec, &now);
  return StrTimeTm(now, -1);
}

std::string StrHumanDuration(int64 duration_ms) {
  std::ostringstream oss;
  if (duration_ms > 3600000) {
    oss << (duration_ms / 3600000) << "h";
    duration_ms = duration_ms % 3600000;
  }
  if (duration_ms > 60000) {
    oss << (duration_ms / 60000) << "m";
    duration_ms = duration_ms % 60000;
  }
  oss << std::setprecision(3);
  oss << (duration_ms / 1000.0) << "s";
  return oss.str();
}

std::string MicroTimer::ToString() const {
    return StrHumanDuration(msec());
}

}  // end namespace timer
}  // end namespace whisper
