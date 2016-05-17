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

#include "whisperlib/base/re.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/scoped_ptr.h"

namespace whisper {
namespace re {

const char* RE::ErrorName(int err) {
  switch ( err ) {
  case 0: return "REG_OK";
  case REG_NOMATCH:
    return "REG_NOMATCH : regexec() failed to match.";
  case REG_BADPAT:
    return "REG_BADPAT : Invalid regular expression.";
  case REG_ECOLLATE:
    return "REG_ECOLLATE : Invalid collating element referenced.";
  case REG_ECTYPE:
    return "REG_ECTYPE : Invalid character class type referenced.";
  case REG_EESCAPE:
    return "REG_EESCAPE : Trailing '\\' in pattern.";
  case REG_ESUBREG:
    return "REG_ESUBREG : Number in \"\\digit\" invalid or in error.";
  case REG_EBRACK:
    return "REG_EBRACK : \"[]\" imbalance.";
  case REG_EPAREN:
    return "REG_EPAREN : \"\\(\\)\" or \"()\" imbalance.";
  case REG_EBRACE:
    return "REG_EBRACE : \"\\{\\}\" imbalance.";
  case REG_BADBR:
    return "REG_BADBR : Content of \"\\{\\}\" invalid: not a number, "
      "number too large, more than two numbers, first larger than second.";
  case REG_ERANGE:
    return "REG_ERANGE : Invalid endpoint in range expression.";
  case REG_ESPACE:
    return "REG_ESPACE : Out of memory.";
  case REG_BADRPT:
    return "REG_BADRPT : '?', '*', or '+' not preceded by valid "
      "regular expression.";
  }
  return "REG_UNKNOWN: unknown error";
}

RE::RE(const std::string& regex, bool ignore_case)
  : regex_(regex),
    err_(0),
    match_begin_(true) {
  match_.rm_eo = 0;
  err_ = regcomp(&reg_, regex.c_str(), REG_EXTENDED | (ignore_case ? REG_ICASE : 0));
}
RE::~RE() {
  regfree(&reg_);
}

bool RE::Matches(const char* s) const {
  if ( err_ ) return false;
  return regexec(&reg_, s, size_t(0), NULL, 0) == 0;
}
bool RE::MatchesNoErr(const char* s) const {
  DCHECK(!err_);
  return regexec(&reg_, s, size_t(0), NULL, 0) == 0;
}

bool RE::MatchNext(const char* s, std::string* ret) {
  if ( err_ ) return false;
  const int begin = match_begin_ ? 0 : match_.rm_eo;
  if ( !*(s + match_.rm_eo) )
    return false;
  const int status = regexec(&reg_, s + match_.rm_eo, 1, &match_,
                             match_begin_ ? 0 : REG_NOTBOL);
  if ( status != 0 ) {
    Reset();
    return false;
  }
  match_.rm_so += begin;
  match_.rm_eo += begin;
  ret->assign(s + match_.rm_so, match_.rm_eo - match_.rm_so);
  match_begin_ = false;
  return true;
}

std::string RE::GroupMatch(const std::string& s) const {
  regmatch_t match[2];
  const int status = regexec(&reg_, s.c_str(), 2, match, 0);
  if (status || match[1].rm_so < 0) { return std::string(); }
  return s.substr(match[1].rm_so, match[1].rm_eo - match[0].rm_so);
}

bool RE::GroupMatches(const std::string& s, std::vector<std::string>* out, size_t num) const {
  scoped_array<regmatch_t> match(new regmatch_t[num + 1]);
  const int status = regexec(&reg_, s.c_str(), num + 1, match.get(), 0);
  if (status) { return false; }
  for (size_t i = 0; i < num; ++i) {
      if (match[i + 1].rm_so < 0) out->push_back(std::string());
      else out->push_back(s.substr(match[i + 1].rm_so, match[i + 1].rm_eo - match[i + 1].rm_so));
  }
  return true;
}


bool RE::Replace(const char* s, const char* r, std::string& out) const {
  regmatch_t matches[10];
  const int status = regexec(&reg_, s, 10, matches, 0);

  if ( status != 0 ) {
    return false;
  }

  out.clear();

  bool ind = false;
  for (const char *c = r; *c; ++c) {
    switch (*c) {
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if (ind) {
          const regmatch_t &match = matches[*c-'0'];
          if (match.rm_so >=0) {
            out += std::string(s+match.rm_so, match.rm_eo-match.rm_so);
          }
          ind = false;
        } else {
          out += *c;
        }
        break;
      case '$':
        if (ind) {
          out += '$';
          ind = false;
        } else {
          ind = true;
        }
        break;
      default:
        out += *c;
    }
  }

  return true;
}
bool RE::Replace(const std::string& s, const std::string& r, std::string& out) const {
  return Replace(s.c_str(), r.c_str(), out);
}
}  // namespace re
}  // namespace whisper
