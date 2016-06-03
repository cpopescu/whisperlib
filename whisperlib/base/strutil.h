// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
//
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
//
// We have here a bunch of utilities for manipulating strings.
// Includes a specialized i18n section for utf-8 strings.
//
// IMPORTANT: use the strutil::i18n for utf-8 / correct i18n string
//            manipulation
//

#ifndef __WHISPERLIB_BASE_STRUTIL_H__
#define __WHISPERLIB_BASE_STRUTIL_H__

#include <strings.h>            // strncasecmp
#include <time.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <string.h>

#include "whisperlib/base/types.h"
#include "whisperlib/base/hash.h"
#include WHISPER_HASH_MAP_HEADER
#include WHISPER_HASH_SET_HEADER
#include "whisperlib/base/log.h"
#include "whisperlib/base/strutil_format.h"
#include <stddef.h>
#include <wchar.h>

#if defined __CYGWIN32__ && !defined __CYGWIN__
   /* For backwards compatibility with Cygwin b19 and
      earlier, we define __CYGWIN__ here, so that
      we can rely on checking just for that macro. */
#  define __CYGWIN__  __CYGWIN32__
#endif

#if defined _WIN32 && !defined __CYGWIN__ && !defined(PATH_SEPARATOR)
   /* Use Windows separators on all _WIN32 defining
      environments, except Cygwin. */
#  define PATH_SEPARATOR               '\\'
#endif

#ifndef PATH_SEPARATOR
   /* Assume that not having this is an indicator that all
      are missing. */
#  define PATH_SEPARATOR               '/'
#endif

