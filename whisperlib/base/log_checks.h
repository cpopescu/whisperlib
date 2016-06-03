// Copyright: Urban Engines, Inc. 2011 onwards
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l nor the names of its
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

#ifndef _WHISPERLIB_BASE_LOG_CHECKS_H
#define _WHISPERLIB_BASE_LOG_CHECKS_H

#undef CHECK
#undef CHECK_EQ
#undef CHECK_LT
#undef CHECK_LE
#undef CHECK_GT
#undef CHECK_GE
#undef CHECK_NE

#undef CHECK_STREQ
#undef CHECK_STRLT
#undef CHECK_STRGT

#undef DCHECK
#undef DCHECK_EQ
#undef DCHECK_LT
#undef DCHECK_LE
#undef DCHECK_GT
#undef DCHECK_GE
#undef DCHECK_NE

//////////////////////////////////////////////////////////////////////
// Conditional macros - a little better then ASSERTs

#define CHECK(cond)  if (!LIKELY_T(cond)) LOG_FATAL << #cond << " FAILED! "

#define DEFINE_CHECK_OP(name, op)                                       \
    template <class T1, class T2>                                       \
    inline bool name(const T1& a, const T2& b) {                        \
        if (LIKELY_T( a op b )) return true;                            \
        return false;                                                   \
    }

#define DEFINE_CHECK_FUN(name, fun, res)                                \
    template <class T1, class T2>                                       \
    inline bool name(const T1& a, const T2& b) {                        \
        if (LIKELY_T( fun(a, b) == res )) return true;                  \
        return false;                                                   \
    }

DEFINE_CHECK_OP(CHECK_EQ_FUN, ==)
DEFINE_CHECK_OP(CHECK_LT_FUN, <)
DEFINE_CHECK_OP(CHECK_LE_FUN, <=)
DEFINE_CHECK_OP(CHECK_GT_FUN, >)
DEFINE_CHECK_OP(CHECK_GE_FUN, >=)
DEFINE_CHECK_OP(CHECK_NE_FUN, !=)

#define PERFORM_CHECK_OP(name, op, a, b)                                \
    if (!LIKELY_T(name((a), (b))))                                      \
        LOG_FATAL << "Check failed: [" << (a) << "] " #op " [" << (b) << "]"
#define CHECK_EQ(a,b) PERFORM_CHECK_OP(CHECK_EQ_FUN, ==, (a),(b))
#define CHECK_LT(a,b) PERFORM_CHECK_OP(CHECK_LT_FUN, <, (a),(b))
#define CHECK_LE(a,b) PERFORM_CHECK_OP(CHECK_LE_FUN, <=, (a),(b))
#define CHECK_GT(a,b) PERFORM_CHECK_OP(CHECK_GT_FUN, >, (a),(b))
#define CHECK_GE(a,b) PERFORM_CHECK_OP(CHECK_GE_FUN, >=, (a),(b))
#define CHECK_NE(a,b) PERFORM_CHECK_OP(CHECK_NE_FUN, !=, (a),(b))

DEFINE_CHECK_FUN(CHECK_STREQ_FUN, strcmp, 0)
DEFINE_CHECK_FUN(CHECK_STRLT_FUN, strcmp, -1)
DEFINE_CHECK_FUN(CHECK_STRGT_FUN, strcmp, 1)

#define PERFORM_CHECK_FUN(name, a, b)                                   \
    if (!LIKELY_T(name((a), (b))))                                      \
        LOG_FATAL << "Check failed: " << #name << "(" << (a) << ", " << (b) << ")"
#define CHECK_STREQ(a,b) PERFORM_CHECK_FUN(CHECK_STREQ_FUN, (a), (b))
#define CHECK_STRLT(a,b) PERFORM_CHECK_FUN(CHECK_STRLT_FUN, (a), (b))
#define CHECK_STRGT(a,b) PERFORM_CHECK_FUN(CHECK_STRGT_FUN, (a), (b))


#if !defined(DCHECK_IS_FATAL)
#define DCHECK(cond)    if (false) LOG_INFO
#define DCHECK_EQ(a, b) if (false) LOG_INFO
#define DCHECK_LT(a, b) if (false) LOG_INFO
#define DCHECK_LE(a, b) if (false) LOG_INFO
#define DCHECK_GT(a, b) if (false) LOG_INFO
#define DCHECK_GE(a, b) if (false) LOG_INFO
#define DCHECK_NE(a, b) if (false) LOG_INFO
#else
#define DCHECK(cond)    CHECK(cond)
#define DCHECK_EQ(a, b) CHECK_EQ(a, b)
#define DCHECK_LT(a, b) CHECK_LT(a, b)
#define DCHECK_LE(a, b) CHECK_LE(a, b)
#define DCHECK_GT(a, b) CHECK_GT(a, b)
#define DCHECK_GE(a, b) CHECK_GE(a, b)
#define DCHECK_NE(a, b) CHECK_NE(a, b)
#endif


#endif   // _WHISPERLIB_BASE_LOG_CHECKS_H
