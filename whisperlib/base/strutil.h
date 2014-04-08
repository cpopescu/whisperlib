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
// Author: Cosmin Tudorache & Catalin Popescu

//
// We have here a bunch of utilities for manipulating strings
//

#ifndef __WHISPERLIB_BASE_STRUTIL_H__
#define __WHISPERLIB_BASE_STRUTIL_H__

#include <strings.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <whisperlib/base/types.h>
#include WHISPER_HASH_MAP_HEADER
#include WHISPER_HASH_SET_HEADER
#include <whisperlib/base/log.h>
#include <whisperlib/base/strutil_format.h>
#include <stddef.h>
#include <wchar.h>

namespace strutil {

// Convert binary string to hex
string ToHex(const unsigned char* cp, int len);
string ToHex(const string& str);

// Test string equality.
bool StrEql(const char* str1, const char* str2);
bool StrEql(const char* p, const string& s);
bool StrEql(const string& s, const char* p);
bool StrEql(const string& str1, const string& str2);

// Test string equality. Ignore case.
bool StrIEql(const char* str1, const char* str2);
bool StrIEql(const string& str1, const string& str2);

// Tests if this string starts with the specified prefix.
bool StrPrefix(const char* str, const char* prefix);
bool StrStartsWith(const char* str, const char* prefix);
bool StrStartsWith(const string& str, const string& prefix);

// Tests if this string starts with the specified prefix - ignoring case.
bool StrCasePrefix(const char* str, const char* prefix);
bool StrIStartsWith(const char* str, const char* prefix);
bool StrIStartsWith(const string& str, const string& prefix);

// Tests if string ends with the specified sufix.
bool StrSuffix(const char* str, const char* suffix);
bool StrSuffix(const string& str, const string& suffix);
bool StrEndsWith(const char* str, const char* suffix);
bool StrEndsWith(const string& str, const string& suffix);

// Passes over the spaces (and tabs) of the given string
const char* StrFrontTrim(const char* str);
// Passes over the spaces (and tabs) of the given string
string StrFrontTrim(const string& str);

// Removes the front and back spaces (and tabs) of the given string
string StrTrim(const string& str);

// Removes the front and back characters from chars_to_trim for the
// given string str
string StrTrimChars(const string& str, const char* chars_to_trim);

// Removes all spaces from the given string
string StrTrimCompress(const string& str);

// Removes CR LF characters from the end of the string
void StrTrimCRLFInPlace(string& str);
string StrTrimCRLF(const string& str);

// Compares two strings w/o case
inline bool StrCaseEqual(const string& s1, const string& s2) {
  return ( s1.size() == s2.size() &&
           !strncasecmp(s1.c_str(), s2.c_str(), s1.size()) );
}

// Moves data inside buffer drom buf + shift_size to the beginning
// of the buffer and fills w/ fill_value afterwards
void ShiftLeftBuffer(void* buf,
                     size_t buf_size,
                     size_t shift_size,
                     int fill_value);

// Given an array of strings it joins them using the provided glue string
string JoinStrings(const char* pieces[], size_t size, const char* glue);

// Given an array of strings it joins them using the provided glue string
string JoinStrings(const vector<string>& pieces,
                   size_t start, size_t limit,
                   const char* glue);

// Given an array of strings it joins them using the provided glue string
inline string JoinStrings(const vector<string>& pieces, const char* glue) {
    return JoinStrings(pieces, 0, pieces.size(), glue);
}

// Takes a string, a separator and splits the string in constituting componants
// separated by the separator (which is in none of them);
void SplitString(const string& s,
                 const string& separator,
                 vector<string>* output);

// Splits the string on any separator char in separator.
void SplitStringOnAny(const char* text, int size,
                      const char* separators,
                      vector<string>* output,
                      bool skip_empty);

inline void SplitStringOnAny(const string& s,
                             const char* separators,
                             vector<string>* output,
                             bool skip_empty) {
  SplitStringOnAny(s.c_str(), int(s.length()), separators, output, skip_empty);
}

inline void SplitStringOnAny(const string& s,
                             const char* separators,
                             vector<string>* output) {
  SplitStringOnAny(s.c_str(), int(s.length()), separators, output, true);
}



// Splits a string in two - before and after the first occurrence of the
// separator (the separator will not appear at all).
// If separator is not found, returns: pair(s,"").
inline pair<string, string> SplitFirst(const char* s, char separator) {
  const char* slash_pos = strchr(s,  separator);
  if ( !slash_pos ) {
    return make_pair(string(s), string(""));
  }
  return make_pair(string(s, slash_pos - s), string(slash_pos + 1));
}

inline pair<string, string> SplitFirst(const string& s,
                                       const string& separator) {
  const size_t pos_sep = s.find(separator);
  if ( pos_sep != string::npos ) {
    return make_pair(s.substr(0, pos_sep), s.substr(pos_sep + 1));
  } else {
    return make_pair(s, string(""));
  }
}

// Similar to SplitFirst, but splits at the last occurrence of 'separator'.
// If separator is not found, returns: pair(s,"")
inline pair<string, string> SplitLast(const string& s,
                                      const string& separator) {
  const size_t pos_sep = s.rfind(separator);
  if ( pos_sep != string::npos ) {
    return make_pair(s.substr(0, pos_sep), s.substr(pos_sep + 1));
  } else {
    return make_pair(s, string(""));
  }
}


// Splits a string that contains a list of pairs of elements:
// <elem1_1>sep2<elem1_2>sep1<elem2_1>sep2<elem2_2>sep1...
//   ...sep1<elemN_1>sep2<elemN_2>
// If the second element in a pair is missing, the pair(s, '') is used.
void SplitPairs(const string& s,
                const string& elements_separator,  // sep1
                const string& pair_separator,      // sep2
                vector< pair<string, string> >* output);

// Same as above but splits to a map
void SplitPairs(const string& s,
                const string& elements_separator,  // sep1
                const string& pair_separator,      // sep2
                map<string, string>* output);

// Splits a string on separators outside the brackets
// e.g.
// SplitBracketedString("a(b, c, d(3)), d(), e(d(3))", ',', '(', ')')
// will generate:
// ["a(b, c, d(3))", " d()", " e(d(3))"]
// Returns false on error (misplaced brackets etc..
bool SplitBracketedString(const char* s,
                          const char separator,
                          const char open_bracket,
                          const char close_bracket,
                          vector<string>* output);
// Removes outermost brackets from a string
// parsed by SplitBracketedString
string RemoveOutermostBrackets(const string& s,
                               const char open_bracket,
                               const char close_bracket);

// Directory name processing helpers
inline const char* Basename(const char* filename) {
  const char* sep = strrchr(filename, PATH_SEPARATOR);
  return sep ? sep + 1 : filename;
}

inline string Basename(const string& filename) {
  return Basename(filename.c_str());
}

inline string Dirname(const string& filename) {
  string::size_type sep = filename.rfind(PATH_SEPARATOR);
  return filename.substr(0, (sep == string::npos) ? 0 : sep);
}
inline string CutExtension(const string& filename) {
  string::size_type dot_pos = filename.rfind('.');
  return (dot_pos == string::npos) ? filename : filename.substr(0, dot_pos);
}
inline string Extension(const string& filename) {
  string::size_type dot_pos = filename.rfind('.');
  return (dot_pos == string::npos) ? string("") : filename.substr(dot_pos + 1);
}

// normalizes a file path (collapses ../, ./ // etc)  but leaves
// all the prefix '/'
string NormalizePath(const string& path, char sep);
inline string NormalizePath(const string& path) {
  return NormalizePath(path, PATH_SEPARATOR);
}

// Joins system paths together, canonically.
// NOTE: The input paths should be correct, don't abuse NormalizePath().
inline string JoinPaths(const string& path1, const string& path2, char sep) {
  if ( path1.empty() ) return path2;
  if ( path2.empty() ) return path1;
  if ( path1.size() == 1 && path1[0] == sep ) {
    return path1 + path2;
  }
  if ( path1[path1.size()-1] == sep || path2[0] == sep ) {
      return path1 + path2;
  }
  return path1 + sep + path2;
}

inline string JoinPaths(const string& path1, const string& path2) {
  return JoinPaths(path1, path2, PATH_SEPARATOR);
}
inline string JoinPaths(const string& path1, const string& path2, const string& path3) {
  return JoinPaths(path1, JoinPaths(path2, path3, PATH_SEPARATOR), PATH_SEPARATOR);
}

// Joins media paths together, canonically.
// The difference to JoinPaths() is the path separator.
// For now the media path separator is identical to system path separator.
inline string JoinMedia(const string& path1, const string& path2) {
  return JoinPaths(path1, path2, PATH_SEPARATOR);
}

// similar with NormalizePath, but collapses the prefix '/'
string NormalizeUrlPath(const string& path);


// Transforms a data buffer to a printable string (a'la od)
string PrintableDataBuffer(const void* buffer, size_t size);
// Similar to PrintableDataBuffer but returns only the HEXA printing.
string PrintableDataBufferHexa(const void* buffer, size_t size);
// prints all bytes in hexa, on a single line
string PrintableDataBufferInline(const void* buffer, size_t size);
// prints all bytes in hexa, on a single line
string PrintableEscapedDataBuffer(const void* buffer, size_t size);

// Some useful functions for formatted printing in a string ..
string StringPrintf(const char* format, ...);
string StringPrintf(const char* format, va_list args);
void StringAppendf(string* s, const char* format, va_list args);


struct toupper_s {
  int operator()(int c) {
    return ::toupper(c);
  }
};
struct tolower_s {
  int operator()(int c) {
    return ::tolower(c);
  }
};

inline string& StrToUpper(string& s) {
  transform(s.begin(), s.end(), s.begin(), toupper_s());
  return s;
}
inline string StrToUpper(const string& s) {
  string upper;
  transform(s.begin(), s.end(),
            insert_iterator<string>(upper, upper.begin()), toupper_s());
  return upper;
}

inline string& StrToLower(string& s) {
  transform(s.begin(), s.end(), s.begin(), tolower_s());
  return s;
}
inline string StrToLower(const string& s) {
  string lower;
  transform(s.begin(), s.end(),
            insert_iterator<string>(lower, lower.begin()), tolower_s());
  return lower;
}

//////////////////////////////////////////////////////////////////////
//
// Debug printing functions.
//    Small helpers to get the string representation of some objects
//
template <class T>
string StringOf(T object) {
  ostringstream os;
  os << object;
  return os.str();
}
}

