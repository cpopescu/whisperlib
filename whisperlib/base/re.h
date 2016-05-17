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
// Author: Catalin Popescu

// This is a wrapper around the POSIX regex library for an easy C++
// Usage examples:

#ifndef __WHISPERLIB_BASE_RE_H__
#define __WHISPERLIB_BASE_RE_H__

#include "whisperlib/base/types.h"
#include <string>
#include <vector>
#include <regex.h>

namespace whisper {
namespace re {

class RE {
 public:
  RE(const std::string& regex, bool ignore_case = false);
  ~RE();

  const std::string& regex() const { return regex_; }
  regex_t reg() const { return reg_; }

  /** Returns if the std::string matches - assumes no error in the regexp.  */
  bool Matches(const char* s) const;
  bool Matches(const std::string& s) const {
    return Matches(s.c_str());
  }
  /** Returns if the std::string matches - assumes no error in the regexp.  */
  bool MatchesNoErr(const char* s) const;
  bool MatchesNoErr(const std::string& s) const {
    return MatchesNoErr(s.c_str());
  }

  /** Matches the expression and advances past it in the return string ret. */
  bool MatchNext(const char* s, std::string* ret);
  bool MatchNext(const std::string& s, std::string* ret) {
    return MatchNext(s.c_str(), ret);
  }
  /** Resets the state for MatchNext */
  void Reset() {
    match_begin_ = true;
    match_.rm_eo = 0;
  }
  /** Matches and returns the first group. Empty  */
  std::string GroupMatch(const std::string& s) const;
  /** Matches a set of groups in out, as instructed by num. */
  bool GroupMatches(const std::string& s, std::vector<std::string>* out, size_t num) const;

  bool Replace(const char* s, const char* r, std::string& out) const;
  bool Replace(const std::string& s, const std::string& r, std::string& out) const;

  bool HasError() const { return err_ != 0; }
  int err() const { return err_; }
  const char* ErrorName() const { return ErrorName(err_); }
  static const char* ErrorName(int err);

 private:
  // the original expression
  const std::string regex_;
  // the compiled expression
  regex_t reg_;
  int err_;
  regmatch_t match_;
  bool match_begin_;
};
}  // namespace re
}  // namespace whisper

#endif  // __WHISPERLIB_BASE_RE_H__
