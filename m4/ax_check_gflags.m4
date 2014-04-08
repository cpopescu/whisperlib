# Author: Ovidiu Predescu
# Date: April 8, 2014
#
# Check if Google's gflags library is installed and usable.
#
# AX_CHECK_GFLAGS(action-if, action-if-not)
#
AC_DEFUN([AX_CHECK_GFLAGS], [
  ax_save_LIBS="$LIBS"
  LIBS="-lgflags"

  AC_LANG_PUSH([C++])
  AC_MSG_CHECKING([for usable gflags library])
AC_TRY_LINK([
#include <gflags/gflags.h>
DEFINE_bool(test, false, "help string");
         ],
  [int argc;
  char ** argv;
  if (FLAGS_test) {
  }
  ],
  [have_gflags=yes
   ],
  [have_gflags=no])
  AC_MSG_RESULT($have_gflags)
  AC_LANG_POP([C++])

  if test "$have_gflags" = "no"; then
    LIBS=$ax_save_LIBS
    ifelse([$2], , :, [$2])
  else
    LIBS="-lgflags $ax_save_LIBS"
    ifelse([$1], , :, [$1])
  fi
])
