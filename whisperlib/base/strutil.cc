// -*- Mode:c++; c-basic-offset:2; indent-tabs-mode:nil; coding:utf-8 -*-
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
//

#include <stdarg.h>
#include <stdio.h>
#include <memory>

#include "whisperlib/base/strutil.h"
#include "whisperlib/base/scoped_ptr.h"

namespace strutil {

// Convert binary string to hex
string ToHex(const unsigned char* cp, int len) {
  char buf[len * 2 + 1];
  for (int i = 0; i < len; ++i) {
    sprintf(buf + i * 2, "%02x", cp[i]);
  }
  return string(buf, len * 2);
}

string ToHex(const string& str) {
  return ToHex(reinterpret_cast<const unsigned char*>(str.data()), str.size());
}


bool StrEql(const char* str1, const char* str2) {
  return (str1 == str2 ||
          0 == strcmp(str1, str2));
}

bool StrEql(const string& str1, const string& str2) {
  return str1 == str2;
}

bool StrIEql(const char* str1, const char* str2) {
  return str1 == str2 || 0 == strcasecmp(str1, str2);
}

bool StrIEql(const string& str1, const string& str2) {
  return 0 == ::strcasecmp(str1.c_str(), str2.c_str());
}

bool StrPrefix(const char* str, const char* prefix) {
  if ( str == prefix ) {
    return true;
  }
  while ( *str && *prefix && *str == *prefix ) {
    str++;
    prefix++;
  }
  return *prefix == '\0';
}

bool StrStartsWith(const char* str, const char* prefix) {
  return StrPrefix(str, prefix);
}

bool StrStartsWith(const string& str, const string& prefix) {
  return StrStartsWith(str.c_str(), prefix.c_str());
}

bool StrCasePrefix(const char* str, const char* prefix) {
  if ( str == prefix ) {
    return true;
  }
  while ( *str && *prefix && (*str == *prefix ||
                              ::toupper(*str) == ::toupper(*prefix)) ) {
    str++;
    prefix++;
  }
  return *prefix == '\0';
}

bool StrIStartsWith(const char* str, const char* prefix) {
  return StrCasePrefix(str, prefix);
}

bool StrIStartsWith(const string& str, const string& prefix) {
  return StrIStartsWith(str.c_str(), prefix.c_str());
}

bool StrSuffix(const char* str, size_t len,
               const char* suffix, size_t suffix_len) {
  if ( suffix_len > len )
    return false;
  return 0 == memcmp(str + len - suffix_len, suffix, suffix_len);
}

bool StrSuffix(const char* str, const char* suffix) {
  return StrSuffix(str, strlen(str), suffix, strlen(suffix));
}

bool StrSuffix(const string& str, const string& suffix) {
  return StrSuffix(str.data(), str.size(), suffix.data(), str.size());
}
bool StrEndsWith(const char* str, const char* suffix) {
  return StrSuffix(str, suffix);
}
bool StrEndsWith(const string& str, const string& suffix) {
  return StrEndsWith(str.c_str(), suffix.c_str());
}

const char* StrFrontTrim(const char* str) {
  while ( isspace(*str) )
    ++str;
  return str;
}

string StrFrontTrim(const string& str) {
  size_t i = 0;
  while ( i < str.size() && isspace(str[i]) ) {
    ++i;
  }
  return str.substr(i);
}

string StrTrim(const string& str) {
  int i = 0;
  int j = str.size() - 1;
  while ( i <= j && isspace(str[i]) ) {
    ++i;
  }
  while ( j >= i && isspace(str[j]) ) {
    --j;
  }
  if ( j < i )  {
    return string("");
  }
  return str.substr(i, j - i + 1);
}

string StrTrimChars(const string& str, const char* chars_to_trim) {
  int i = 0;
  int j = str.size() - 1;
  while ( i <= j && strchr(chars_to_trim, str[i]) != NULL ) {
    ++i;
  }
  while ( j >= i && strchr(chars_to_trim, str[j]) != NULL ) {
    --j;
  }
  if ( j < i )  {
    return string("");
  }
  return str.substr(i, j - i + 1);
}

string StrTrimCompress(const string& str) {
  string s;
  for ( size_t i = 0; i < str.size(); i++ ) {
    if ( !isspace(str[i]) )
      s.append(1, str[i]);
  }
  return s;
}

void StrTrimCRLFInPlace(string& s) {
  int e = 0;
  while ( s.size() > e && (s[s.size()-1-e] == 0x0a ||
                           s[s.size()-1-e] == 0x0d) ) {
    e++;
  }
  s.erase(s.size()-e, e);
}
string StrTrimCRLF(const string& str) {
  string s = str;
  StrTrimCRLFInPlace(s);
  return s;
}

void ShiftLeftBuffer(void* buf,
                     size_t buf_size,
                     size_t shift_size,
                     int fill_value) {
  if ( shift_size == 0 ) return;
  CHECK_GE(buf_size, shift_size);
  uint8* const data = reinterpret_cast<uint8*>(buf);
  const size_t cb = buf_size - shift_size;
  memmove(data, data + shift_size, cb);
  memset(data + cb, fill_value, shift_size);
}

string NormalizeUrlPath(const string& path) {
  if ( path == "" ) {
    return "/";
  }
  string ret(strutil::NormalizePath(path, '/'));
  int i = 0;
  while ( i < ret.size() && ret[i] == '/' ) {
    ++i;
  }
  if ( i >= ret.size() ) {
    return "/";
  }
  if ( i == 0 ) {
    return ret;
  }
  return ret.substr(i - 1);
}

string NormalizePath(const string& path, char sep) {
  string s(path);
  const char sep_str[] = { sep, '\0' };
  // Normalize the slashes and add leading slash if necessary
  for ( size_t i = 0; i < s.size(); ++i ) {
    if ( s[i] == '\\' ) {
      s[i] = sep;
    }
  }
  bool slash_added = false;
  if ( s[0] != sep ) {
    s = sep_str + s;
    slash_added = true;
  }

  // Resolve occurrences of "///" in the normalized path
  const char triple_sep[] = { sep, sep, sep, '\0' };
  while ( true ) {
    const size_t index = s.find(triple_sep);
    if ( index == string::npos ) break;
    s = s.substr(0, index) + s.substr(index + 2);
  }
  // Resolve occurrences of "//" in the normalized path (but not beginning !)
  const char* double_sep = triple_sep + 1;
  while ( true ) {
    const size_t index = s.find(double_sep, 1);
    if ( index == string::npos ) break;
    s = s.substr(0, index) + s.substr(index + 1);
  }
  // Resolve occurrences of "/./" in the normalized path
  const char sep_dot_sep[] = { sep, '.', sep, '\0' };
  while ( true ) {
    const size_t index = s.find(sep_dot_sep);
    if ( index == string::npos ) break;
    s = s.substr(0, index) + s.substr(index + 2);
  }
  // Resolve occurrences of "/../" in the normalized path
  const char sep_dotdot_sep[] = { sep, '.', '.', sep, '\0' };
  while  ( true ) {
    const size_t index = s.find(sep_dotdot_sep);
    if ( index == string::npos ) break;
    if ( index == 0 )
      return slash_added ? "" : sep_str;
       // The only left path is the root.
    const size_t index2 = s.find_last_of(sep, index - 1);
    if ( index2 == string::npos )
      return slash_added ? "": sep_str;
    s = s.substr(0, index2) + s.substr(index + 3);
  }
  // Resolve ending "/.." and "/."
  {
    const char sep_dot[] = { sep, '.' , '\0' };
    const size_t index = s.rfind(sep_dot);
    if ( index != string::npos && index  == s.length() - 2 ) {
      s = s.substr(0, index);
    }
  }
  {
    const char sep_dotdot[] = { sep, '.', '.',  '\0' };
    size_t index = s.rfind(sep_dotdot);
    if ( index != string::npos && index == s.length() - 3 ) {
      if ( index == 0 )
        return slash_added ? "": sep_str;
      const size_t index2 = s.find_last_of(sep, index - 1);
      if ( index2 == string::npos )
        return slash_added ? "": sep_str;
      s = s.substr(0, index2);
    }
    if ( !slash_added && s.empty() ) s = sep_str;
  }
  if ( !slash_added || s.empty() ) return s;
  return s.substr(1);
}

string JoinStrings(const char* pieces[], size_t size, const char* glue) {
  string s;
  if ( size > 0 ) {
    s += pieces[0];
  }
  for ( size_t i = 1; i < size; i++ ) {
    s += glue;
    s += pieces[i];
  }
  return s;
}

string JoinStrings(const vector<string>& pieces, size_t start, size_t limit, const char* glue) {
  string s;
  if ( pieces.size() > start ) {
    s += pieces[start];
  }
  for ( size_t i = start + 1; i < limit && i < pieces.size(); ++i ) {
    s += glue;
    s += pieces[i];
  }
  return s;
}

string PrintableDataBuffer(const void* pbuffer, size_t size) {
  const uint8* buffer = reinterpret_cast<const uint8*>(pbuffer);
  string l1, l2;
  l1.reserve(size * 8 + (size / 16) * 10);
  l2.reserve(size * 8 + (size / 16) * 10);
  for ( size_t i = 0; i < size; i++ ) {
    if ( i % 16 == 0 ) {
      l1 += strutil::StringPrintf("\n%06d", static_cast<int32>(i));
      l2 += strutil::StringPrintf("\n%06d", static_cast<int32>(i));
    }
    l1 += strutil::StringPrintf("  0x%02x, ",
                                static_cast<int32>(buffer[i] & 0xff));
    if ( buffer[i] >= ' ' && buffer[i] <= '}' ) {
      l2 += strutil::StringPrintf("   '%c', ",
                                  static_cast<char>(buffer[i]));
    } else {
      l2 += strutil::StringPrintf(" '\\x%02x',",
                                  static_cast<int32>(buffer[i] & 0xff));
    }
  }
  const string str_size = StringOf(size);
  return "#" + str_size + " bytes HEXA: \n" + l1 + "\n" +
         "#" + str_size + " bytes CHAR: \n" + l2 + "\n";
}
string PrintableDataBufferHexa(const void* pbuffer, size_t size) {
  const uint8* buffer = reinterpret_cast<const uint8*>(pbuffer);
  string l1;
  l1.reserve(size * 8 + (size / 16) * 10);
  for ( size_t i = 0; i < size; i++ ) {
    if ( i % 16 == 0 ) {
      l1 += strutil::StringPrintf("\n%06d", static_cast<int32>(i));
    }
    l1 += strutil::StringPrintf("  0x%02x, ",
                                static_cast<int32>(buffer[i] & 0xff));
  }
  const string str_size = StringOf(size);
  return "#" + str_size + " bytes HEXA: \n" + l1 + "\n";
}
string PrintableDataBufferInline(const void* vbuffer, size_t size) {
  const unsigned char* buffer = static_cast<const unsigned char*>(vbuffer);
  ostringstream oss;
  oss << size << " bytes {";
  for ( int32 i = 0; i < size; i++ ) {
    if ( i != 0 ) {
      oss << " ";
    }
    char text[8] = {0};
    ::snprintf(text, sizeof(text)-1, "%02x", buffer[i]);
    oss << text;
  }
  oss << "}";
  return oss.str();
}

void SplitString(const string& s,
                 const string& separator,
                 vector<string>* output) {
  if ( separator.length() == 0 ) {
    // split all characters
    for ( string::const_iterator it = s.begin(); it != s.end(); ++it ) {
        output->push_back(string(1, *it));
    }
    return;
  }

  size_t pos = 0;
  size_t last_pos = string::npos;
  while ( true ) {
    last_pos = s.find(separator, pos);
    output->push_back(s.substr(pos, last_pos - pos));
    if ( last_pos == string::npos ) {
      return;
    }
    pos = last_pos + separator.size();
  }
}

void SplitPairs(const string& s,
                const string& elements_separator,  // sep1
                const string& pair_separator,      // sep2
                vector< pair<string, string> >* output) {
  vector<string> tmp;
  SplitString(s, elements_separator, &tmp);
  for ( size_t i = 0; i < tmp.size(); ++i ) {
    output->push_back(SplitFirst(tmp[i].c_str(), pair_separator));
  }
}

void SplitPairs(const string& s,
                const string& elements_separator,  // sep1
                const string& pair_separator,      // sep2
                map<string, string>* output) {
  vector<string> tmp;
  SplitString(s, elements_separator, &tmp);
  for ( size_t i = 0; i < tmp.size(); ++i ) {
    output->insert(SplitFirst(tmp[i].c_str(), pair_separator));
  }
}

bool SplitBracketedString(const char* s,
                          const char separator,
                          const char open_bracket,
                          const char close_bracket,
                          vector<string>* output) {
  const char* b = s;
  const char* p = s;
  int in_paranthesis = 0;
  while ( *p ) {
    if ( *p == open_bracket ) {
      ++in_paranthesis;
    } else if ( *p == close_bracket ) {
      --in_paranthesis;
    } else if ( *p == separator && in_paranthesis == 0 ) {
      output->push_back(string(b, p - b));
      b = p + 1;
    }
    ++p;
    if ( in_paranthesis < 0 )
      return false;
  }
  if ( in_paranthesis == 0 && p > b && *b ) {
    output->push_back(string(b, p - b));
  }
  return in_paranthesis == 0;
}
string RemoveOutermostBrackets(const string& s,
                               const char open_bracket,
                               const char close_bracket) {
  if (s.length() > 2) {
    if (s[0] == open_bracket && s[s.length()-1] == close_bracket) {
      return s.substr(1, s.length()-2);
    }
  }
  return s;
}

namespace {
inline void set_char_bit(uint32* charbits, char c) {
  const uint8 id = static_cast<uint8>(c);
  const int pos = (id >> 5);
  charbits[pos] = (charbits[pos]) | (1 << (id & 0x1f));
}
inline bool is_set_bit(uint32* charbits, char c) {
  const uint8 id = static_cast<uint8>(c);
  const int pos = (id >> 5);
  return ((charbits[pos] & (1 << (id & 0x1f)))) != 0;
}
inline uint8 hexval(char c) {
  if ( c >= '0' && c <= '9' ) return static_cast<uint8>(c - '0');
  if ( c >= 'a' && c <= 'f' ) return static_cast<uint8>(10 + c - 'a');
  if ( c >= 'A' && c <= 'F' ) return static_cast<uint8>(10 + c - 'A');
  return 0;
}
}

void SplitStringOnAny(const char* text, int size,
                      const char* separators,
                      vector<string>* output,
                      bool skip_empty) {
  uint32 charbits[256/32] = { 0, 0, 0, 0,
                              0, 0, 0, 0 };
  const char* p = separators;
  while ( *p ) {
    set_char_bit(charbits, *p++);
  }
  const char* t = text;
  size_t i = 0;
  size_t next_word_start = 0;
  while ( i < size ) {
    if ( is_set_bit(charbits, *t) ) {
      if (i > next_word_start || !skip_empty) {
        output->push_back(string(text + next_word_start,
                                 i - next_word_start));
      }
      next_word_start = i + 1;
    }
    ++i;
    ++t;
  }
  if (next_word_start < size) {
      output->push_back(string(text + next_word_start,
                               size - next_word_start));
  }
}


string StrNEscape(const char* text, size_t size, char escape,
                  const char* chars_to_escape) {
  uint32 escapes[256/32] = { 0, 0, 0, 0,
                             0, 0, 0, 0 };
  const char* p = chars_to_escape;
  while ( *p ) {
    set_char_bit(escapes, *p++);
  }
  set_char_bit(escapes, escape);
  const char* t = text;
  int i = 0;
  string ret;
  while ( i++ < size ) {
    if ( is_set_bit(escapes, *t) || *t  < 32 || *t > 126 ) {
      ret.append(strutil::StringPrintf("%c%02x",
                                       escape,
                                       static_cast<unsigned int>(
                                           static_cast<uint8>(*t) & 0xff)));
    } else {
      ret.push_back(*t);
    }
    ++t;
  }
  return ret;
}

const string& BoolToString(bool value) {
  static const string str_true = "true";
  static const string str_false = "false";
  return value ? str_true : str_false;
}

string GetNextInLexicographicOrder(const string& prefix) {
  if ( prefix.empty() ) {
    return "";
  }
  if ( prefix[prefix.size() - 1] == '\xff' ) {
    return GetNextInLexicographicOrder(prefix.substr(0, prefix.size() - 1));
  }

  string upper_bound(prefix);
  upper_bound[upper_bound.size() - 1] = prefix[prefix.size() - 1] + 1;
  return upper_bound;
}

string StrEscape(const char* text, char escape,
                 const char* chars_to_escape) {
  return StrNEscape(text, strlen(text), escape, chars_to_escape);
}

string StrEscape(const string& text,
                 char escape,
                 const char* chars_to_escape) {
  return StrNEscape(text.data(), text.size(),escape, chars_to_escape);
}

string StrUnescape(const char* text, char escape) {
  const char* p = text;
  size_t esc_count = 0;

  scoped_array<char> dest(new char[strlen(text)+1]);
  char* d = dest.get();
  while ( *p ) {
    if ( *p == escape ) {
      esc_count = 1;
    } else if ( esc_count ) {
      if ( esc_count == 2 ) {
        *d++ = static_cast<char>(hexval(*p) + 16 * hexval(*(p-1)));
        esc_count = 0;
      } else {
        ++esc_count;
      }
    } else {
      *d++ = *p;
    }
    ++p;
  }
  return string(dest.get(), d - dest.get());
}

string StrUnescape(const string& text, char escape) {
  return StrUnescape(text.c_str(), escape);
}
}

