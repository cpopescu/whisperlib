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

#include <pthread.h>
#include <unistd.h>
#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/timer.h"

namespace timer {

#if defined(HAVE_MACH_TIMEBASE) || defined(HAVE_MACH_MACH_TIME_H)
#include <sys/time.h>
#include <mach/mach_time.h>

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
#include <sys/time.h>

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

}  // end namespace timer
