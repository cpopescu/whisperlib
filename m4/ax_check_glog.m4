# Author: Ovidiu Predescu
# Date: April 8, 2014
#
# Check if Google's glog library is installed and usable.
#
# AX_CHECK_GLOG(action-if, action-if-not)
#
AC_DEFUN([AX_CHECK_GLOG], [
  ax_save_LIBS="$LIBS"
  LIBS="-lglog"

  AC_MSG_CHECKING([for usable glog library])
  AC_LANG_PUSH([C++])
AC_TRY_LINK([
#include <glog/logging.h>
         ],
  [int argc;
  char ** argv;
  google::InitGoogleLogging(argv[0]);
  ],
  [have_glog=yes
   ],
  [have_glog=no])
  AC_MSG_RESULT($have_glog)
  AC_LANG_POP([C++])

  if test "$have_glog" = "no"; then
    LIBS=$ax_save_LIBS
    ifelse([$2], , :, [$2])
  else
    LIBS="-lglog $ax_save_LIBS"
    ifelse([$1], , :, [$1])
  fi
])