namespace {
static char kHexaDigits[] = "0123456789abcdef";
}

namespace strutil {
// Old implementation : user    0m6.368s
// New implementation : user    0m0.568s

string JsonStrEscape(const char* text, size_t size) {
  const char* p = text;
  scoped_array<char> dest(new char[size * 3 + 4]);
  uint8* d = reinterpret_cast<uint8*>(dest.get());
  string s;
  while ( size-- ) {
    const uint8 c = *p++;
    if ( c >= ' ' && c <= '~' ) {
      // ASCII printable
      if ( c == '\\' || c == '\"' || c == '/' ) {
        *d++ = '\\';
      }
      *d++ = c;
    } else {
      *d++ = '\\';
      switch ( c ) {
        case '\b': *d++ = 'b'; break;
        case '\f': *d++ = 'f'; break;
        case '\n': *d++ = 'n'; break;
        case '\r': *d++ = 'r'; break;
        case '\t': *d++ = 't'; break;
        default:
          *d++ = 'u';
          if ( size > 0 ) {
            --size;
            const uint8 c2 = *p++;
            *d++ = kHexaDigits[c >> 4];
            *d++ = kHexaDigits[c & 0xf];
            *d++ = kHexaDigits[c2 >> 4];
            *d++ = kHexaDigits[c2 & 0xf];
          } else {
            *d++ = '0';
            *d++ = '0';
            *d++ = kHexaDigits[c >> 4];
            *d++ = kHexaDigits[c & 0xf];
          }
      }
    }
  }
  return string(dest.get(), d - reinterpret_cast<uint8*>(dest.get()));
}

#ifdef HAVE_UNICODE_UTF_8
#include <unicode/utf8.h>      // ICU/source/common/unicode/utf8.h
#else
#define U8_IS_SINGLE(c) (((c)&0x80)==0)
#endif

string JsonStrUnescape(const char* text, size_t size) {
  const char* p = text;
  scoped_array<char> dest(new char[size * 3 + 4]);
  uint8* d = reinterpret_cast<uint8*>(dest.get());
  bool in_escape = false;
  while ( size ) {
    --size;
    if ( in_escape ) {
      switch ( *p ) {
        case '\\': *d++ = '\\'; break;
        case '\"': *d++ = '\"'; break;
        case '/':  *d++ = '/' ; break;
        case 'b':  *d++ = '\b'; break;
        case 'f':  *d++ = '\f'; break;
        case 'n':  *d++ = '\n'; break;
        case 'r':  *d++ = '\r'; break;
        case 't':  *d++ = '\t'; break;
        case 'u':  {
          if ( size < 4 ) {
            break;
          }
          ++p;
          *d++ = static_cast<uint8>(hexval(*(p+1)) +
                                   16 * hexval(*p-1));
          ++p; ++p;
          *d++ = static_cast<uint8>(hexval(*(p+1)) +
                                    16 * hexval(*p-1));
          ++p;  // leave room for the next ++
          size -= 4;
        }
          break;
        default:  *d++ = *p; break;
      }
      in_escape = false;
    } else if ( *p == '\\' ) {
      in_escape = true;
    } else {
      if ( !U8_IS_SINGLE(*p) ) {
        // this is not a US-ASCII 0..0x7f but a UTF-8 lead or trail byte
      } else {
        *d++ = *p;
      }
    }
    ++p;
  }
  return string(dest.get(), d - reinterpret_cast<uint8*>(dest.get()));
}

// TODO(cosmin): clear this up!
//               Can it start with a digit?
bool IsValidIdentifier(const char* s) {
  if ( *s < '0' || (*s > '9' && *s < 'A') ||
            (*s > 'Z' && *s < 'a') || *s > 'z' ) {
    return false;
  }
  ++s;
  while ( *s ) {
    if ( *s < '0' ||
         (*s > '9' && *s < 'A') ||
         (*s > 'Z' && *s < '_') ||
         (*s > '_' && *s < 'a') ||
         *s > 'z' ) {
      return false;
    }
    ++s;
  }
  return true;
}

const char* const kBinDigits[16] = {
  "0000",
  "0001",
  "0010",
  "0011",
  "0100",
  "0101",
  "0110",
  "0111",
  "1000",
  "1001",
  "1010",
  "1011",
  "1100",
  "1101",
  "1110",
  "1111"
};

}