// Useful for logging pair s.
// e.g. ToString(const vector< pair<int64, int64> >& );
template<typename A, typename B>
ostream& operator<<(ostream& os, const pair<A,B>& p) {
  return os << "(" << p.first << ", " << p.second << ")";
}

namespace strutil {

template <typename A, typename B>
string ToString(const pair<A,B>& p) {
  ostringstream oss;
  oss << "(" << p.first << ", " << p.second << ")";
  return oss.str();
}

template <typename K, typename V>
string ToString(const map<K, V>& m) {
  ostringstream oss;
  oss << "map #" << m.size() << "{";
  for ( typename map<K, V>::const_iterator it = m.begin(); it != m.end(); ) {
    const K& k = it->first;
    const V& v = it->second;
    oss << k << ": " << v;
    ++it;
    if ( it != m.end() ) {
      oss << ", ";
    }
  }
  oss << "}";
  return oss.str();
}
template <typename K, typename V>
string ToString(const hash_map<K, V>& m) {
  ostringstream oss;
  oss << "hash_map #" << m.size() << "{";
  for ( typename hash_map<K, V>::const_iterator it = m.begin();
        it != m.end(); ) {
    const K& k = it->first;
    const V& v = it->second;
    oss << k << ": " << v;
    ++it;
    if ( it != m.end() ) {
      oss << ", ";
    }
  }
  oss << "}";
  return oss.str();
}
template <typename T>
string ToString(const set<T>& v, uint32 limit = kMaxUInt32) {
  ostringstream oss;
  oss << "set #" << v.size() << "{";
  uint32 i = 0;
  for (typename set<T>::const_iterator it = v.begin(); it != v.end() && i < limit; ++it, ++i) {
    const T& t = *it;
    if (it != v.begin()) {
      oss << ", ";
    }
    oss << t;
  }
  if (i < v.size()) {
      oss << ", ... #" << (v.size() - i) << " items omitted";
  }
  oss << "}";
  return oss.str();
}
template <typename T>
string ToString(const hash_set<T>& v, uint32 limit = kMaxUInt32) {
  ostringstream oss;
  oss << "set #" << v.size() << "{";
  uint32 i = 0;
  for (typename hash_set<T>::const_iterator it = v.begin(); it != v.end() && i < limit; ++it, ++i) {
    const T& t = *it;
    if (it != v.begin()) {
      oss << ", ";
    }
    oss << t;
  }
  if (i < v.size()) {
      oss << ", ... #" << (v.size() - i) << " items omitted";
  }
  oss << "}";
  return oss.str();
}
template <typename T>
string ToString(const vector<T>& v, uint32 limit = kMaxUInt32) {
  ostringstream oss;
  oss << "vector #" << v.size() << "{";
  uint32 i = 0;
  for (typename vector<T>::const_iterator it = v.begin(); it != v.end() && i < limit; ++it, ++i) {
    const T& t = *it;
    if (it != v.begin()) {
        oss << ", ";
    }
    oss << t;
  }
  if (i < v.size()) {
      oss << ", ... #" << (v.size() - i) << " items omitted";
  }
  oss << "}";
  return oss.str();
}
template <typename P1, typename P2>
string ToString(const vector< pair<P1, P2> >& v) {
  ostringstream oss;
  oss << "vector #" << v.size() << "[";
  for ( typename vector< pair<P1, P2> >::const_iterator it = v.begin(); it != v.end(); ) {
    oss << *it;
    ++it;
    if ( it != v.end() ) {
      oss << ", ";
    }
  }
  oss << "]";
  return oss.str();
}
template <typename K, typename V>
string ToStringP(const map<K, V*>& m) {
  ostringstream oss;
  oss << "map #" << m.size() << "{";
  for ( typename map<K, V*>::const_iterator it = m.begin(); it != m.end(); ) {
    const K& k = it->first;
    const V& v = *it->second;
    oss << k << ": " << v;
    ++it;
    if ( it != m.end() ) {
      oss << ", ";
    }
  }
  oss << "}";
  return oss.str();
}
template <typename T>
string ToStringP(const set<T*>& v) {
  ostringstream oss;
  oss << "set #" << v.size() << "{";
  for ( typename set<T*>::const_iterator it = v.begin(); it != v.end(); ) {
    const T& t = **it;
    oss << t;
    ++it;
    if ( it != v.end() ) {
      oss << ", ";
    }
  }
  oss << "}";
  return oss.str();
}
template <typename T>
string ToStringP(const hash_set<T*>& v) {
  ostringstream oss;
  oss << "set #" << v.size() << "{";
  for ( typename hash_set<T*>::const_iterator it = v.begin(); it != v.end(); ) {
    const T& t = **it;
    oss << t;
    ++it;
    if ( it != v.end() ) {
      oss << ", ";
    }
  }
  oss << "}";
  return oss.str();
}
template <typename T>
string ToStringP(const vector<T*>& v) {
  ostringstream oss;
  oss << "vector #" << v.size() << "[";
  for ( typename vector<T*>::const_iterator it = v.begin(); it != v.end(); ) {
    const T& t = **it;
    oss << t;
    ++it;
    if ( it != v.end() ) {
      oss << ", ";
    }
  }
  oss << "]";
  return oss.str();
}
template <typename K, typename V>
string ToStringKeys(const map<K, V>& m) {
  ostringstream oss;
  oss << "map-keys #" << m.size() << "{";
  for ( typename map<K,V>::const_iterator it = m.begin(); it != m.end(); ) {
    const K& k = it->first;
    oss << k;
    ++it;
    if ( it != m.end() ) {
      oss << ", ";
    }
  }
  oss << "}";
  return oss.str();

}
const string& BoolToString(bool value);

// Works with numeric types only: int8, uint8, int16, uint16, ...
// e.g. 0xa3 => "10100011"

extern const char* const kBinDigits[16];

template <typename T>
string ToBinary(T x) {
  T crt = x;
  const size_t buf_size = sizeof(crt) * 8 + 1;
  char buf[buf_size];
  char* p = buf + buf_size - 1;
  *p-- = '\0';
  while ( p > buf ) {
    const char* dig = kBinDigits[crt & 0xf] + 3;
    *p-- = *dig--;
    *p-- = *dig--;
    *p-- = *dig--;
    *p-- = *dig--;
    crt >>= 4;
  }
  return string(buf, buf_size);
}

// e.g. enum Flag { FLAG_A, FLAG_B, FLAG_C };
//      const string& FlagName(Flag f) {..};
//      uint32 v = FLAG_A | FLAG_C;
//
//      //Prints: {FLAG_A, FLAG_C}
//      LOG_INFO << StrBitFlagsName(v, &FlagName);
//
// T: must be an integer type. It holds the bit flags
// FLAG_TYPE: is the flags type (usually an enum).
// get_flag_name: must return the name of the given flag
// returns: human readable array of flag names
template <typename T, typename FLAG_TYPE>
string StrBitFlagsName(T value, const char* (*get_flag_name)(FLAG_TYPE) ) {
    if (value == (T)(-1)) {
        return "{ALL}";
    }
    ostringstream oss;
    oss << "{";
    bool first = true;
    for (uint32 i = 0; i < sizeof(value)*8; i++) {
        const T flag = (T(1) << i);
        if (value & flag) {
            if (!first) oss << ", ";
            oss << get_flag_name(FLAG_TYPE(flag));
            first = false;
        }
    }
    oss << "}";
    return oss.str();
}

// return a string s such that:
//      s > prefix
//  and
//     does not exists s' such that:
//        s > s' > prefix
//
string GetNextInLexicographicOrder(const string& prefix);

// Utility for finding bounds in a map keyed by strings, givven a key prefix
template<class C>
void GetBounds(const string& prefix,
               map<string, C>* m,
               typename map<string, C>::iterator* begin,
               typename map<string, C>::iterator* end) {
  if ( prefix.empty() ) {
    *begin = m->begin();
  } else {
    *begin = m->lower_bound(prefix);
  }
  const string upper_bound = strutil::GetNextInLexicographicOrder(prefix);
  if ( upper_bound.empty() ) {
    *end = m->end();
  } else {
    *end = m->upper_bound(upper_bound);
  }
}
template<class C>
void GetBounds(const string& prefix,
               const map<string, C>& m,
               typename map<string, C>::const_iterator* begin,
               typename map<string, C>::const_iterator* end) {
  if ( prefix.empty() ) {
    *begin = m.begin();
  } else {
    *begin = m.lower_bound(prefix);
  }
  const string upper_bound = strutil::GetNextInLexicographicOrder(prefix);
  if ( upper_bound.empty() ) {
    *end = m.end();
  } else {
    *end = m.upper_bound(upper_bound);
  }
}

//  Replace characters "szCharsToEscape" in "text" with escape sequences
// marked by "escape". The escaped chars are replaced by "escape" character
// followed by their ASCII code as 2 digit text.
//
//  The escape char is replace by "escape""escape".
// e.g.
//  StrEscape("a,., b,c", '#', ",.") => "a#44#46#44 b#44c"
//  StrEscape("a,.# b,c", '#', ",.") => "a#44#46## b#44c"
string StrEscape(const char* text, char escape,
                      const char* chars_to_escape);
string StrEscape(const string& text,
                 char escape,
                 const char* chars_to_escape);
string StrNEscape(const char* text, size_t size, char escape,
                  const char* chars_to_escape);


//  The reverse of StrEscape.
string StrUnescape(const char* text, char escape);
string StrUnescape(const string& text, char escape);

// Escapes a string for JSON encoding
string JsonStrEscape(const char* text, size_t size);
inline string JsonStrEscape(const string& text) {
  return JsonStrEscape(text.c_str(), text.size());
}
string JsonStrUnescape(const char* text, size_t size);
inline string JsonStrUnescape(const string& text) {
  return JsonStrUnescape(text.c_str(), text.length());
}

// Escape a string for XML encoding
string XmlStrEscape(const string& text);

// Returns true if the string is a valid identifier (a..z A..Z 0..9 and _)
bool IsValidIdentifier(const char* s);

// Replaces named variables in the given string with corresponding one
// found in the 'vars' map, much in the vein of python formatters.
//
// E.g.
// string s("We found user ${User} who wants to \${10 access ${Resource}.")
// map<string, string> m;
// m["User"] = "john";
// m["Resource"] = "disk";
// cout << strutil::StrMapFormat(s, m, "${", "}", '\\') << endl;
//
// Would result in:
// We found user john who wants to ${10 access disk.
// escape_char escapes first chars in both arg_begin and arg_end.
//
string StrMapFormat(const char* s,
                    const map<string, string>& m,
                    const char* arg_begin = "${",
                    const char* arg_end = "}",
                    char escape_char = '\\');

// returns a human readable string describing the given 'duration_ms'
// e.g. 83057 -> "1m23.057s"
string StrHumanDuration(int64 duration_ms);

// returns a human readable string describing the given byte size
// e.g. 83057 -> "83KB"
string StrHumanBytes(uint64 bytes);

////////////////////////////////////////////////////////////////////////////////

//  i18n stuff

namespace i18n {

// Returns the length of the given UTF-8 encoded string
size_t Utf8Strlen(const char *s);

// Copies the current codepoint (at most 4 bytes) at s to codepoint,
// returning the next codepoint pointer
const char* Utf8ToCodePoint(const char* start, const char* end, wint_t* codepoint);
// Appends the utf8 encoded value of codepoint to s
size_t CodePointToUtf8(wint_t codepoint, string* s);

// Converts from utf8 to wchar into a newly allocated string (of the right size).
wchar_t* Utf8ToWchar(const char* s, size_t size_bytes);
inline wchar_t* Utf8ToWchar(const string& s) {
    return Utf8ToWchar(s.c_str(), s.length());
}

// Converts from wchar_t* to a string.
inline void WcharToUtf8StringSet(const wchar_t* s, string* result) {
  while (*s) {
    CodePointToUtf8(*s++, result);
  }
}
inline void WcharToUtf8StringSetWithLen(const wchar_t* s, size_t len,
                                        string* result) {
  const wchar_t* p = s + len;
  while (*s && (s < p)) {
    CodePointToUtf8(*s++, result);
  }
}
inline string WcharToUtf8String(const wchar_t* s) {
    string result;
    WcharToUtf8StringSet(s, &result);
    return result;
}

/** Returns true if a utf string is valid */
bool IsUtf8Valid(const string& s);

/** Return the first N chars in utf8 string s */
string Utf8StrPrefix(const string& s, size_t prefix_size);

/** Trims the utf8 s by removing whitespaces at the beginning and the end */
string Utf8StrTrim(const string& s);

/** Splits and trims on whitespaces. In terms will end up only non empty
 * substrings */
void Utf8SplitOnWhitespace(const string& s, vector<string>* terms);

/** Split a string on the first encouter of split  */
pair<string, string> Utf8SplitPairs(const string& s, wchar_t split);

/** Splits and trims on whitespaces. In terms will end up only non empty
 * substrings */
void Utf8SplitOnWChars(const string& s, const wchar_t* splits, vector<string>* terms);

/** The size that needs to be allocated for a utf8 size. */
size_t WcharToUtf8Size(const wchar_t* s);

// Checking stuff
bool IsSpace(const wint_t c);
bool IsAlnum(const wint_t c);

wint_t ToUpper(wint_t c);
wint_t ToLower(wint_t c);

// I18n relevant ToUpper
string ToUpperStr(const string& s);
string ToLowerStr(const string& s);

// This will transform a text from unicode text to equivalent
// ascii (e.g. Zürich -> Zurich).
// If they would be the same we return null, else we return
// a freshly allocated string
wchar_t* UmlautTransform(const wchar_t* s);
// Same as above, but we deal w/ utf8 strings
bool UmlautTransformUtf8(const string& s, string* result);
}

}

# endif  // __WHISPERLIB_BASE_STRUTIL_H__
