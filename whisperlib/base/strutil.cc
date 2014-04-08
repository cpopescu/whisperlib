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
#include <iomanip>      // std::setprecision

#include "whisperlib/base/strutil.h"
#include "whisperlib/base/scoped_ptr.h"
#include "whisperlib/base/unicode_table.h"

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
// Comparison between char* and string
bool StrEql(const char* p, const string& s) {
    return p ? !strcmp(p, s.c_str()) : s.empty();
}
bool StrEql(const string& s, const char* p) {
    return p ? !strcmp(p, s.c_str()) : s.empty();
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

string PrintableEscapedDataBuffer(const void* buffer, size_t size) {
  const char* p = (const char*)buffer;
  const char* end = p + size;
  string s;
  while ( p < end ) {
    wint_t codepoint;
    const char* next = strutil::i18n::Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint == WEOF || (codepoint < 32 || codepoint == 127)) {
      s += "\\x" + ToHex((const unsigned char*)(p), next - p);
    } else {
      strutil::i18n::CodePointToUtf8(codepoint, &s);
    }
    p = next;
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
  if ( s.empty() ) {
    return;
  }
  if ( separator.empty() ) {
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
    if ( last_pos == string::npos ) {
         output->push_back(s.substr(pos, string::npos));
         return;
    }
    output->push_back(s.substr(pos, last_pos - pos));
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
    output->push_back(SplitFirst(tmp[i], pair_separator));
  }
}

void SplitPairs(const string& s,
                const string& elements_separator,  // sep1
                const string& pair_separator,      // sep2
                map<string, string>* output) {
  vector<string> tmp;
  SplitString(s, elements_separator, &tmp);
  for ( size_t i = 0; i < tmp.size(); ++i ) {
    output->insert(SplitFirst(tmp[i], pair_separator));
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

namespace strutil {
// Old implementation : user    0m6.368s
// New implementation : user    0m0.568s

#ifdef HAVE_UNICODE_UTF_8
#include <unicode/utf8.h>      // ICU/source/common/unicode/utf8.h
#else
#define U8_IS_SINGLE(c) (((c)&0x80)==0)
#endif

string JsonStrEscape(const char* text, size_t size) {
  const char* p = text;
  scoped_array<char> dest(new char[size * 3 + 4]);
  uint8* d = reinterpret_cast<uint8*>(dest.get());
  while (size > 0) {
    const uint8 c = *p++;
    size--;

    if ( c >= ' ' && c <= '~' ) {
      // ASCII printable
      if ( c == '\\' || c == '\"' || c == '/' ) {
        *d++ = '\\';
      }
      *d++ = c;
      continue;
    }
    switch ( c ) {
      case '\b': *d++ = '\\'; *d++ = 'b'; break;
      case '\f': *d++ = '\\'; *d++ = 'f'; break;
      case '\n': *d++ = '\\'; *d++ = 'n'; break;
      case '\r': *d++ = '\\'; *d++ = 'r'; break;
      case '\t': *d++ = '\\'; *d++ = 't'; break;
      default: {
        // NOTE(cosmin): leave it UTF8. JSON is perfectly capable of carrying it.
        //               Converting to Unicode is not such a simple task as writing a hex code.
        *d++ = c;
        while (size > 0 && (((*p) & 0xc0) == 0x80)) {
          *d++ = *p++;
          size--;
        }
      }
    }
  }
  return string(dest.get(), d - reinterpret_cast<uint8*>(dest.get()));
}

string JsonStrUnescape(const char* text, size_t size) {
  const char* p = text;
  scoped_array<char> dest(new char[size * 3 + 4]);
  uint8* d = reinterpret_cast<uint8*>(dest.get());
  bool in_escape = false;
  while ( size > 0 ) {
    const uint8 c = *p++;
    size--;

    if ( in_escape ) {
      switch ( c ) {
        case '\\': *d++ = '\\'; break;
        case '\"': *d++ = '\"'; break;
        case '/':  *d++ = '/' ; break;
        case 'b':  *d++ = '\b'; break;
        case 'f':  *d++ = '\f'; break;
        case 'n':  *d++ = '\n'; break;
        case 'r':  *d++ = '\r'; break;
        case 't':  *d++ = '\t'; break;
        case 'u': {
          // assuming the unicode value has only 4 hex digits (Basic Multilingual Plane)
          if ( size < 4 ) {
            break;
          }
          const uint16 v = ((uint16)(hexval(*(p)) << 24) +
                            ((uint16)(hexval(*(p + 1))) << 16) +
                            ((uint16)(hexval(*(p + 2))) <<  8) +
                            ((uint16)(hexval(*(p + 3))) <<  0));
          p += 4;
          size -= 4;

          if (v < 0x007F) {
            *d++ = v; // single byte
          } else if (v < 0x07FF) {    // v has 11 bits
            *d++ = 0xC0 + (v >> 6);  // copy first 5 bits
            *d++ = v & 0x3F;         // copy last 6 bits
          } else if (v < 0xFFFF) {             // v has 16 bits
              *d++ = 0xE0 + (v >> 12);         // copy first 4 bits
              *d++ = 0x80 + ((v >> 6) & 0x3F); // copy next 6 bits
              *d++ = 0x80 + (v & 0x3F);        // copy last 6 bits
          } // the rest of Unicode cases until: 0x7FFFFFFF not handled
        }
        break;
        default:  *d++ = c; break;
      }
      in_escape = false;
      continue;
    }
    if ( c == '\\' ) {
      in_escape = true;
      continue;
    }

    // default copying behaviour
    *d++ = c;
  }
  return string(dest.get(), d - reinterpret_cast<uint8*>(dest.get()));
}

string XmlStrEscape(const string& original) {
  string result;
  result.reserve(original.size());
  for (int i = 0; i != original.size(); ++i) {
    switch (original[i]) {
      case '&':
        result.append("&amp;");
        break;
      case '\"':
        result.append("&quot;");
        break;
      case '\'':
        result.append("&apos;");
        break;
      case '<':
        result.append("&lt;");
        break;
      case '>':
        result.append("&gt;");
        break;
      default:
        result.append(1, original[i]);
    }
  }
  return result;
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

string StrHumanDuration(int64 duration_ms) {
    ostringstream oss;
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
string StrHumanBytes(uint64 bytes) {
    if (bytes > 1024 * 1024 * 1024) {
        return StringPrintf("%.2fGB", bytes / (1024.0f * 1024 * 1024));
    }
    if (bytes > 1024 * 1024) {
        return StringPrintf("%.2fMB", bytes / (1024.0f * 1024));
    }
    if (bytes > 1024) {
        return StringPrintf("%.2fKB", bytes / (1024.0f));
    }
    return StringPrintf("%" PRIu64 " bytes", bytes);
}
}

////////////////////////////////////////////////////////////////////////////////

namespace {
map<wchar_t, const wchar_t*> glb_umlaut_map;
map<wint_t, const char*> glb_utf8_umlaut_map;
map<wchar_t, wchar_t> glb_upper_map;
map<wchar_t, wchar_t> glb_lower_map;
hash_set<wchar_t> glb_spaces;
}

namespace strutil {
namespace i18n {

inline pair<size_t, uint8> Utf8CharLen(uint8 p) {
  if (p <= 0x7F)  {
    /* 0XXX XXXX one byte */
    return make_pair(1, p & 0x7f);
  }
  if ((p & 0xE0) == 0xC0) {
    /* 110X XXXX  two bytes */
    return make_pair(2, p & 0x1f);
  }
  if ((p & 0xF0) == 0xE0) {
    /* 1110 XXXX  three bytes */
    return make_pair(3, p & 0x0f);
  }
  if ((p & 0xF8) == 0xF0) {
    /* 1111 0XXX  four bytes */
    return make_pair(4, p & 0x07);
  }
  if ((p & 0xFC) == 0xF8) {
    /* 1111 10XX  five bytes */
    return make_pair(5, p & 0x03);
  }
  if ((p & 0xFE) == 0xFC) {
    /* 1111 110X  six bytes */
    return make_pair(6, p & 0x01);
  }
  return make_pair(0, 0);
}

size_t Utf8Strlen(const char *s) {
    const uint8* p = (const uint8*) s;
    size_t len = 0;

    while (*p) {
      size_t crt_len = Utf8CharLen(*p).first;
      if (!crt_len) {
        return 0;   // invalid utf8
      }
      crt_len--; ++p;
      while (crt_len && *p) {
        if ((*p & 0xc0 ) != 0x80) {
          return 0;
        }
        ++p;
        --crt_len;
      }
      if (crt_len) {
        return 0;   // invalid utf8
      }
      ++len;
    }
    return len;
}

const char* Utf8ToCodePoint(const char* start, const char* end, wint_t* codepoint) {
  if (end == NULL) {
    end = start + strlen(start);
  }

  pair<size_t, uint8> crt = Utf8CharLen(uint8(*start));
  if (!crt.first) {
    *codepoint = WEOF;
    return start + 1;
  }
  const char* p = start + 1;
  *codepoint = wint_t(crt.second);
  while (--crt.first) {
    const uint8 val = uint8(*p);
    if ((val & 0xc0 ) != 0x80) {
      *codepoint = WEOF;
      return p;
    }
    *codepoint <<= 6;
    *codepoint |= (wint_t(val & 0x3f));
    ++p;
  }
  return p;
}

wint_t ToUpper(wint_t c) {
  // NOTE: towupper does not handle correctly accented chars so we need
  //  our own map
  map<wchar_t, wchar_t>::const_iterator it = glb_upper_map.find(c);
  if (it != glb_upper_map.end()) {
    return it->second;
  }
  return towupper(c);
}

wint_t ToLower(wint_t c) {
  // NOTE: towlower does not handle correctly accented chars so we need
  //  our own map
  map<wchar_t, wchar_t>::const_iterator it = glb_lower_map.find(c);
  if (it != glb_lower_map.end()) {
    return it->second;
  }
  return towlower(c);
}


static const size_t kMaxUtf8Size = 6;

size_t CodePointToUtf8(wint_t codepoint, string* s) {
    if (codepoint <= 0x007F) {
      // UTF-8, 1 bytes: 0vvvvvvv 10vvvvvv 10vvvvvv
      s->append(1, char(codepoint));
      return 1;
    }
    if (codepoint <= 0x07FF) {
      // UTF-8, 2 bytes: 110vvvvv 10vvvvvv 10vvvvvv
      s->append(1, char(0xC0 | ((codepoint >> 6) & 0x1F)));
      s->append(1, char(0x80 | (codepoint & 0x3F)));
      return 2;
    }
    if (codepoint <= 0xFFFF) {
      // UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv
      s->append(1, char(0xE0 | ((codepoint >> 12) & 0x0F)));
      s->append(1, char(0x80 | ((codepoint >> 6) & 0x3F)));
      s->append(1, char(0x80 | (codepoint & 0x3F)));
      return 3;
    }
    if (codepoint <= 0x1FFFFF) {
      // UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
      s->append(1, char(0xF0 | ((codepoint >> 18) & 0x07)));
      s->append(1, char(0x80 | ((codepoint >> 12) & 0x3F)));
      s->append(1, char(0x80 | ((codepoint >> 6) & 0x3F)));
      s->append(1, char(0x80 | (codepoint & 0x3F)));
      return 4;
    }
    if (codepoint <= 0x3FFFFFF) {
      // UTF-8, 5 bytes: 111110vv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
      s->append(1, char(0xF8 | ((codepoint >> 24) & 0x03)));
      s->append(1, char(0x80 | ((codepoint >> 18) & 0x3F)));
      s->append(1, char(0x80 | ((codepoint >> 12) & 0x3F)));
      s->append(1, char(0x80 | ((codepoint >> 6) & 0x3F)));
      s->append(1, char(0x80 | (codepoint & 0x3F)));
      return 5;
    }
    if (codepoint <= 0x7FFFFFFF) {
      // UTF-8, 6 bytes: 1111110v 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
      s->append(1, char(0xFC | ((codepoint >> 30) & 0x01)));
      s->append(1, char(0x80 | ((codepoint >> 24) & 0x3F)));
      s->append(1, char(0x80 | ((codepoint >> 18) & 0x3F)));
      s->append(1, char(0x80 | ((codepoint >> 12) & 0x3F)));
      s->append(1, char(0x80 | ((codepoint >> 6) & 0x3F)));
      s->append(1, char(0x80 | (codepoint & 0x3F)));
      return 6;
    }
    return 0;
}

wchar_t* Utf8ToWchar(const char* s, size_t size_bytes) {
  wchar_t* ws = new wchar_t[size_bytes + 1];
  wchar_t* wp = ws;
  if (size_bytes) {
    const char* end = s + size_bytes;
    const char* p = s;
    wint_t codepoint;
    while (p < end) {
      p = Utf8ToCodePoint(p, end, &codepoint);
      if (codepoint != WEOF) {
        *wp++ = codepoint;
      }
    }
  }
  *wp = L'\0';
  return ws;
}

size_t WcharToUtf8Size(const wchar_t* s) {
    return kMaxUtf8Size * wcslen(s) + 1;
}

string ToUpperStr(const string& s) {
  const char* end = s.c_str() + s.size();
  const char* p = s.c_str();
  string result;
  wint_t codepoint;
  while (p < end) {
    p = Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint != WEOF) {
      CodePointToUtf8(ToUpper(codepoint), &result);
    }
  }
  return result;
}

string ToLowerStr(const string& s) {
  const char* end = s.c_str() + s.size();
  const char* p = s.c_str();
  string result;
  wint_t codepoint;
  while (p < end) {
    p = Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint != WEOF) {
      CodePointToUtf8(ToLower(codepoint), &result);
    }
  }
  return result;
}

bool IsUtf8Valid(const string& s) {
  const char* end = s.c_str() + s.size();
  const char* p = s.c_str();
  wint_t codepoint;
  while (p < end) {
    p = Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint == WEOF) {
      return false;
    }
  }
  return true;
}

string Utf8StrPrefix(const string& s, size_t prefix_size) {
  const char* end = s.c_str() + s.size();
  const char* p = s.c_str();
  size_t num = 0;

  while (p < end && num < prefix_size) {
    size_t crt_len = Utf8CharLen(uint8(*p)).first;
    if (!crt_len) return string();
    p += crt_len;
    ++num;
  }
  if (p > end) {
    return string();
  }
  return string (s.c_str(), p - s.c_str());
}

string Utf8StrTrim(const string& s) {
  const char* p = s.c_str();
  const char* end = p + s.size();
  const char* begin_non_space = NULL;
  const char* last_space_start = NULL;

  wint_t codepoint;
  while (p < end) {
    const char* next = Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint != WEOF) {
      if (!IsSpace(codepoint)) {
        if (!begin_non_space) {
          begin_non_space = p;
        }
        last_space_start = NULL;
      } else if (!last_space_start) {
        last_space_start = p;
      }
    }
    p = next;
  }
  if (!begin_non_space) {
    return s;
  }
  if (!last_space_start) {
    return s.substr(begin_non_space - s.c_str());
  }
  return s.substr(begin_non_space - s.c_str(), last_space_start - begin_non_space);
}

pair<string, string> Utf8SplitPairs(const string& s, wchar_t split) {
  const char* end = s.c_str() + s.size();
  const char* p = s.c_str();

  wint_t codepoint;
  while (p < end) {
    const char* next = Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint == split) {
      return make_pair(s.substr(0, p - s.c_str()), s.substr(next - s.c_str()));
    }
    p = next;
  }
  return make_pair(s, "");
}

void Utf8SplitOnWChars(const string& s, const wchar_t* splits, vector<string>* terms) {
  const char* end = s.c_str() + s.size();
  const char* p = s.c_str();
  const char* starter = NULL;

  wint_t codepoint;

  while (p < end) {
    const char* e = Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint != WEOF) {
      bool is_split = false;
      const wchar_t* psplits = splits;
      while (*psplits && !is_split) {
        is_split = *psplits == codepoint;
        ++psplits;
      }
      if (is_split) {
        if (starter != NULL && p > starter) {
          terms->push_back(string(starter, p - starter));
          starter = NULL;
        }
      } else if (starter == NULL) {
        starter = p;
      }
    } else {
      starter = NULL;
    }
    p = e;
  }
  if (p <= end && starter != NULL && p > starter) {
    terms->push_back(string(starter, p - starter));
  }
}

void Utf8SplitOnWhitespace(const string& s, vector<string>* terms) {
  const char* end = s.c_str() + s.size();
  const char* p = s.c_str();
  const char* starter = NULL;

  wint_t codepoint;

  while (p < end) {
    const char* e = Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint != WEOF) {
      if (IsSpace(codepoint)) {
        if (starter != NULL && p > starter) {
          terms->push_back(string(starter, p - starter));
          starter = NULL;
        }
      } else if (starter == NULL) {
        starter = p;
      }
    } else {
      starter = NULL;
    }
    p = e;
  }
  if (p <= end && starter != NULL && p > starter) {
    terms->push_back(string(starter, p - starter));
  }
}

