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

#ifndef __WHISPERLIB_BASE_SYSTEM_H__
#define __WHISPERLIB_BASE_SYSTEM_H__

#include <string>
#include <stdarg.h>

#if defined(HAVE_NAMESER8_COMPAT_H)
#include <nameser8_compat.h>
#elif defined(HAVE_ENDIAN_H)
#include <endian.h>
#elif defined(HAVE_MACHINE_ENDIAN_H)
#include <machine/endian.h>
#endif

#include <whisperlib/base/types.h>

namespace common {

// This function does all the good stuff needed when a program is
// started:
//   -- initialize log
//   -- set signal handlers
//   -- process flags
//   -- writes some stuff in the log
void Init(int& argv, char**& argc);
int Exit(int error, bool forced = false);

// considering int value 0x01020304
enum ByteOrder {
  BIGENDIAN,  // written as: 0x01, 0x02, 0x03, 0x04 ;
              //                     PowerPC, SGI Origin, Sun Sparc
  LILENDIAN,  // written as: 0x04, 0x03, 0x02, 0x01 ; Intel x86, Alpha AXP
  PDPENDIAN,  // written as: 0x03, 0x04, 0x01, 0x02 ; ARM -- NO SUPPORTED
  UNKNOWNENDIAN
};

// returns:
//  the name of the given byte order
const char* ByteOrderName(ByteOrder order);

//////////////////////////////////////////////////////////////////////
//
// Determine byte order at compile time
//
#ifdef BYTE_ORDER
# if defined BYTE_ORDER && defined LITTLE_ENDIAN && defined BIG_ENDIAN
#   if BYTE_ORDER == LITTLE_ENDIAN
#      define LOCAL_MACHINE_BYTE_ORDER_DEFINED
const ByteOrder kByteOrder = LILENDIAN;
#   elif BYTE_ORDER == BIG_ENDIAN
#     define LOCAL_MACHINE_BYTE_ORDER_DEFINED
const ByteOrder kByteOrder = BIGENDIAN;
#   endif
# endif
#else
# if defined __BYTE_ORDER && defined __LITTLE_ENDIAN && defined __BIG_ENDIAN
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#      define LOCAL_MACHINE_BYTE_ORDER_DEFINED
const ByteOrder kByteOrder = LILENDIAN;
#   elif __BYTE_ORDER == __BIG_ENDIAN
#     define LOCAL_MACHINE_BYTE_ORDER_DEFINED
const ByteOrder kByteOrder = BIGENDIAN;
#   endif
# endif
#endif

# if !defined LOCAL_MACHINE_BYTE_ORDER_DEFINED
#   error "We should have the byteorder defined at compile time"
# endif
}

#endif  // __WHISPERLIB_BASE_SYSTEM_H__
