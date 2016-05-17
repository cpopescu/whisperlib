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

#ifndef __WHISPERLIB_BASE_TIMER_H__
#define __WHISPERLIB_BASE_TIMER_H__

#include <ctime>
#include <string>
#include "whisperlib/base/types.h"

namespace whisper {
namespace timer {

// returns: nanoseconds since an unspecified point in the past (for example,
//          system start-up time, or the Epoch). (uses CLOCK_MONOTONIC)
//          This value is not affected by sytem time changes and therefore
//          offers a reliable way of measuring time intervals.
int64 TicksNsec();

// returns: microseconds since an unspecified point in the past (for example,
// system start-up time, or the Epoch) (uses CLOCK_MONOTONIC)
inline int64 TicksUsec() {
  return TicksNsec() / 1000LL;
}
// returns: miliseconds since an unspecified point in the past (for example,
// system start-up time, or the Epoch) (uses CLOCK_MONOTONIC)
inline int64 TicksMsec() {
  return TicksNsec() / 1000000LL;
}

// returns: nanoseconds in CPU time. Uses CLOCK_PROCESS_CPUTIME_ID
int64 CpuNsec();

// Make the calling thread Sleep for "miliseconds" miliseconds.
// This call ignores signals.
void SleepMsec(int32 miliseconds);

// Returns a timespec that counts for the given milisecond interval
struct timespec TimespecFromMsec(int64 miliseconds);

// Reversed operation
int64 TimespecToMsec(struct timespec ts);

// Returns the timezone offset (in seconds) on this device/host
inline int TimezoneOffset() {
    time_t now = time(NULL);
    return now - mktime(gmtime(&now));
}

// Set moment: NOW + "shift_up_miliseconds"
// These methods should never fail. The only possbile failure is:
//  - uses CLOCK_REALTIME is not supported on this system, case when the
//    program will abort().
void SetTimespecAbsoluteMsec(struct timespec *ts,
                             int64 shift_up_miliseconds);
// Same as SetTimespecAbsoluteMsec but w/ a timestamp for interval
void SetTimespecAbsoluteTimespec(struct timespec* ts,
                                 const struct timespec& delta_ts);

// Another interface for SetTimespecAbsoluteMsec;
struct timespec TimespecAbsoluteMsec(int64 shift_up_miliseconds);

// Another interface for SetTimespecAbsoluteTimespec
void TimespecAbsoluteTimespec(struct timespec* ts,
                              const struct timespec& delta_ts);

/** Returns a human readable string describing the given 'duration_ms'
 * e.g. 83057 -> "1m23.057s" */
std::string StrHumanDuration(int64 duration_ms);

/** Returns a human readable time from time in ms - in local timezone */
std::string StrHumanTimeMs(int64 time_ms);
/** Returns a human readable time from time in ms - in utc timezone */
std::string StrHumanUtcTimeMs(int64 time_ms);

/** Returns a human readable time from time in sec - in local timezone */
std::string StrHumanTimeSec(time_t time_sec);
/** Returns a human readable time from time in sec - in utc timezone */
std::string StrHumanUtcTimeSec(time_t time_sec);

class MicroTimer {
public:
  MicroTimer() : start_ns_(0), accum_ns_(0) {
    Start();
  }

  void Start() {
    start_ns_ = TicksNsec();
  }
  bool IsRunning() const {
    return start_ns_ > 0;
  }
  void Stop() {
    if (not IsRunning())  return;
    accum_ns_ += TicksNsec() - start_ns_;
    start_ns_ = 0;
  }
  void Restart() {
    accum_ns_ = 0;
    start_ns_ = TicksNsec();
  }

  int64 nsec() const {
    if (IsRunning()) {
      return TicksNsec() - start_ns_ + accum_ns_;
    } else {
      return accum_ns_;
    }
  }
  double sec() const {
    return nsec() * 1e-9;
  }
  int64 msec() const {
    return static_cast<int64>(nsec() * 1e-6);
  }
  int64 usec() const {
    return static_cast<int64>(nsec() * 1e-3);
  }
  std::string ToString() const;

private:
  int64 start_ns_;
  int64 accum_ns_;
};
inline std::ostream& operator<<(std::ostream& os, const MicroTimer& tm) {
  return os << tm.ToString();
}

}   // namespace timer
}   // namespace whisper

#endif  // __WHISPERLIB_BASE_TIMER_H__