wchar_t* UmlautTransform(const wchar_t* s) {
    // One pass for size
    const wchar_t* p = s;
    int size = 0;
    bool need_conversion = false;
    while (*p) {
        map<wchar_t, const wchar_t*>::const_iterator it = glb_umlaut_map.find(*p);
        if (it == glb_umlaut_map.end()) {
            ++size;
        } else {
            need_conversion = true;
            size += wcslen(it->second);
        }
        ++p;
    }
    if (!need_conversion) {
        return NULL;
    }

    p = s;
    wchar_t* dest = new wchar_t[size + 1];
    wchar_t* d = dest;
    // Second pass to convert
    while (*p) {
        map<wchar_t, const wchar_t*>::const_iterator it = glb_umlaut_map.find(*p);
        if (it == glb_umlaut_map.end()) {
            *d++ = *p++;
        } else {
            ++p;
            wcscpy(d, it->second);
            d += wcslen(it->second);
        }
    }
    *d = L'\0';
    CHECK_EQ(wcslen(dest), size);

    return dest;
}

bool UmlautTransformUtf8(const string& s, string* result) {
  const char* end = s.c_str() + s.size();
  const char* p = s.c_str();
  wint_t codepoint;
  bool transformed = false;

  while (p < end) {
    const char* e = Utf8ToCodePoint(p, end, &codepoint);
    if (codepoint != WEOF) {
      map<wint_t, const char*>::const_iterator it = glb_utf8_umlaut_map.find(codepoint);
      if (it != glb_utf8_umlaut_map.end()) {
        transformed = true;
        result->append(it->second);
      } else {
        result->append(p, e - p);
      }
    } else {
      // This means invalid utf8 - skip / do something ?
      // LOG_ERROR << "Invalid UTF8 detected [" << s << "]";
    }
    p = e;
  }
  return transformed;
}

