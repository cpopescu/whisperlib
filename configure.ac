AC_PREREQ([2.69])
AC_INIT([whisperlib], [1.0])
AM_INIT_AUTOMAKE([foreign color-tests dist-bzip2 parallel-tests subdir-objects])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_CXX

AC_PROG_YACC
AC_PROG_LEX
AC_PROG_RANLIB

AC_LANG(C++)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Require the GNU stl
AX_CXX_COMPILE_STDCXX_11([ext],[mandatory])

CXX_FLAG_CHECK([-stdlib=libc++])

# Add check for ICU. Let the user disable the use of ICU if it's
# present on the system by passing --without-icu to configure.
AC_ARG_WITH([icu],
   AS_HELP_STRING(
        [--without-icu],
        [Ignore the presence of ICU and disable it]))

AS_IF([test "x$with_icu" != "xno"],
  [AX_CHECK_ICU([51],
   [AC_DEFINE(HAVE_ICU)
    have_icu=yes
   ])],
  [have_icu=no])

AM_CONDITIONAL([HAVE_ICU], [test "x$have_icu" = "xyes"])

AS_IF([test "x$have_icu" = "xyes"],
  [],
  [AS_IF([test "x$with_icu" = "xyes"],
    [AC_MSG_ERROR([ICU library requested but not found])])
    ])

AC_C_BIGENDIAN

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/param.h sys/socket.h sys/stat.h sys/time.h unistd.h gflags/gflags.h glog/logging.h nameser8_compat.h endian.h sys/epoll.h sys/poll.h poll.h])

AC_CHECK_HEADERS([unordered_set tr1/unordered_set ext/hash_set unordered_map tr1/unordered_map ext/hash_map functional_hash.h tr1/functional_hash.h ext/hash_fun.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_STRUCT_TIMEZONE
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([atexit bzero clock_gettime ftruncate getcwd gethostbyname gettimeofday localeconv localtime_r memmove memset mkdir regcomp rmdir setlocale socket strcasecmp strchr strdup strerror strncasecmp strpbrk strrchr strtol strtoul strtoull])

AC_OUTPUT

AC_OUTPUT
