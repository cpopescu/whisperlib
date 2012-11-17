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

#ifndef __COMMON_BASE_DATE_H__
#define __COMMON_BASE_DATE_H__

#include <time.h>
#include <iostream>
#include <string>
#include <whisperlib/base/types.h>

namespace timer {

class Date {
 protected:
  // the number of milliseconds since January 1, 1970, 00:00:00 UTC
  int64 time_;

  // cached broken down time
  // FYI:
  // struct tm {
  //    int tm_sec;         // seconds (0..59)
  //    int tm_min;         // minutes (0..59)
  //    int tm_hour;        // hours (0..23)
  //    int tm_mday;        // day of the month (1..31)
  //    int tm_mon;         // month (0..11)
  //    int tm_year;        // number of years since 1900
  //    int tm_wday;        // day of the week (0..6)
  //    int tm_yday;        // day in the year (0..365)
  //    int tm_isdst;       // daylight saving time (flag:
  //                        //  - 1 is daylight saving time is in effect,
  //                        //  - 0 if it's not,
  //                        //  - negative if the information is not available)
  // };
  //
  struct tm broken_down_time_;

  // because struct tm does not have milisecond
  int broken_down_milisecond_;

  //  true  = the broken_down_time is expressed in Coordinated
  //          Universal Time (UTC).
  //        The logging and printing is also UTC.
  //  false = the broken_down_time is relative to local user's time zone.
  //        The logging and printing is also local.
  bool is_utc_;

  bool has_errors_;

 public:
  // 0 -> January
  // 1 -> February
  // ....
  static const char* MonthName(int month_number);

  // 0 -> Sunday
  // 1 -> Monday
  // 2 -> Tuesday
  // ....
  static const char* DayOfTheWeekName(int day_of_the_week_number);

  // returns: the number of milliseconds since January 1, 1970, 00:00:00 UTC
  static int64 Now();

  // returns: the time on a different day at the same 'time' (hour, minute, second).
  // For UTC time this is simple: just shift time by 'days' * 24 * 3600000 ms .
  // For local time: takes care of daylight saving time.
  static int64 ShiftDay(int64 time, int32 days, bool is_utc);

 public:
  // Initialize to current (now!) date & time.
  // If use_utc = true the broken down time will be stored in UTC format,
  // else local timezone is used.
  explicit Date(bool use_utc = false);

  // Initialize to the given moment.
  // time_in_ms_since_epoch = the number of milliseconds since the Epoch
  //          (January 1, 1970, 00:00:00 UTC)
  // use_utc = true: the broken down time will be stored at UTC time,
  //                 also Print and LogTo will write the time in UTC format.
  //           false: local timezone is used.
  //                  Print and LogTo will write the localtime.
  Date(int64 time_in_ms_since_epoch, bool use_utc = false);

  Date(int year, int month, int day, int hour,
       int minute, int second, int milisecond, bool use_utc = false);

  Date(const Date& date);

  virtual ~Date() {
  }

  //
  // Accessors
  //
  int GetYear() const { return 1900 + broken_down_time_.tm_year; }
  int GetMonth() const { return broken_down_time_.tm_mon; }
  int GetDay() const { return broken_down_time_.tm_mday; }
  int GetHour() const { return broken_down_time_.tm_hour; }
  int GetMinute() const { return broken_down_time_.tm_min; }
  int GetSecond() const { return broken_down_time_.tm_sec; }
  int GetMilisecond() const { return broken_down_milisecond_; }

  int GetDayOfTheWeek() const { return broken_down_time_.tm_wday; }
  int GetDayOfTheYear() const { return broken_down_time_.tm_yday; }

  const char* GetTimezoneName() const {
    return broken_down_time_.tm_zone;
  }
  const char* GetMonthName() const {
    return MonthName(GetMonth());
  }
  const char* GetDayOfTheWeekName() const {
    return DayOfTheWeekName(GetDayOfTheWeek());
  }

  // Returns ture iff the current broken down representation is set to UTC.
  bool IsUTC() const {  return is_utc_; }

  // Returns if the value set in the last operation was OK (all values were
  // valid);
  bool HasErrors() const { return has_errors_; }

  // returns: the number of milliseconds since January 1, 1970, 00:00:00 UTC
  //          represented by this date.
  int64 GetTime() const { return time_; }

  //
  // Settors
  //

  // Set the date to represent the given broken-down-time in UTC
  // or local timezone format.
  // Returns the succes value
  bool Set(int year, int month, int day, int hour,
           int minute, int second, int milisecond, bool use_utc = false);

  // Set the date to represent the specified number of milliseconds since the
  // standard base time known as "the epoch", namely (1/1/1970, 00:00:00 UTC)
  void SetTime(int64 time);

  // Assigns the current moment of time as the date & time in this object
  void SetNow();

  // sets the current broken down representation to UTC or Local Time.
  void SetUTC(bool use_utc);

  // Builds a date from a short string
  bool SetFromShortString(const string& s, bool is_utc);
  //
  // Operators
  //
  Date& operator=(const Date&);
  bool operator==(const Date&) const;

  string ToShortString() const;
  string ToString() const;
};

ostream& operator<<(ostream& os, const Date& date);
}


#endif   // __COMMON_BASE_DATE_H__
