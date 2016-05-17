/* Section coming from acconfig.h */

#undef HAVE_GLOG
#undef HAVE_GFLAGS
#undef HAVE_PROTOBUF

#undef HAVE_UNORDERED_SET
#undef HAVE_TR1_UNORDERED_SET
#undef HAVE_EXT_HASH_SET

#undef HAVE_UNORDERED_MAP
#undef HAVE_TR1_UNORDERED_MAP
#undef HAVE_EXT_HASH_MAP

#undef HAVE_FUNCTIONAL_HASH_H
#undef HAVE_TR1_FUNCTIONAL_HASH_H
#undef HAVE_EXT_HASH_FUN_H


#if defined(__APPLE__) && defined(__MACH__)
	/* Apple OSX and iOS (Darwin). ------------------------------ */
# include <TargetConditionals.h>
# if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#  define IOS
# elif TARGET_OS_MAC == 1
#  define MACOSX
# endif
#endif


#undef WORDS_BIGENDIAN

// BYTE_ORDER, BIG_ENDIAN, and LITTLE_ENDIAN are defined on Mach in
// i386/endian.h, so make sure we don't redefine them here.
#ifndef LITTLE_ENDIAN
# define LITTLE_ENDIAN 0
#endif

#ifndef BIG_ENDIAN
# define BIG_ENDIAN 1
#endif

#ifdef WORDS_BIGENDIAN
# ifndef BYTE_ORDER
#  define BYTE_ORDER BIG_ENDIAN
# endif
#else
# ifndef BYTE_ORDER
#  define BYTE_ORDER LITTLE_ENDIAN
# endif
#endif

#undef HAVE_SYS_EPOLL_H
#undef HAVE_SYS_POLL_H
#undef HAVE_POLL_H

#undef HAVE_SYS_STAT_H

#undef HAVE_ICU

#if HAVE_GLOG
# define USE_GLOG_LOGGING 1
#endif

#if HAVE_GFLAGS
# define USE_GFLAGS 1
#endif

#ifdef HAVE_ICU
# define USE_ICU 1
#endif

/* End of section coming from acconfig.h */