namespace {
enum StrFormatState {
  IN_TEXT,
  IN_SYMBOL,
  IN_ESCAPE_TEXT,
  IN_ESCAPE_SYMBOL,
};
}

namespace strutil {
string StrMapFormat(const char* s,
                    const map<string, string>& m,
                    const char* arg_begin,
                    const char* arg_end,
                    char escape_char) {
  CHECK(*arg_begin != '\0');
  CHECK(*arg_end != '\0');
  const int size_begin = strlen(arg_begin);
  const int size_end = strlen(arg_end);
  StrFormatState state = IN_TEXT;

  string out;
  string symbol;
  const char* begin = s;
  const char* p = s;
  while ( *p ) {
    switch ( state ) {
      case IN_TEXT:
        if ( *p == escape_char ) {
          state = IN_ESCAPE_TEXT;
          out.append(begin, p - begin);
          ++p;
          begin = p;
        } else if ( StrPrefix(p, arg_begin) ) {
          out.append(begin, p - begin);
          state = IN_SYMBOL;
          symbol.clear();
          p += size_begin;
          begin = p;
        } else {
          ++p;
        }
        break;
      case IN_SYMBOL:
        if ( *p == escape_char ) {
          state = IN_ESCAPE_SYMBOL;
          symbol.append(begin, p - begin);
          ++p;
          begin = p;
        } else if ( StrPrefix(p, arg_end) ) {
          symbol.append(begin, p - begin);
          state = IN_TEXT;
          p += size_end;
          begin = p;
          map<string, string>::const_iterator it = m.find(symbol);
          if ( it != m.end() ) {
            out.append(it->second);
          }
        } else {
          ++p;
        }
        break;
      case IN_ESCAPE_TEXT:
        if ( *p != *arg_begin ) {
          out.append(1, escape_char);
        }
        out.append(1, *p);
        ++p;
        begin = p;
        state = IN_TEXT;
        break;
      case IN_ESCAPE_SYMBOL:
        if ( *p != *arg_end ) {
          symbol.append(1, escape_char);
        }
        symbol.append(1, *p);
        ++p;
        begin = p;
        state = IN_SYMBOL;
        break;
    }
  }
  switch ( state ) {
    case IN_TEXT:
      out.append(begin, p - begin);
      break;
    case IN_SYMBOL:
    case IN_ESCAPE_SYMBOL:
      // LOG_WARNING << " Invalid escape string received for formatting: ["
      //  << JsonStrEscape(s) << "]";
      break;
    case IN_ESCAPE_TEXT:
      out.append(1, escape_char);
      break;
  }
  return out;
}

int Utf8Strlen(const char *s) {
    const uint8* p = (const uint8*) s;
    int len = 0;
    int in_seq = 0;

    while (*p) {
        if (in_seq) {
            --in_seq;
            ++p;
            continue;
        }
        ++len;
        if (*p <= 0x7F)  {
            /* 0XXX XXXX one byte */
            in_seq = 0;
        } else if (((*p) & 0xE0) == 0xC0) {
            /* 110X XXXX  two bytes */
            in_seq = 1;
        } else if (((*p) & 0xF0) == 0xE0) {
            /* 1110 XXXX  three bytes */
            in_seq = 2;
        } else if (((*p) & 0xF8) == 0xF0) {
            /* 1111 0XXX  four bytes */
            in_seq = 3;
        } else if (((*p) & 0xFC) == 0xF8) {
            /* 1111 10XX  five bytes */
            in_seq = 4;
        } else if (((*p) & 0xFE) == 0xFC) {
            /* 1111 110X  six bytes */
            in_seq = 5;
        } else {
            return 0;
        }
        ++p;
    }
    if (in_seq) {
        return 0;
    }
    return len;
}
}