bool IsSpace(const wint_t c) {
  return glb_spaces.find(c) != glb_spaces.end();
}

bool IsAlnum(const wint_t c) {
  if ((c >> 3) > sizeof(glb_alnum_table))
    return false;   // unknown - out of range
  return (glb_alnum_table[c >> 3] & (1 << (c & 7))) != 0;
}
}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
struct __InitUmlaut {
#define _ADD(a, b, c)                                   \
  do {                                                  \
    glb_umlaut_map.insert(make_pair((a), (b)));         \
    glb_utf8_umlaut_map.insert(make_pair((a), (c)));    \
  } while(false)
#define _UPPER_ADD(u, l)                                \
  do {                                                  \
    glb_upper_map.insert(make_pair((l), (u)));          \
    glb_lower_map.insert(make_pair((u), (l)));   \
  } while(false)
#define _SPACE_ADD(c) \
  glb_spaces.insert(c)

  // NOTE/TODO(cp):  this is imperfect, need to take in considerations other
  //   factors. For example unicode character U+0110 should be translated
  //   to 'Dj' for serbo-croatian (e.g. is the first letter in Djokovic)
  //   but to a simple D for vietnamese.
  __InitUmlaut() {
      _ADD(L'\u00C0', L"A", "A");
      _ADD(L'\u00E0', L"a", "a");
      _ADD(L'\u00C1', L"A", "A");
      _ADD(L'\u00E1', L"a", "a");
      _ADD(L'\u00C2', L"A", "A");
      _ADD(L'\u00E2', L"a", "a");
      _ADD(L'\u00C3', L"A", "A");
      _ADD(L'\u00E3', L"a", "a");
      _ADD(L'\u00C4', L"A", "A");
      _ADD(L'\u00E4', L"a", "a");
      _ADD(L'\u00C5', L"A", "A");
      _ADD(L'\u00E5', L"a", "a");
      _ADD(L'\u0100', L"A", "A");
      _ADD(L'\u0101', L"a", "a");
      _ADD(L'\u0102', L"A", "A");
      _ADD(L'\u0103', L"a", "a");
      _ADD(L'\u0104', L"A", "A");
      _ADD(L'\u0105', L"a", "a");
      _ADD(L'\u01DE', L"A", "A");
      _ADD(L'\u01DF', L"a", "a");
      _ADD(L'\u01FA', L"A", "A");
      _ADD(L'\u01FB', L"a", "a");
      _ADD(L'\u00C6', L"Ae", "Ae");
      _ADD(L'\u00E6', L"ae", "ae");
      _ADD(L'\u01FC', L"Ae", "Ae");
      _ADD(L'\u01FD', L"ae", "ae");
      _ADD(L'\u1E02', L"B", "B");
      _ADD(L'\u1E03', L"b", "b");
      _ADD(L'\u0106', L"C", "C");
      _ADD(L'\u0107', L"c", "c");
      _ADD(L'\u00C7', L"C", "C");
      _ADD(L'\u00E7', L"c", "c");
      _ADD(L'\u010C', L"C", "C");
      _ADD(L'\u010D', L"c", "c");
      _ADD(L'\u0108', L"C", "C");
      _ADD(L'\u0109', L"c", "c");
      _ADD(L'\u010A', L"C", "C");
      _ADD(L'\u010B', L"c", "c");
      _ADD(L'\u1E10', L"D", "D");
      _ADD(L'\u1E11', L"d", "d");
      _ADD(L'\u010E', L"D", "D");
      _ADD(L'\u010F', L"d", "d");
      _ADD(L'\u1E0A', L"D", "D");
      _ADD(L'\u1E0B', L"d", "d");
      _ADD(L'\u0110', L"D", "D");
      _ADD(L'\u0111', L"d", "d");
      _ADD(L'\u00D0', L"D", "D");
      _ADD(L'\u00F0', L"d", "d");
      _ADD(L'\u01F1', L"Dz", "Dz");
      _ADD(L'\u01F3', L"dz", "dz");
      _ADD(L'\u01C4', L"Dz", "Dz");
      _ADD(L'\u01C6', L"dz", "dz");
      _ADD(L'\u00C8', L"E", "E");
      _ADD(L'\u00E8', L"e", "e");
      _ADD(L'\u00C9', L"E", "E");
      _ADD(L'\u00E9', L"e", "e");
      _ADD(L'\u011A', L"E", "E");
      _ADD(L'\u011B', L"e", "e");
      _ADD(L'\u00CA', L"E", "E");
      _ADD(L'\u00EA', L"e", "e");
      _ADD(L'\u00CB', L"E", "E");
      _ADD(L'\u00EB', L"e", "e");
      _ADD(L'\u0112', L"E", "E");
      _ADD(L'\u0113', L"e", "e");
      _ADD(L'\u0114', L"E", "E");
      _ADD(L'\u0115', L"e", "e");
      _ADD(L'\u0118', L"E", "E");
      _ADD(L'\u0119', L"e", "e");
      _ADD(L'\u0116', L"E", "E");
      _ADD(L'\u0117', L"e", "e");
      _ADD(L'\u01B7', L"E", "E");
      _ADD(L'\u0292', L"e", "e");
      _ADD(L'\u01EE', L"E", "E");
      _ADD(L'\u01EF', L"e", "e");
      _ADD(L'\u1E1E', L"F", "F");
      _ADD(L'\u1E1F', L"f", "f");
      _ADD(L'\u0192', L"f", "f");
      _ADD(L'\uFB00', L"ff", "ff");
      _ADD(L'\uFB01', L"ff", "ff");
      _ADD(L'\uFB02', L"ffi", "ffi");
      _ADD(L'\uFB03', L"ffl", "ffl");
      _ADD(L'\uFB04', L"ffl", "ffl");
      _ADD(L'\uFB05', L"ft", "ft");
      _ADD(L'\u01F4', L"G", "G");
      _ADD(L'\u01F5', L"g", "g");
      _ADD(L'\u0122', L"G", "G");
      _ADD(L'\u0123', L"g", "g");
      _ADD(L'\u01E6', L"G", "G");
      _ADD(L'\u01E7', L"g", "g");
      _ADD(L'\u011C', L"G", "G");
      _ADD(L'\u011D', L"g", "g");
      _ADD(L'\u011E', L"G", "G");
      _ADD(L'\u011F', L"g", "g");
      _ADD(L'\u0120', L"G", "G");
      _ADD(L'\u0121', L"g", "g");
      _ADD(L'\u01E4', L"G", "G");
      _ADD(L'\u01E5', L"g", "g");
      _ADD(L'\u0124', L"H", "H");
      _ADD(L'\u0125', L"h", "h");
      _ADD(L'\u0126', L"H", "H");
      _ADD(L'\u0127', L"h", "h");
      _ADD(L'\u00CC', L"I", "I");
      _ADD(L'\u00EC', L"i", "i");
      _ADD(L'\u00CD', L"I", "I");
      _ADD(L'\u00ED', L"i", "i");
      _ADD(L'\u00CE', L"I", "I");
      _ADD(L'\u00EE', L"i", "i");
      _ADD(L'\u0128', L"I", "I");
      _ADD(L'\u0129', L"i", "i");
      _ADD(L'\u00CF', L"I", "I");
      _ADD(L'\u00EF', L"i", "i");
      _ADD(L'\u012A', L"I", "I");
      _ADD(L'\u012B', L"i", "i");
      _ADD(L'\u012C', L"I", "I");
      _ADD(L'\u012D', L"i", "i");
      _ADD(L'\u012E', L"I", "I");
      _ADD(L'\u012F', L"i", "i");
      _ADD(L'\u0130', L"I", "I");
      _ADD(L'\u0131', L"i", "i");
      _ADD(L'\u0132', L"Ij", "Ij");
      _ADD(L'\u0133', L"ij", "ij");
      _ADD(L'\u0134', L"J", "J");
      _ADD(L'\u0135', L"j", "j");
      _ADD(L'\u1E30', L"K", "K");
      _ADD(L'\u1E31', L"k", "k");
      _ADD(L'\u0136', L"K", "K");
      _ADD(L'\u0137', L"k", "k");
      _ADD(L'\u01E8', L"K", "K");
      _ADD(L'\u01E9', L"k", "k");
      _ADD(L'\u0138', L"K", "K");
      _ADD(L'\u0139', L"L", "L");
      _ADD(L'\u013A', L"l", "l");
      _ADD(L'\u013B', L"L", "L");
      _ADD(L'\u013C', L"l", "l");
      _ADD(L'\u013D', L"L", "L");
      _ADD(L'\u013E', L"l", "l");
      _ADD(L'\u013F', L"L", "L");
      _ADD(L'\u0140', L"l", "l");
      _ADD(L'\u0141', L"L", "L");
      _ADD(L'\u0142', L"l", "l");
      _ADD(L'\u01C7', L"Lj", "Lj");
      _ADD(L'\u01C9', L"lj", "lj");
      _ADD(L'\u1E40', L"M", "M");
      _ADD(L'\u1E41', L"m", "m");
      _ADD(L'\u0143', L"N", "N");
      _ADD(L'\u0144', L"n", "n");
      _ADD(L'\u0145', L"N", "N");
      _ADD(L'\u0146', L"n", "n");
      _ADD(L'\u0147', L"N", "N");
      _ADD(L'\u0148', L"n", "n");
      _ADD(L'\u00D1', L"N", "N");
      _ADD(L'\u00F1', L"n", "n");
      _ADD(L'\u0149', L"n", "n");
      _ADD(L'\u014A', L"Eng", "Eng");
      _ADD(L'\u014B', L"eng", "eng");
      _ADD(L'\u01CA', L"Nj", "Nj");
      _ADD(L'\u01CC', L"nj", "nj");
      _ADD(L'\u00D2', L"O", "O");
      _ADD(L'\u00F2', L"o", "o");
      _ADD(L'\u00D3', L"O", "O");
      _ADD(L'\u00F3', L"o", "o");
      _ADD(L'\u00D4', L"O", "O");
      _ADD(L'\u00F4', L"o", "o");
      _ADD(L'\u00D5', L"O", "O");
      _ADD(L'\u00F5', L"o", "o");
      _ADD(L'\u00D6', L"O", "O");
      _ADD(L'\u00F6', L"o", "o");
      _ADD(L'\u014C', L"O", "O");
      _ADD(L'\u014D', L"o", "o");
      _ADD(L'\u014E', L"O", "O");
      _ADD(L'\u014F', L"o", "o");
      _ADD(L'\u00D8', L"O", "O");
      _ADD(L'\u00F8', L"o", "o");
      _ADD(L'\u0150', L"O", "O");
      _ADD(L'\u0151', L"o", "o");
      _ADD(L'\u01FE', L"O", "O");
      _ADD(L'\u01FF', L"o", "o");
      _ADD(L'\u0152', L"Oe", "Oe");
      _ADD(L'\u0153', L"oe", "oe");
      _ADD(L'\u1E56', L"P", "P");
      _ADD(L'\u1E57', L"p", "p");
      _ADD(L'\u0154', L"R", "R");
      _ADD(L'\u0155', L"r", "r");
      _ADD(L'\u0156', L"R", "R");
      _ADD(L'\u0157', L"r", "r");
      _ADD(L'\u0158', L"R", "R");
      _ADD(L'\u0159', L"r", "r");
      _ADD(L'\u027C', L"r", "r");
      _ADD(L'\u015A', L"S", "S");
      _ADD(L'\u015B', L"s", "s");
      _ADD(L'\u015E', L"S", "S");
      _ADD(L'\u015F', L"s", "s");
      _ADD(L'\u0160', L"S", "S");
      _ADD(L'\u0161', L"s", "s");
      _ADD(L'\u015C', L"S", "S");
      _ADD(L'\u015D', L"s", "s");
      _ADD(L'\u1E60', L"S", "S");
      _ADD(L'\u1E61', L"s", "s");
      _ADD(L'\u017F', L"s", "s");
      _ADD(L'\u00DF', L"ss", "ss");
      _ADD(L'\u1E9E', L"Ss", "Ss");
      _ADD(L'\u0162', L"T", "T");
      _ADD(L'\u0163', L"t", "t");
      _ADD(L'\u0164', L"T", "T");
      _ADD(L'\u0165', L"t", "t");
      _ADD(L'\u1E6A', L"T", "T");
      _ADD(L'\u1E6B', L"t", "t");
      _ADD(L'\u0166', L"T", "T");
      _ADD(L'\u0167', L"t", "t");
      _ADD(L'\u00DE', L"T", "T");
      _ADD(L'\u00FE', L"t", "t");
      _ADD(L'\u00D9', L"U", "U");
      _ADD(L'\u00F9', L"u", "u");
      _ADD(L'\u00DA', L"U", "U");
      _ADD(L'\u00FA', L"u", "u");
      _ADD(L'\u00DB', L"U", "U");
      _ADD(L'\u00FB', L"u", "u");
      _ADD(L'\u0168', L"U", "U");
      _ADD(L'\u0169', L"u", "u");
      _ADD(L'\u00DC', L"U", "U");
      _ADD(L'\u00FC', L"u", "u");
      _ADD(L'\u016E', L"U", "U");
      _ADD(L'\u016F', L"u", "u");
      _ADD(L'\u016A', L"U", "U");
      _ADD(L'\u016B', L"u", "u");
      _ADD(L'\u016C', L"U", "U");
      _ADD(L'\u016D', L"u", "u");
      _ADD(L'\u0172', L"U", "U");
      _ADD(L'\u0173', L"u", "u");
      _ADD(L'\u0170', L"U", "U");
      _ADD(L'\u0171', L"u", "u");
      _ADD(L'\u1E80', L"W", "W");
      _ADD(L'\u1E81', L"w", "w");
      _ADD(L'\u1E82', L"W", "W");
      _ADD(L'\u1E83', L"w", "w");
      _ADD(L'\u0174', L"W", "W");
      _ADD(L'\u0175', L"w", "w");
      _ADD(L'\u1E84', L"W", "W");
      _ADD(L'\u1E85', L"w", "w");
      _ADD(L'\u1EF2', L"Y", "Y");
      _ADD(L'\u1EF3', L"y", "y");
      _ADD(L'\u00DD', L"Y", "Y");
      _ADD(L'\u00FD', L"y", "y");
      _ADD(L'\u0176', L"Y", "Y");
      _ADD(L'\u0177', L"y", "y");
      _ADD(L'\u0178', L"Y", "Y");
      _ADD(L'\u00FF', L"y", "y");
      _ADD(L'\u0179', L"Z", "Z");
      _ADD(L'\u017A', L"z", "z");
      _ADD(L'\u017B', L"Z", "Z");
      _ADD(L'\u017C', L"z", "z");
      _ADD(L'\u017D', L"Z", "Z");
      _ADD(L'\u017E', L"z", "z");

      _ADD(L'\u0200', L"A", "A");
      _ADD(L'\u0201', L"a", "a");
      _ADD(L'\u0202', L"A", "A");
      _ADD(L'\u0203', L"a", "a");
      _ADD(L'\u0204', L"E", "E");
      _ADD(L'\u0205', L"e", "e");
      _ADD(L'\u0206', L"E", "E");
      _ADD(L'\u0207', L"e", "e");
      _ADD(L'\u0208', L"I", "I");
      _ADD(L'\u0209', L"i", "i");
      _ADD(L'\u020A', L"I", "I");
      _ADD(L'\u020B', L"i", "i");
      _ADD(L'\u020C', L"O", "O");
      _ADD(L'\u020D', L"o", "o");
      _ADD(L'\u020E', L"O", "O");
      _ADD(L'\u020F', L"o", "o");
      _ADD(L'\u0210', L"R", "R");
      _ADD(L'\u0211', L"r", "r");
      _ADD(L'\u0212', L"R", "R");
      _ADD(L'\u0213', L"r", "r");
      _ADD(L'\u0214', L"R", "R");
      _ADD(L'\u0215', L"r", "r");
      _ADD(L'\u0216', L"R", "R");
      _ADD(L'\u0217', L"r", "r");

      _ADD(L'\u0218', L"S", "S");
      _ADD(L'\u0219', L"s", "s");
      _ADD(L'\u021A', L"T", "T");
      _ADD(L'\u021B', L"t", "t");


      _UPPER_ADD(L'\u00C0', L'\u00E0');
      _UPPER_ADD(L'\u00C1', L'\u00E1');
      _UPPER_ADD(L'\u00C2', L'\u00E2');
      _UPPER_ADD(L'\u00C3', L'\u00E3');
      _UPPER_ADD(L'\u00C4', L'\u00E4');
      _UPPER_ADD(L'\u00C5', L'\u00E5');
      _UPPER_ADD(L'\u0100', L'\u0101');
      _UPPER_ADD(L'\u0102', L'\u0103');
      _UPPER_ADD(L'\u0104', L'\u0105');
      _UPPER_ADD(L'\u01DE', L'\u01DF');
      _UPPER_ADD(L'\u01FA', L'\u01FB');
      _UPPER_ADD(L'\u00C6', L'\u00E6');
      _UPPER_ADD(L'\u01FC', L'\u01FD');
      _UPPER_ADD(L'\u1E02', L'\u1E03');
      _UPPER_ADD(L'\u0106', L'\u0107');
      _UPPER_ADD(L'\u00C7', L'\u00E7');
      _UPPER_ADD(L'\u010C', L'\u010D');
      _UPPER_ADD(L'\u0108', L'\u0109');
      _UPPER_ADD(L'\u010A', L'\u010B');
      _UPPER_ADD(L'\u1E10', L'\u1E11');
      _UPPER_ADD(L'\u010E', L'\u010F');
      _UPPER_ADD(L'\u1E0A', L'\u1E0B');
      _UPPER_ADD(L'\u0110', L'\u0111');
      _UPPER_ADD(L'\u00D0', L'\u00F0');
      _UPPER_ADD(L'\u01F1', L'\u01F3');
      _UPPER_ADD(L'\u01C4', L'\u01C6');
      _UPPER_ADD(L'\u00C8', L'\u00E8');
      _UPPER_ADD(L'\u00C9', L'\u00E9');
      _UPPER_ADD(L'\u011A', L'\u011B');
      _UPPER_ADD(L'\u00CA', L'\u00EA');
      _UPPER_ADD(L'\u00CB', L'\u00EB');
      _UPPER_ADD(L'\u0112', L'\u0113');
      _UPPER_ADD(L'\u0114', L'\u0115');
      _UPPER_ADD(L'\u0118', L'\u0119');
      _UPPER_ADD(L'\u0116', L'\u0117');
      _UPPER_ADD(L'\u01B7', L'\u0292');
      _UPPER_ADD(L'\u01EE', L'\u01EF');
      _UPPER_ADD(L'\u1E1E', L'\u1E1F');

      _UPPER_ADD(L'\u01F4', L'\u01F5');
      _UPPER_ADD(L'\u0122', L'\u0123');
      _UPPER_ADD(L'\u01E6', L'\u01E7');
      _UPPER_ADD(L'\u011C', L'\u011D');
      _UPPER_ADD(L'\u011E', L'\u011F');
      _UPPER_ADD(L'\u0120', L'\u0121');
      _UPPER_ADD(L'\u01E4', L'\u01E5');
      _UPPER_ADD(L'\u0124', L'\u0125');
      _UPPER_ADD(L'\u0126', L'\u0127');
      _UPPER_ADD(L'\u00CC', L'\u00EC');
      _UPPER_ADD(L'\u00CD', L'\u00ED');
      _UPPER_ADD(L'\u00CE', L'\u00EE');
      _UPPER_ADD(L'\u0128', L'\u0129');
      _UPPER_ADD(L'\u00CF', L'\u00EF');
      _UPPER_ADD(L'\u012A', L'\u012B');
      _UPPER_ADD(L'\u012C', L'\u012D');
      _UPPER_ADD(L'\u012E', L'\u012F');
      _UPPER_ADD(L'\u0130', L'\u0131');
      _UPPER_ADD(L'\u0132', L'\u0133');
      _UPPER_ADD(L'\u0134', L'\u0135');
      _UPPER_ADD(L'\u1E30', L'\u1E31');
      _UPPER_ADD(L'\u0136', L'\u0137');
      _UPPER_ADD(L'\u01E8', L'\u01E9');
      _UPPER_ADD(L'\u0139', L'\u013A');
      _UPPER_ADD(L'\u013B', L'\u013C');
      _UPPER_ADD(L'\u013D', L'\u013E');
      _UPPER_ADD(L'\u013F', L'\u0140');
      _UPPER_ADD(L'\u0141', L'\u0142');
      _UPPER_ADD(L'\u01C7', L'\u01C9');
      _UPPER_ADD(L'\u1E40', L'\u1E41');
      _UPPER_ADD(L'\u0143', L'\u0144');
      _UPPER_ADD(L'\u0145', L'\u0146');
      _UPPER_ADD(L'\u0147', L'\u0148');
      _UPPER_ADD(L'\u00D1', L'\u00F1');
      _UPPER_ADD(L'\u014A', L'\u014B');
      _UPPER_ADD(L'\u01CA', L'\u01CC');
      _UPPER_ADD(L'\u00D2', L'\u00F2');
      _UPPER_ADD(L'\u00D3', L'\u00F3');
      _UPPER_ADD(L'\u00D4', L'\u00F4');
      _UPPER_ADD(L'\u00D5', L'\u00F5');
      _UPPER_ADD(L'\u00D6', L'\u00F6');
      _UPPER_ADD(L'\u014C', L'\u014D');
      _UPPER_ADD(L'\u014E', L'\u014F');
      _UPPER_ADD(L'\u00D8', L'\u00F8');
      _UPPER_ADD(L'\u0150', L'\u0151');
      _UPPER_ADD(L'\u01FE', L'\u01FF');
      _UPPER_ADD(L'\u0152', L'\u0153');
      _UPPER_ADD(L'\u1E56', L'\u1E57');
      _UPPER_ADD(L'\u0154', L'\u0155');
      _UPPER_ADD(L'\u0156', L'\u0157');
      _UPPER_ADD(L'\u0158', L'\u0159');
      _UPPER_ADD(L'\u015A', L'\u015B');
      _UPPER_ADD(L'\u015E', L'\u015F');
      _UPPER_ADD(L'\u0160', L'\u0161');
      _UPPER_ADD(L'\u015C', L'\u015D');
      _UPPER_ADD(L'\u1E60', L'\u1E61');
      _UPPER_ADD(L'\u0162', L'\u0163');
      _UPPER_ADD(L'\u0164', L'\u0165');
      _UPPER_ADD(L'\u1E6A', L'\u1E6B');
      _UPPER_ADD(L'\u0166', L'\u0167');
      _UPPER_ADD(L'\u00DE', L'\u00FE');
      _UPPER_ADD(L'\u00D9', L'\u00F9');
      _UPPER_ADD(L'\u00DA', L'\u00FA');
      _UPPER_ADD(L'\u00DB', L'\u00FB');
      _UPPER_ADD(L'\u0168', L'\u0169');
      _UPPER_ADD(L'\u00DC', L'\u00FC');
      _UPPER_ADD(L'\u016E', L'\u016F');
      _UPPER_ADD(L'\u016A', L'\u016B');
      _UPPER_ADD(L'\u016C', L'\u016D');
      _UPPER_ADD(L'\u0172', L'\u0173');
      _UPPER_ADD(L'\u0170', L'\u0171');
      _UPPER_ADD(L'\u1E80', L'\u1E81');
      _UPPER_ADD(L'\u1E82', L'\u1E83');
      _UPPER_ADD(L'\u0174', L'\u0175');
      _UPPER_ADD(L'\u1E84', L'\u1E85');
      _UPPER_ADD(L'\u1EF2', L'\u1EF3');
      _UPPER_ADD(L'\u00DD', L'\u00FD');
      _UPPER_ADD(L'\u0176', L'\u0177');
      _UPPER_ADD(L'\u0178', L'\u00FF');
      _UPPER_ADD(L'\u0179', L'\u017A');
      _UPPER_ADD(L'\u017B', L'\u017C');
      _UPPER_ADD(L'\u017D', L'\u017E');

      _UPPER_ADD(L'\u0200', L'\u0201');
      _UPPER_ADD(L'\u0202', L'\u0203');
      _UPPER_ADD(L'\u0204', L'\u0205');
      _UPPER_ADD(L'\u0206', L'\u0207');
      _UPPER_ADD(L'\u0208', L'\u0209');
      _UPPER_ADD(L'\u020A', L'\u020B');
      _UPPER_ADD(L'\u020C', L'\u020D');
      _UPPER_ADD(L'\u020E', L'\u020F');
      _UPPER_ADD(L'\u0210', L'\u0211');
      _UPPER_ADD(L'\u0212', L'\u0213');
      _UPPER_ADD(L'\u0214', L'\u0215');
      _UPPER_ADD(L'\u0216', L'\u0217');

      _UPPER_ADD(L'\u0218', L'\u0219');
      _UPPER_ADD(L'\u021A', L'\u021B');

      _SPACE_ADD(L' ');
      _SPACE_ADD(L'\t');   // tab treated also as a space
      _SPACE_ADD(L'\n');   // <cr> & <lf> also treated as spaces
      _SPACE_ADD(L'\r');

      _SPACE_ADD(L'\xa0');
      _SPACE_ADD(L'\u1680');
      _SPACE_ADD(L'\u180e');
      _SPACE_ADD(L'\u2000');
      _SPACE_ADD(L'\u2001');
      _SPACE_ADD(L'\u2002');
      _SPACE_ADD(L'\u2003');
      _SPACE_ADD(L'\u2004');
      _SPACE_ADD(L'\u2005');
      _SPACE_ADD(L'\u2006');
      _SPACE_ADD(L'\u2007');
      _SPACE_ADD(L'\u2008');
      _SPACE_ADD(L'\u2009');
      _SPACE_ADD(L'\u200a');
      _SPACE_ADD(L'\u2028');
      _SPACE_ADD(L'\u2029');
      _SPACE_ADD(L'\u202f');
      _SPACE_ADD(L'\u205f');
      _SPACE_ADD(L'\u3000');
      _SPACE_ADD(L'\uFEFF');  // does not appear as is_space but it is
                              // (ZERO WIDTH NO-BREAK SPACE)
  }

};
__InitUmlaut __init_umlaut;


}
