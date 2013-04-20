// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines utility functions for working with strings.
//
// 1618labs node: Picked from chromium/src/base/string_util.h
//
//////////////////////////////////////////////////////////////////////

#ifndef __CORE_EXT_BASE_STRING_UTIL_H__
#define __CORE_EXT_BASE_STRING_UTIL_H__

#include <string>
#include <vector>
#include <stdarg.h>   // va_list

#include <whisperlib/base/types.h>

#ifdef HAVE_GLIB_2_0_GLIB_GMACROS_H
#include <glib-2.0/glib/gmacros.h>
#else
#define G_GNUC_PRINTF(p1, p2)
#endif

namespace strutil {

// Wrapper for vsnprintf that always null-terminates and always returns the
// number of characters that would be in an untruncated formatted
// string, even when truncation occurs.
int vsnprintf(char* buffer, size_t size, const char* format, va_list arguments);

// Some of these implementations need to be inlined.

inline int snprintf(char* buffer, size_t size, const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}


// Specialized string-conversion functions.
std::string IntToString(int value);
std::string UintToString(unsigned int value);
std::string Int64ToString(int64 value);
std::string Uint64ToString(uint64 value);

// Return a C++ string given printf-like input.
std::string StringPrintf(const char* format, ...) G_GNUC_PRINTF (1, 2);


// Store result into a supplied string and return it
const std::string& SStringPrintf(std::string* dst,
                                 const char* format, ...) G_GNUC_PRINTF (2, 3);;

// Append result to a supplied string
void StringAppendF(std::string* dst,
                   const char* format, ...) G_GNUC_PRINTF (2, 3);;

// Lower-level routine that takes a va_list and appends to a specified
// string.  All other routines are just convenience wrappers around it.
void StringAppendV(std::string* dst, const char* format, va_list ap);

}


#endif  // __CORE_EXT_BASE_STRING_UTIL_H__
