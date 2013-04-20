// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines utility functions for working with strings.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <errno.h>

#include <whisperlib/base/strutil_format.h>
#include <whisperlib/base/log.h>

namespace strutil {

// Overloaded wrappers around vsnprintf and vswprintf. The buf_size parameter
// is the size of the buffer. These return the number of characters in the
// formatted string excluding the NUL terminator. If the buffer is not
// large enough to accommodate the formatted string without truncation, they
// return the number of characters that would be in the fully-formatted string
// (vsnprintf, and vswprintf on Windows), or -1 (vswprintf on POSIX platforms).
inline int vsnprintfT(char* buffer,
                      size_t buf_size,
                      const char* format,
                      va_list argptr) {
  return ::vsnprintf(buffer, buf_size, format, argptr);
}


// Templatized backend for StringPrintF/StringAppendF. This does not finalize
// the va_list, the caller is expected to do that.
template <class char_type>
static void StringAppendVT(
    std::basic_string<char_type, std::char_traits<char_type> >* dst,
    const char_type* format,
    va_list ap) {

  // First try with a small fixed size buffer.
  // This buffer size should be kept in sync with StringUtilTest.GrowBoundary
  // and StringUtilTest.StringPrintfBounds.
  char_type stack_buf[1024];

  va_list backup_ap;
  ::va_copy(backup_ap, ap);

#if !defined(OS_WIN)
  errno = 0;
#endif
  int result = vsnprintfT(stack_buf, arraysize(stack_buf), format, backup_ap);
  va_end(backup_ap);

  if (result >= 0 && result < static_cast<int>(arraysize(stack_buf))) {
    // It fit.
    dst->append(stack_buf, result);
    return;
  }

  // Repeatedly increase buffer size until it fits.
  int mem_length = arraysize(stack_buf);
  while (true) {
    if (result < 0) {
#if !defined(OS_WIN)
      // On Windows, vsnprintfT always returns the number of characters in a
      // fully-formatted string, so if we reach this point, something else is
      // wrong and no amount of buffer-doubling is going to fix it.
      if (errno != 0 && errno != EOVERFLOW)
#endif
      {
        // If an error other than overflow occurred, it's never going to work.
        DLOG_WARNING << "Unable to printf the requested string due to error.";
        return;
      }
      // Try doubling the buffer size.
      mem_length *= 2;
    } else {
      // We need exactly "result + 1" characters.
      mem_length = result + 1;
    }

    if (mem_length > 32 * 1024 * 1024) {
      // That should be plenty, don't try anything larger.  This protects
      // against huge allocations when using vsnprintfT implementations that
      // return -1 for reasons other than overflow without setting errno.
      DLOG_WARNING << "Unable to printf the requested string due to size: "
                   << mem_length;
      return;
    }

    std::vector<char_type> mem_buf(mem_length);

    // Restore the va_list before we use it again.
    ::va_copy(backup_ap, ap);

    result = vsnprintfT(&mem_buf[0], mem_length, format, ap);
    va_end(backup_ap);

    if ((result >= 0) && (result < mem_length)) {
      // It fit.
      dst->append(&mem_buf[0], result);
      return;
    }
  }
}

namespace {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-compare"

template <typename STR, typename INT, typename UINT, bool NEG>
struct IntToStringT {

  // This is to avoid a compiler warning about unary minus on unsigned type.
  // For example, say you had the following code:
  //   template <typename INT>
  //   INT abs(INT value) { return value < 0 ? -value : value; }
  // Even though if INT is unsigned, it's impossible for value < 0, so the
  // unary minus will never be taken, the compiler will still generate a
  // warning.  We do a little specialization dance...
  template <typename INT2, typename UINT2, bool NEG2>
  struct ToUnsignedT { };

  template <typename INT2, typename UINT2>
  struct ToUnsignedT<INT2, UINT2, false> {
    static UINT2 ToUnsigned(INT2 value) {
      return static_cast<UINT2>(value);
    }
  };

  template <typename INT2, typename UINT2>
  struct ToUnsignedT<INT2, UINT2, true> {
    static UINT2 ToUnsigned(INT2 value) {
      return static_cast<UINT2>(value < 0 ? -value : value);
    }
  };

  static STR IntToString(INT value) {
    // log10(2) ~= 0.3 bytes needed per bit or per byte log10(2**8) ~= 2.4.
    // So round up to allocate 3 output characters per byte, plus 1 for '-'.
    const int kOutputBufSize = 3 * sizeof(INT) + 1;

    // Allocate the whole string right away, we will right back to front, and
    // then return the substr of what we ended up using.
    STR outbuf(kOutputBufSize, 0);

    bool is_neg = value < 0;
    // Even though is_neg will never be true when INT is parameterized as
    // unsigned, even the presence of the unary operation causes a warning.

    UINT res = ToUnsignedT<INT, UINT, NEG>::ToUnsigned(value);

    for (typename STR::iterator it = outbuf.end();;) {
      --it;
      DCHECK(it != outbuf.begin());
      *it = static_cast<typename STR::value_type>((res % 10) + '0');
      res /= 10;

      // We're done..
      if (res == 0) {
        if (is_neg) {
          --it;
          DCHECK(it != outbuf.begin());
          *it = static_cast<typename STR::value_type>('-');
        }
        return STR(it, outbuf.end());
      }
    }
    // NOTREACHED();
    return STR();
  }
};
#pragma GCC diagnostic pop


}

std::string IntToString(int value) {
  return IntToStringT<std::string, int, unsigned int, true>::
      IntToString(value);
}
std::string UintToString(unsigned int value) {
  return IntToStringT<std::string, unsigned int, unsigned int, false>::
      IntToString(value);
}
std::string Int64ToString(int64 value) {
  return IntToStringT<std::string, int64, uint64, true>::
      IntToString(value);
}
std::string Uint64ToString(uint64 value) {
  return IntToStringT<std::string, uint64, uint64, false>::
      IntToString(value);
}

inline void StringAppendV(std::string* dst, const char* format, va_list ap) {
  StringAppendVT<char>(dst, format, ap);
}


std::string StringPrintf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  std::string result;
  StringAppendV(&result, format, ap);
  va_end(ap);
  return result;
}

const std::string& SStringPrintf(std::string* dst, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  dst->clear();
  StringAppendV(dst, format, ap);
  va_end(ap);
  return *dst;
}

void StringAppendF(std::string* dst, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringAppendV(dst, format, ap);
  va_end(ap);
}

}
