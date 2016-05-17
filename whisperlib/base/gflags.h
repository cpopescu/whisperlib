//
// Stub for including the gflags header or some dumb - global variable defines.
//

#ifndef __WHISPERLIB_BASE_GFLAGS_H
#define __WHISPERLIB_BASE_GFLAGS_H

#include <whisperlib/base/core_config.h>

#ifndef GFLAGS_NAMESPACE
#define GFLAGS_NAMESPACE google
#endif

#if defined(USE_GFLAGS)
// We have real flags
#include <gflags/gflags.h>

#else  // ! USE_GFLAGS

#if defined(USE_GLOG_LOGGING) && defined(HAVE_GLOG)
#error "If you enable glog, you cannot disable gflags."
#endif

#include <string>
// Flags definitions - no flags - just global variable placeholders

#define DEFINE_VARIABLE(type, shorttype, name, value)                   \
namespace fL##shorttype {                                               \
    static const type FLAGSX_##name = value;                            \
    type FLAGS_##name = FLAGSX_##name;                                  \
}                                                                       \
using fL##shorttype::FLAGS_##name;                                      \

#define DEFINE_bool(name, val, txt)             \
    DEFINE_VARIABLE(bool, B, name, val)

#define DEFINE_int32(name, val, txt)            \
    DEFINE_VARIABLE(int32_t, I, name, val)

#define DEFINE_int64(name, val, txt)            \
    DEFINE_VARIABLE(int64_t, I64, name, val)

#define DEFINE_uint64(name, val, txt)           \
    DEFINE_VARIABLE(uint64_t, U64, name, val)

#define DEFINE_double(name, val, txt)           \
    DEFINE_VARIABLE(double, D, name, val)

#define DEFINE_string(name, val, txt)                                   \
namespace fLS {                                                         \
    static std::string* const FLAGSX_##name =                           \
        val ? new std::string(val) : new std::string("");               \
    std::string& FLAGS_##name = *FLAGSX_##name;                         \
}                                                                       \
using fLS::FLAGS_##name                                                 \

// Flags declarations - for using.

#define DECLARE_VARIABLE(type, shorttype, name)         \
    namespace fL##shorttype { extern type FLAGS_##name; }   \
    using fL##shorttype::FLAGS_##name

#define DECLARE_bool(name)                      \
    DECLARE_VARIABLE(bool, B, name)

#define DECLARE_int32(name)                     \
    DECLARE_VARIABLE(int32, I, name)

#define DECLARE_int64(name)                     \
    DECLARE_VARIABLE(int64, I64, name)

#define DECLARE_uint64(name)                    \
    DECLARE_VARIABLE(uint64, U64, name)

#define DECLARE_double(name)                    \
    DECLARE_VARIABLE(double, D, name)

#define DECLARE_string(name)                    \
namespace fLS {                                 \
    extern string& FLAGS_##name;       \
}                                               \
using fLS::FLAGS_##name


#define ParseCommandLineFlags(argc, argv, t)

#endif  // USE_GFLAGS
#endif  // __WHISPERLIB_BASE_GFLAGS_H