namespace strutil {

/**
 ** ASCII based functions - for UTF-8 version please look down.
 **/

////////////////////////////////////////////////// String finding / searching

/** Test the equality of two strings */
bool StrEql(const char* str1, const char* str2);
bool StrEql(const char* p, const std::string& s);
bool StrEql(const std::string& s, const char* p);
bool StrEql(const std::string& str1, const std::string& str2);

/** Test the equality of two strings - ignoring case*/
bool StrIEql(const char* str1, const char* str2);
bool StrIEql(const std::string& str1, const std::string& str2);

/** Compares two strings w/o case */
bool StrCaseEqual(const std::string& s1, const std::string& s2);

/** Tests if this string str starts with the specified prefix. */
bool StrPrefix(const char* str, const char* prefix);
bool StrStartsWith(const char* str, const char* prefix);
bool StrStartsWith(const std::string& str, const std::string& prefix);

/** Tests if string str starts with the specified prefix - ignoring case. */
bool StrCasePrefix(const char* str, const char* prefix);
bool StrIStartsWith(const char* str, const char* prefix);
bool StrIStartsWith(const std::string& str, const std::string& prefix);

/** Tests if string str ends with the specified suffix */
bool StrSuffix(const char* str, const char* suffix);
bool StrSuffix(const std::string& str, const std::string& suffix);
bool StrEndsWith(const char* str, const char* suffix);
bool StrEndsWith(const std::string& str, const std::string& suffix);

/** Returns true if the string is a valid identifier (a..z A..Z 0..9 and _) */
bool IsValidIdentifier(const char* s);

////////////////////////////////////////////////// String trimming / replacing


/** Passes over the spaces (and tabs) of the given string,
 * and returns a pointer to the first non-space character in str
 * (uses isspace()) */
const char* StrFrontTrim(const char* str);
/** Passes over the spaces (and tabs) of the given string, and
 * returns a copy of the string without those */
std::string StrFrontTrim(const std::string& str);

/** Removes the front and back spaces (and tabs) of the given string (uses isspace()) **/
std::string StrTrim(const std::string& str);

/** Removes the front and back characters of str that are found in chars_to_trim,
 * and returns a string copy of str without those. */
std::string StrTrimChars(const std::string& str, const char* chars_to_trim);

/** Removes all spaces from the given string (uses isspace()) */
std::string StrTrimCompress(const std::string& str);
/** Removes all spaces from all items in str (uses isspace()) */
void StrTrimCompress(std::vector<std::string>* str);

/** Removes CR LF characters from the end of the string */
void StrTrimCRLFInPlace(std::string& str);
/** Removes CR LF characters from the end of the string and returns the resulting string*/
std::string StrTrimCRLF(const std::string& str);

/** Removes the 'comment_char' and all the character following from str */
void StrTrimCommentInPlace(std::string& str, char comment_char);
/** Removes the 'comment_char' and all the character following from str and
 * returns the resulting string */
std::string StrTrimComment(const std::string& str, char comment_char);

/** Replaces all occurrences of 'find_what' with 'replace_with' in str,
 * and returns the resulting string */
std::string StrReplaceAll(const std::string& str,
                          const std::string& find_what,
                          const std::string& replace_with);
/** Replaces all occurrences of 'find_what' with 'replace_with' in str
 * and modifies str accordingly */
void StrReplaceAllInPlace(std::string& str,
                          const std::string& find_what,
                          const std::string& replace_with);


////////////////////////////////////////////////// String joining / splitting


/** Given an array of strings it joins them using the provided glue string */
std::string JoinStrings(const char* pieces[], size_t size, const char* glue);

/** Given an array of strings it joins them using the provided glue string */
std::string JoinStrings(const std::vector<std::string>& pieces,
                        size_t start, size_t limit, const char* glue);

/** Given a container of strings it joins them using the provided glue string */
std::string JoinStrings(const std::vector<std::string>& pieces, const char* glue);
std::string JoinStrings(const std::initializer_list<std::string>& pieces, const char* glue);

/** Given iterators over string containers it joins them using the provided glue string */
template <typename Iter>
std::string JoinStrings(Iter begin, Iter end, const char* glue);

/** Takes a string, a separator and splits the string in constituting components
 * separated by the separator (which is in none of them). */
void SplitString(const std::string& s, const std::string& separator,
                 std::vector<std::string>* output);
std::vector<std::string> SplitString(const std::string& s, const std::string& separator);

/** Splits a string in a set - each string will be unique, and in a different order */
void SplitStringToSet(const std::string& s, const std::string& separator,
                      std::set<std::string>* output);
std::set<std::string> SplitStringToSet(const std::string& s, const std::string& separator);

/** Splits the string on any separator char in separator, optionally skipping the resulting
 * empty string from output */
void SplitStringOnAny(const char* text, size_t size, const char* separators,
                      std::vector<std::string>* output, bool skip_empty);

/** Splits the string on any separator char in separator, optionally skipping the resulting
 * empty string from output */
void SplitStringOnAny(const std::string& s, const char* separators,
                      std::vector<std::string>* output, bool skip_empty);

/** Splits the string on any separator char in separator and stores the result in output */
void SplitStringOnAny(const std::string& s, const char* separators,
                      std::vector<std::string>* output);

/** Splits a string in two - before and after the first occurrence of the
 * separator (the separator will not appear in either).
 * If separator is not found, it returns: pair(s,"").
 */
std::pair<std::string, std::string> SplitFirst(const char* s, char separator);

/** Splits a string in two - before and after the first occurrence of the
 * separator (the separator will not appear in either).
 * If separator is not found, it returns: pair(s,"").
 */
std::pair<std::string, std::string> SplitFirst(const std::string& s,
                                               const std::string& separator);

/** Similar to SplitFirst, but splits at the last occurrence of 'separator'.
 * If separator is not found, returns: pair(s,"")
 */
std::pair<std::string, std::string> SplitLast(const std::string& s,
                                              const std::string& separator);

/** Splits a string that contains a list of pairs of elements:
 * <elem1_1>sep2<elem1_2>sep1<elem2_1>sep2<elem2_2>sep1...
 *   ...sep1<elemN_1>sep2<elemN_2>
 * If the second element in a pair is missing, the pair(s, '') is used.
 */
void SplitPairs(const std::string& s,
                const std::string& elements_separator,  // sep1
                const std::string& pair_separator,      // sep2
                std::vector< std::pair<std::string, std::string> >* output);

/** Splits a string that contains a list of pairs of elements:
 * <elem1_1>sep2<elem1_2>sep1<elem2_1>sep2<elem2_2>sep1...
 *   ...sep1<elemN_1>sep2<elemN_2>
 * If the second element in a pair is missing, the pair(s, '') is used.
 */
void SplitPairs(const std::string& s,
                const std::string& elements_separator,  // sep1
                const std::string& pair_separator,      // sep2
                std::map<std::string, std::string>* output);

/** Splits a string on separators outside the brackets
 * e.g. SplitBracketedString("a(b, c, d(3)), d(), e(d(3))", ',', '(', ')')
 * will generate:  ["a(b, c, d(3))", " d()", " e(d(3))"]
 * Returns false on error (misplaced brackets etc.. */
bool SplitBracketedString(const char* s, const char separator,
                          const char open_bracket, const char close_bracket,
                          std::vector<std::string>* output);

/** Removes outermost brackets from a string parsed by SplitBracketedString */
std::string RemoveOutermostBrackets(const std::string& s,
                                    const char open_bracket,
                                    const char close_bracket);

////////////////////////////////////////////////// String transformations.

/** To-upper character conversion operator */
struct toupper_s { int operator()(int c) { return ::toupper(c); } };
/** To-lower character conversion operator */
struct tolower_s { int operator()(int c) { return ::tolower(c); } };

/** Transforms a string to ASCII upper - in place */
void StrToUpper(std::string* s);
/** Returns a full uppercase copy of a string */
std::string StrToUpper(const std::string& s);
/** Transforms a string to ASCII lower - in place */
void StrToLower(std::string* s);
/** Returns a full lowercase copy of a string */
std::string StrToLower(const std::string& s);

////////////////////////////////////////////////// Path processing

/** Given a full-path filename it returns a pointer to the basename
 * (i.e. the last path component) */
const char* Basename(const char* filename);
/** Given a full-path filename it returns the basename (i.e. the last path component) */
std::string Basename(const std::string& filename);

/** Given a full-path filename it returns the dirname
 * (i.e. the path components without the last one) */
std::string Dirname(const std::string& filename);
/** Cuts the dot delimited extension from a filename */
std::string CutExtension(const std::string& filename);
/** Returns the dot-delimited extension from a filename */
std::string Extension(const std::string& filename);

/**  Normalizes a file path (collapses ../, ./ // etc)  but leaves
 * all the prefix '/'  ( with a custom path separator character ) */
std::string NormalizePath(const std::string& path, char sep);
/**  Normalizes a file path (collapses ../, ./ // etc)  but leaves
 * all the prefix '/' */
std::string NormalizePath(const std::string& path);

/** similar with NormalizePath, but collapses the prefix '/' */
std::string NormalizeUrlPath(const std::string& path);

/** Joins system paths together, canonically.
 * NOTE: The input paths should be correct, we don't abuse NormalizePath().
 */
std::string JoinPaths(const std::string& path1, const std::string& path2, char sep);

/** Joins system paths together, canonically. */
std::string JoinPaths(const std::string& path1, const std::string& path2);
/** Joins three system paths together, canonically. */
std::string JoinPaths(const std::string& path1, const std::string& path2,
                      const std::string& path3);

////////////////////////////////////////////////// String printing / formatting

/** Prints some components according to the formatting directives in format and the
 * parameters parsed. Some sort of snprintf with string */
std::string StringPrintf(const char* format, ...);
/** Prints some components according to the formatting directives in format and the
 * parameters parsed. Some sort of snprintf with string */
std::string StringPrintf(const char* format, va_list args);
/** Appends a StringPrintf(format, args) to s */
void StringAppendf(std::string* s, const char* format, va_list args);

////////////////////////////////////////////////// DEBUG-intended string printing / formatting

/** Transforms a data buffer to a printable string (a'la od) */
std::string PrintableDataBuffer(const void* buffer, size_t size);
/** Similar to PrintableDataBuffer but returns only the HEXA printing. */
std::string PrintableDataBufferHexa(const void* buffer, size_t size);
/** Prints all bytes from buffer in hexa, on a single line */
std::string PrintableDataBufferInline(const void* buffer, size_t size);
/** Prints all bytes from buffer in hexa, on a single line */
std::string PrintableDataBufferInline(const std::string& s);
/** Prints all bytes from buffer  in hexa, on a single line, but using the \x format */
std::string PrintableEscapedDataBuffer(const void* buffer, size_t size);

/** Convert byte string to its hex representation */
std::string ToHex(const unsigned char* cp, size_t len);
/** Convert byte string to its hex representation */
std::string ToHex(const std::string& str);

/** Converts a numeric type to its binary representation. (e.g. 0xa3 => "10100011")
 * Works with numeric types only: int8, uint8, int16, uint16, ...
 */
template <typename T> std::string ToBinary(T x);

/** StrToBool returns false for ""|"False"|"false"|"0" */
bool StrToBool(const std::string& str);
/** BoolToString returns the string representation of a boolean */
const std::string& BoolToString(bool value);

/** StrHumanBytes:  returns a human readable string describing the given byte size
 * e.g. 83057 -> "83KB" */
std::string StrHumanBytes(size_t bytes);

/** StrOrdinal:  returns the ordinal word
 * e.g. 1 -> 'first'
 *      2 -> 'second'
 *      ...
 *      the maximum is 12.
 * For 0 or v > 12 returns 'nth' */
std::string StrOrdinal(size_t v);

/** StrOrdinalNth:  returns the compact ordinal word starting with the number
    1 -> "1st"
    2 -> "2nd"
    23 -> "23rd"
*/
std::string StrOrdinalNth(size_t x);

template<typename A, typename B>
size_t PercentageI(A a, B b) { return b == 0 ? 100 : (a * 100LL / b); }
template<typename A, typename B>
double PercentageF(A a, B b) { return b == 0 ? 100 : (a * 100.0 / b); }

// e.g. StrPercentageI(3,4) => "3/4(75%)"
//      StrPercentageI(3,2) => "3/2(150%)"
//      StrPercentageI(3,0) => "3/0(100%)"
template<typename A, typename B>
std::string StrPercentage(A a, B b, bool integer) {
    std::string s;
    s.reserve(128);
    s.append(std::to_string(a));
    s.append("/");
    s.append(std::to_string(b));
    s.append("(");
    if (integer) {
        s.append(std::to_string(PercentageI(a, b)));
    } else {
        s.append(std::to_string(PercentageF(a, b)));
    }
    s.append("%)");
    return s;
}
template<typename A, typename B>
std::string StrPercentageI(A a, B b) { return StrPercentage(a, b, true); }
template<typename A, typename B>
std::string StrPercentageF(A a, B b) { return StrPercentage(a, b, false); }

}   // namespace strutil



