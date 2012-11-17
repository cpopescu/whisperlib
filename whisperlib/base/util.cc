// Copyright (c) 2011, Whispersoft s.r.l.
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

#include <whisperlib/base/util.h>
#include <whisperlib/base/strutil.h>

namespace util {

Interval::Interval(const Interval& other)
    : min_(other.min_), max_(other.max_) {}
Interval::Interval(int32 min, int32 max)
    : min_(min), max_(max) {}

bool Interval::Read(const string& s) {
  pair<string, string> p = strutil::SplitFirst(s, "-");
  string s_min = strutil::StrTrim(p.first);
  string s_max = strutil::StrTrim(p.second);
  min_ = ::atoll(s_min.c_str());
  max_ = ::atoll(s_max.c_str());
  if ( (min_ == 0 && s_min != "" && s_min[0] != '0') ||
       (max_ == 0 && s_max != "" && s_max[0] != '0') ) {
    LOG_ERROR << "Invalid number in interval: [" << s << "]";
    return false;
  }
  if ( max_ == 0 ) {
    max_ = min_;
  }
  if ( min_ > max_ ) {
    LOG_ERROR << "Invalid interval, number order: [" << s << "]";
    return false;
  }
  return true;
}

int32 Interval::Rand(unsigned int* seed) const {
  CHECK_GE(max_, min_);
  if ( min_ == max_ ) {
    return min_;
  }
#ifdef ANDROID
  // No rand_r on android
  int rnd = ::rand();
#else
  int rnd = (seed == NULL ? ::rand() : ::rand_r(seed));
#endif
  return min_ + (rnd % (max_ - min_));
}

}
