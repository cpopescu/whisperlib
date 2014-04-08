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

#include <whisperlib/base/types.h>
#include <string>
#include <regex.h>

namespace re {

class RE {
 public:
  RE(const string& regex, int cflags = 0);
  RE(const char* regex, int cflags = 0);
  ~RE();

  const string& regex() const { return regex_; }

  bool Matches(const char* s) const;
  bool Matches(const string& s) const {
    return Matches(s.c_str());
  }

  bool MatchNext(const char* s, string* ret);
  bool MatchNext(const string& s, string* ret) {
    return MatchNext(s.c_str(), ret);
  }
  void MatchEnd() {
    match_begin_ = true;
    match_.rm_eo = 0;
  }

  bool Replace(const char* s, const char* r, string& out);
  bool Replace(const string& s, const string& r, string& out);

  bool HasError() const { return err_ != 0; }
  int err() const { return err_; }
  const char* ErrorName() const { return ErrorName(err_); }
  static const char* ErrorName(int err);

 private:
  // the original expression
  const string regex_;
  // the compiled expression
  regex_t reg_;
  int err_;
  regmatch_t match_;
  bool match_begin_;
};
}

#endif  // __WHISPERLIB_BASE_RE_H__