////////////////////////////////////////////////// DEBUG-intended printing of structures

/** Useful for logging pairs (E.g. ToString(const std::vector< std::pair<int64, int64> >& )) */
template<typename A, typename B>
std::ostream& operator << (std::ostream& os, const std::pair<A, B>& p) {
  return os << "(" << p.first << ", " << p.second << ")";
}

namespace strutil {
/** Returns a string version of the object (if it supports << streaming operator) */
template <class T> std::string StringOf(T object);

// These ToString function return a string of the content of the provided structure, in
//  a format suitable for debug printing

template <typename A, typename B> std::string ToString(const std::pair<A, B>& p);
template <typename K, typename V> std::string ToString(const std::map<K, V>& m);
template <typename K, typename V> std::string ToString(const hash_map<K, V>& m);
template <typename T> std::string ToString(const std::set<T>& v,
                                           size_t limit = std::string::npos,
                                           bool multi_line = false);
template <typename T> std::string ToString(const hash_set<T>& v,
                                           size_t limit = std::string::npos,
                                           bool multi_line = false);
template <typename T> std::string ToString(const std::vector<T>& v,
                                           size_t limit = std::string::npos,
                                           bool multi_line = false);
template <typename T> std::string ToString(const std::list<T>& v,
                                           size_t limit = std::string::npos,
                                           bool multi_line = false);
template <typename K, typename V>  std::string ToStringP(const std::map<K, V*>& m);
template <typename K, typename V>  std::string ToStringP(const hash_map<K, V*>& m);
template <typename T> std::string ToStringP(const std::set<T*>& v);
template <typename T> std::string ToStringP(const hash_set<T*>& v);
template <typename T> std::string ToStringP(const std::vector<T*>& vec);
template <typename T> std::string ToStringP(const std::list<T*>& vec);

template <typename CT> std::string ToStringKeys(const CT& m,
                                                size_t limit = std::string::npos);

/** Returns the '|' string representation of an integer that is an or of
 * a set of flags, using a custom naming function.
 *
 * e.g. enum Flag { FLAG_A, FLAG_B, FLAG_C };
 *      const std::string& FlagName(Flag f) {..};
 *      uint32 v = FLAG_A | FLAG_C;
 *
 *      LOG_INFO << StrBitFlagsName(v, &FlagName);
 *      // Prints: {FLAG_A, FLAG_C}
 *
 * @param T: must be an integer type. It holds the bit flags
 * @param FLAG_TYPE: is the flags type (usually an enum).
 * @param all_flags: a T value containing all the flags
 * @param get_flag_name: must return the name of the given flag
 *
 * @return: human readable array of flag names
 */
template <typename T, typename FLAG_TYPE>
std::string StrBitFlagsName(T all_flags, T value,
                            const char* (*get_flag_name)(FLAG_TYPE));

////////////////////////////////////////////////// String finding

/** Return a string s such that:
 *       s > prefix
 *  and does not exists s' such that:
 *        s > s' > prefix
 *  NOTE: this is ascii only no utf-8, lexicographic order is a complicated thing that
 *        needs to be done with something like icu for a specific locale.
 */
std::string GetNextInLexicographicOrder(const std::string& prefix);

/** Utility for finding bounds in a map keyed by strings, givven a key prefix */
template<class C> void GetBounds(const std::string& prefix,
                                 std::map<std::string, C>* m,
                                 typename std::map<std::string, C>::iterator* begin,
                                 typename std::map<std::string, C>::iterator* end);
/** Utility for finding bounds in a map keyed by strings, givven a key prefix
 * - const_iterator flavor. */
template<class C> void GetBounds(const std::string& prefix,
                                 const std::map<std::string, C>& m,
                                 typename std::map<std::string, C>::const_iterator* begin,
                                 typename std::map<std::string, C>::const_iterator* end);

////////////////////////////////////////////////// String escaping


/** Escapes a string for JSON encoding */
std::string JsonStrEscape(const char* text, size_t size);
/** Escapes a string for JSON encoding */
std::string JsonStrEscape(const std::string& text);

/** Unescapes a string from JSON */
std::string JsonStrUnescape(const char* text, size_t size);
/** Unescapes a string from JSON */
std::string JsonStrUnescape(const std::string& text);

/** Escape a string for XML encoding */
std::string XmlStrEscape(const std::string& text);

/**  Replace characters "szCharsToEscape" in "text" with escape sequences
 * marked by "escape". The escaped chars are replaced by "escape" character
 * followed by their ASCII code as 2 digit text.
 *
 *  The escape char is replace by "escape""escape".
 *  StrEscape("a,., b,c", '#', ",.") => "a#44#46#44 b#44c"
 *  StrEscape("a,.# b,c", '#', ",.") => "a#44#46## b#44c"
 */
std::string StrEscape(const char* text, char escape, const char* chars_to_escape);
std::string StrEscape(const std::string& text, char escape, const char* chars_to_escape);
std::string StrNEscape(const char* text, size_t size, char escape, const char* chars_to_escape);


/**  The reverse of StrEscape. **/
std::string StrUnescape(const char* text, char escape);
/**  The reverse of StrEscape. **/
std::string StrUnescape(const std::string& text, char escape);

/** Replaces named variables in the given string with corresponding one
 * found in the 'vars' map, much in the vein of python formatters.
 *
 * E.g.
 * string s("We found user ${User} who wants to \${10 access ${Resource}.")
 * std::map<string, string> m;
 * m["User"] = "john";
 * m["Resource"] = "disk";
 * cout << strutil::StrMapFormat(s, m, "${", "}", '\\') << endl;
 *
 * Would result in:
 * We found user john who wants to ${10 access disk.
 * escape_char escapes first chars in both arg_begin and arg_end.
 */
std::string StrMapFormat(const char* s,
                    const std::map<std::string, std::string>& m,
                    const char* arg_begin = "${",
                    const char* arg_end = "}",
                    char escape_char = '\\');

////////////////////////////////////////////////// i18n / UTF8
//
// General i18n rules:
//   - strings that that are used to represent text, hold that text
//     in utf8 encoding (string-s in protobuf are like that too).
//   - for all user facing / serious text holding operations use these
//     functions to manipulate text (using something like strutil::SplitString
//     on a utf8 string may and will result in many non-utf8 strings, so
//     use strutil::i18n::Utf8Split* functions)
//   - for unicode character holding use and constant passing use
//     wchar_t* / wint_t (e.g. L"abc", L'-')
//   - for i18n manipulation all utf8 string need to be converted to wchar_t
//     (i.e. unicode characters) and then back to utf8 string.
//   - for serious i18n work (e.g. serious transliteration, user facing lexicographic
//     ordering that that is not for the faint of hart) use icu library.
//
//
namespace i18n {

/** Returns the length of the given UTF-8 encoded string */
size_t Utf8Strlen(const char *s);
/** Returns the length of the given UTF-8 encoded string */
inline size_t Utf8Strlen(const std::string& s);

/** The size that needs to be allocated for a utf8 size. */
size_t WcharToUtf8Size(const wchar_t* s);

/** If the given unicode character can be considered a space (text separator */
bool IsSpace(const wint_t c);
/** If the given unicode character can be considered an alphanumeric character */
bool IsAlnum(const wint_t c);

/** Copies the current codepoint (at most 4 bytes) at s to codepoint,
 * returning the next codepoint pointer */
const char* Utf8ToCodePoint(const char* start, const char* end, wint_t* codepoint);
/** Appends the utf8 encoded value of codepoint to s */
size_t CodePointToUtf8(wint_t codepoint, std::string* s);
/** Returns the utf8 encoded value */
inline std::string CodePointToUtf8(wint_t codepoint) {
  std::string s;
  CodePointToUtf8(codepoint, &s);
  return s;
}
/** Appends the utf8 encoded value of codepoint string pointed by p
 * (enough space must be available in p - up to 6 bytes).
 */
char* CodePointToUtf8Char(wint_t codepoint, char* p);

/** Converts from utf8 to wchar into a newly allocated string (of the right size). */
wchar_t* Utf8ToWchar(const char* s, size_t size_bytes);
/** Converts from utf8 to wchar into a newly allocated string (of the right size). */
wchar_t* Utf8ToWchar(const std::string& s);

/** Converts from utf8 to wchar into a newly allocated string (of the right size).
 *  Returns also the size of the string in wchar_t.
 */
std::pair<wchar_t*, size_t> Utf8ToWcharWithLen(const char* s, size_t size_bytes);
/** Converts from utf8 to wchar into a newly allocated string (of the right size).
 *  Returns also the size of the string in wchar_t.
 */
std::pair<wchar_t*, size_t> Utf8ToWcharWithLen(const std::string& s);


/** Converts from wchar_t* to a mutable string. */
void WcharToUtf8StringSet(const wchar_t* s, std::string* result);
/** Converts from wchar_t* to a mutable string, with a maximum input lenght */
void WcharToUtf8StringSetWithLen(const wchar_t* s, size_t len, std::string* result);
/** Converts from wchar_t* to a returned string. */
std::string WcharToUtf8String(const wchar_t* s);
/** Converts from wchar_t* to a mutable string, with a maximum input lenght */
std::string WcharToUtf8StringWithLen(const wchar_t* s, size_t len);

/** Returns true if a utf string is valid */
bool IsUtf8Valid(const std::string& s);

/** Converts ISO-8859-1 to Utf8 */
std::string Iso8859_1ToUtf8(const std::string& s);

/** Return the first N chars in utf8 string s */
std::string Utf8StrPrefix(const std::string& s, size_t prefix_size);

/** Trims the utf8 s by removing whitespaces at the beginning and the end */
std::string Utf8StrTrim(const std::string& s);
/** Trims the utf8 s by removing whitespaces at the beginning */
std::string Utf8StrTrimFront(const std::string& s);
/** Trims the utf8 s by removing whitespaces at the end */
std::string Utf8StrTrimBack(const std::string& s);


/** Splits and trims on whitespaces. In terms will end up only non empty substrings */
void Utf8SplitOnWhitespace(const std::string& s, std::vector<std::string>* terms);

/** Splits and trims on whitespaces. In terms will end up only non empty substrings */
void Utf8SplitString(const std::string& s, const std::string& split, std::vector<std::string>* terms);

/** Split a string on the first encouter of split  */
std::pair<std::string, std::string> Utf8SplitPairs(const std::string& s, wchar_t split);

/** Splits and trims on whitespaces. In terms will end up only non empty substrings */
void Utf8SplitOnWChars(const std::string& s, const wchar_t* splits,
                       std::vector<std::string>* terms);

/** Compares two utf8 string w/o expanding them all, and considering umlauts / spaces
 * i.e. e < é < f.
 *  NOTE: this may not be correct for some languages !
 *        lexicographic order is a complicated thing that needs to be done with something
 *        like icu for a specific locale, this is just a quick helper that works in most
 *        non user-facing cases.
 */
int Utf8Compare(const std::string& s1, const std::string& s2, bool umlaut_expand);

/** Returns the uppercase for the provided unicode character,
 * keeping the same accents and such. (e.g. e=>E and é=>É) */
wint_t ToUpper(wint_t c);
/** Returns the lowercase for the provided unicode character,
 * keeping the same accents and such. (e.g. É=>é and E=>e) */
wint_t ToLower(wint_t c);

/** I18n relevant uppercase transformation for a UTF8 string */
std::string ToUpperStr(const std::string& s);
/** I18n relevant lowercase transformation for a UTF8 string */
std::string ToLowerStr(const std::string& s);

/**
 * This will transform a text from unicode text to equivalent
 * ascii (e.g. Zürich -> Zurich).
 * If they would be the same we return null, else we return
 * a freshly allocated string. (dispose the returning string with delete [])
 * Note that this is not standard, just a quick hack. For proper
 * transliteration use icu library.
 */
wchar_t* UmlautTransform(const wchar_t* s);
/** Same as above, but accepts / returns UTF8 strings */
bool UmlautTransformUtf8(const std::string& s, std::string* result);

/** Iterates the unicode wchar charactes of a utf-8 string.
 *  Example for efficiently finding out if there is any unicode
 *  space in a string.
 *
 *  bool HasSpace(const std::string& s) {
 *    strutil::i18n::Utf8ToWcharIterator it(s);
 *    while (it.IsValid()) {
 *      if (strutil::i18n::IsSpace(it->ValueAndNext())) {
 *         return true;
 *      }
 *    }
 *    return false;
 *  }
 */
class Utf8ToWcharIterator {
    const char* p_;
    const char* const end_;
public:
    explicit Utf8ToWcharIterator(const std::string& s)
        : p_(s.c_str()), end_(p_ + s.size()) {
    }
    explicit Utf8ToWcharIterator(const char* s)
        : p_(s), end_(s + strlen(s)) {
    }
    bool IsValid() const {
        return p_ < end_;
    }
    /** Gets the current wchar value, and advances the
     * iterator (unified for efficiency);
     */
    wint_t ValueAndNext() {
        wint_t codepoint;
        p_ = Utf8ToCodePoint(p_, end_, &codepoint);
        return codepoint;
    }
protected:
    DISALLOW_EVIL_CONSTRUCTORS(Utf8ToWcharIterator);
};

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Inlined functions -- Leave always a commented declarationsabove these lines --
//   (even when the inline definition is bellow).
//
inline bool StrCaseEqual(const std::string& s1, const std::string& s2) {
  return ( s1.size() == s2.size() &&
           !strncasecmp(s1.c_str(), s2.c_str(), s1.size()) );
}
inline std::string JoinStrings(const std::vector<std::string>& pieces, const char* glue) {
    return JoinStrings(pieces.begin(), pieces.end(), glue);
}
inline std::string JoinStrings(const std::initializer_list<std::string>& pieces, const char* glue) {
    return JoinStrings(pieces.begin(), pieces.end(), glue);
}
template<typename Iter>
inline std::string JoinStrings(Iter begin, Iter end, const char* glue) {
    std::string s;
    for (Iter it = begin; it != end;) {
        s += *it;
        if (++it != end) { s += glue; }
    }
    return s;
}

inline void SplitStringOnAny(const std::string& s, const char* separators,
                             std::vector<std::string>* output, bool skip_empty) {
  SplitStringOnAny(s.c_str(), int(s.length()), separators, output, skip_empty);
}
inline void SplitStringOnAny(const std::string& s, const char* separators,
                             std::vector<std::string>* output) {
  SplitStringOnAny(s.c_str(), int(s.length()), separators, output, true);
}
inline std::pair<std::string, std::string> SplitFirst(const char* s, char separator) {
  const char* slash_pos = strchr(s,  separator);
  if ( !slash_pos ) {
    return make_pair(std::string(s), std::string());
  }
  return make_pair(std::string(s, slash_pos - s), std::string(slash_pos + 1));
}

inline std::pair<std::string, std::string> SplitFirst(const std::string& s,
                                                      const std::string& separator) {
  const size_t pos_sep = s.find(separator);
  if ( pos_sep != std::string::npos ) {
    return make_pair(s.substr(0, pos_sep), s.substr(pos_sep + 1));
  } else {
    return make_pair(s, std::string());
  }
}
inline std::pair<std::string, std::string> SplitLast(const std::string& s,
                                                     const std::string& separator) {
  const size_t pos_sep = s.rfind(separator);
  if ( pos_sep != std::string::npos ) {
    return make_pair(s.substr(0, pos_sep), s.substr(pos_sep + 1));
  } else {
    return make_pair(s, std::string());
  }
}
inline void StrTrimCompress(std::vector<std::string>* str) {
  for (std::vector<std::string>::iterator it = str->begin(); it != str->end(); ++it) {
    *it = StrTrimCompress(*it);
  }
}

////////////////////////////////////////

inline void StrToUpper(std::string* s) {
  std::transform(s->begin(), s->end(), s->begin(), toupper_s());
}
inline std::string StrToUpper(const std::string& s) {
  std::string upper;
  std::transform(s.begin(), s.end(),
                 std::insert_iterator<std::string>(
                   upper, upper.begin()), toupper_s());
  return upper;
}
inline void StrToLower(std::string* s) {
  std::transform(s->begin(), s->end(), s->begin(), tolower_s());
}
inline std::string StrToLower(const std::string& s) {
  std::string lower;
  std::transform(s.begin(), s.end(),
                 std::insert_iterator<std::string>(
                   lower, lower.begin()), tolower_s());
  return lower;
}

////////////////////////////////////////

inline const char* Basename(const char* filename) {
  const char* sep = strrchr(filename, PATH_SEPARATOR);
  return sep ? sep + 1 : filename;
}
inline std::string Basename(const std::string& filename) {
  return Basename(filename.c_str());
}
inline std::string Dirname(const std::string& filename) {
  std::string::size_type sep = filename.rfind(PATH_SEPARATOR);
  return filename.substr(0, (sep == std::string::npos) ? 0 : sep);
}
inline std::string CutExtension(const std::string& filename) {
  std::string::size_type dot_pos = filename.rfind('.');
  return (dot_pos == std::string::npos) ? filename : filename.substr(0, dot_pos);
}
inline std::string Extension(const std::string& filename) {
  std::string::size_type dot_pos = filename.rfind('.');
  return (dot_pos == std::string::npos) ? std::string("") : filename.substr(dot_pos + 1);
}
inline std::string NormalizePath(const std::string& path) {
  return NormalizePath(path, PATH_SEPARATOR);
}
inline std::vector<std::string> SplitString(const std::string& s, const std::string& separator) {
    std::vector<std::string> output;
    SplitString(s, separator, &output);
    return output;
}
inline void SplitStringToSet(const std::string& s, const std::string& separator,
                             std::set<std::string>* output) {
  std::vector<std::string> v;
  strutil::SplitString(s, separator, &v);
  output->insert(v.begin(), v.end());
}
inline std::set<std::string> SplitStringToSet(const std::string& s, const std::string& separator) {
    std::set<std::string> output;
    SplitStringToSet(s, separator, &output);
    return output;
}
inline std::string JoinPaths(const std::string& path1, const std::string& path2, char sep) {
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
inline std::string JoinPaths(const std::string& path1, const std::string& path2) {
  return JoinPaths(path1, path2, PATH_SEPARATOR);
}
inline std::string JoinPaths(const std::string& path1, const std::string& path2,
                             const std::string& path3) {
  return JoinPaths(path1, JoinPaths(path2, path3, PATH_SEPARATOR), PATH_SEPARATOR);
}
template <class T> std::string StringOf(T object) {
  std::ostringstream os; os << object; return os.str();
}

template <typename A, typename B>
std::string ToString(const std::pair<A,B>& p) {
  std::ostringstream oss;
  oss << "(" << p.first << ", " << p.second << ")";
  return oss.str();
}

#define DEFINE_CONTAINER_TO_STRING(fname, accessor)                     \
template <typename Cont>                                                \
std::string fname(const Cont& m, const char* name,                      \
                  size_t limit = std::string::npos,                     \
                  bool multi_line = false) {                            \
    std::ostringstream oss;                                             \
    const size_t sz = m.size();                                         \
    oss << name << " #" << sz << "{";                                   \
    size_t ndx = 0;                                                     \
    for (typename Cont::const_iterator it = m.begin();                  \
         it != m.end() && ndx < limit; ++ndx, ++it) {                   \
        if (multi_line) oss << "\n - ";                                 \
        else if (it != m.begin()) oss << ", ";                          \
        oss << (accessor);                                              \
    }                                                                   \
    if (ndx < sz) oss << (multi_line ? "\n - " : ", ")                  \
                      << "... #" << (sz - ndx) << " items omitted";     \
    oss << "}";                                                         \
    return oss.str();                                                   \
}

DEFINE_CONTAINER_TO_STRING(ContainerToString, *it)
DEFINE_CONTAINER_TO_STRING(ContainerPPairToString, *(it->second))
DEFINE_CONTAINER_TO_STRING(ContainerKeysToString, (it->first))
DEFINE_CONTAINER_TO_STRING(ContainerPToString, *(*it))
#undef DEFINE_CONTAINER_TO_STRING

template <typename K, typename V> std::string ToString(const std::map<K, V>& m) {
  return ContainerToString(m, "map");
}
template <typename K, typename V> std::string ToString(const std::multimap<K, V>& m) {
  return ContainerToString(m, "multimap");
}
template <typename K, typename V> std::string ToString(const hash_map<K, V>& m) {
  return ContainerToString(m, "hash_map");
}
template <typename T> std::string ToString(const std::set<T>& v, size_t limit, bool multi_line) {
  return ContainerToString(v, "set", limit, multi_line);
}
template <typename T> std::string ToString(const hash_set<T>& v, size_t limit, bool multi_line) {
  return ContainerToString(v, "hash_set", limit, multi_line);
}
template <typename T> std::string ToString(const std::vector<T>& v, size_t limit, bool multi_line) {
  return ContainerToString(v, "vector", limit, multi_line);
}
template <typename T> std::string ToString(const std::list<T>& v, size_t limit, bool multi_line) {
  return ContainerToString(v, "list", limit, multi_line);
}
template <typename K, typename V> std::string ToStringP(const std::map<K, V*>& m) {
  return ContainerPPairToString(m, "map");
}
template <typename K, typename V> std::string ToStringP(const hash_map<K, V*>& m) {
  return ContainerPPairToString(m, "hash_map");
}
template <typename T> std::string ToStringP(const std::set<T*>& v) {
  return ContainerPToString(v, "set");
}
template <typename T> std::string ToStringP(const hash_set<T*>& v) {
  return ContainerPToString(v, "hash_set");
}
template <typename T> std::string ToStringP(const std::vector<T*>& v) {
  return ContainerPToString(v, "vector");
}
template <typename T> std::string ToStringP(const std::list<T*>& v) {
  return ContainerPToString(v, "list");
}
template <typename CT> std::string ToStringKeys(const CT& m, size_t limit) {
  return ContainerKeysToString(m, "map-keys", limit);
}

extern const char* const kBinDigits[16];

template <typename T>
std::string ToBinary(T x) {
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
  return std::string(buf, buf_size);
}

////////////////////////////////////////

template <typename T, typename FLAG_TYPE>
std::string StrBitFlagsName(T all_flags, T value, const char* (*get_flag_name)(FLAG_TYPE) ) {
    if (value == all_flags) {
        return "{ALL}";
    }
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (size_t i = 0; i < sizeof(value)*8; i++) {
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

template<class C> void GetBounds(const std::string& prefix,
                                 std::map<std::string, C>* m,
                                 typename std::map<std::string, C>::iterator* begin,
                                 typename std::map<std::string, C>::iterator* end) {
  if ( prefix.empty() ) {
    *begin = m->begin();
  } else {
    *begin = m->lower_bound(prefix);
  }
  const std::string upper_bound = strutil::GetNextInLexicographicOrder(prefix);
  if ( upper_bound.empty() ) {
    *end = m->end();
  } else {
    *end = m->upper_bound(upper_bound);
  }
}
template<class C> void GetBounds(const std::string& prefix,
                                 const std::map<std::string, C>& m,
                                 typename std::map<std::string, C>::const_iterator* begin,
                                 typename std::map<std::string, C>::const_iterator* end) {
  if ( prefix.empty() ) {
    *begin = m.begin();
  } else {
    *begin = m.lower_bound(prefix);
  }
  const std::string upper_bound = strutil::GetNextInLexicographicOrder(prefix);
  if ( upper_bound.empty() ) {
    *end = m.end();
  } else {
    *end = m.upper_bound(upper_bound);
  }
}

////////////////////////////////////////

inline std::string JsonStrEscape(const std::string& text) {
  return JsonStrEscape(text.c_str(), text.size());
}
inline std::string JsonStrUnescape(const std::string& text) {
  return JsonStrUnescape(text.c_str(), text.length());
}

////////////////////////////////////////

namespace i18n {

inline std::pair<wchar_t*, size_t> Utf8ToWcharWithLen(const std::string& s) {
  return Utf8ToWcharWithLen(s.c_str(), s.length());
}
inline wchar_t* Utf8ToWchar(const std::string& s) {
  return Utf8ToWcharWithLen(s.c_str(), s.length()).first;
}
inline wchar_t* Utf8ToWchar(const char* s, size_t size_bytes) {
  return Utf8ToWcharWithLen(s, size_bytes).first;
}
inline void WcharToUtf8StringSet(const wchar_t* s, std::string* result) {
  while (*s) {
    CodePointToUtf8(*s++, result);
  }
}
inline void WcharToUtf8StringSetWithLen(const wchar_t* s, size_t len,
                                        std::string* result) {
  const wchar_t* p = s + len;
  while (*s && (s < p)) {
    CodePointToUtf8(*s++, result);
  }
}
inline std::string WcharToUtf8String(const wchar_t* s) {
    std::string result;
    WcharToUtf8StringSet(s, &result);
    return result;
}
inline std::string WcharToUtf8StringWithLen(const wchar_t* s, size_t len) {
    std::string result;
    WcharToUtf8StringSetWithLen(s, len, &result);
    return result;
}
inline size_t Utf8Strlen(const std::string& s) {
  return Utf8Strlen(s.c_str());
}
}
}

# endif  // __WHISPERLIB_BASE_STRUTIL_H__
