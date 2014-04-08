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

# Sets `variable` to the namespace used by the gflags library
#
# AX_GFLAGS_NAMESPACE(variable)
AC_DEFUN([AX_GFLAGS_NAMESPACE], [
  AC_MSG_CHECKING([for gflags namespace])
  AC_LANG_PUSH([C++])
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([[
#include <gflags/gflags.h>
int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
}
/* override the definition of main, since we do not want to interfere with
   ours */
#define main test
]],
      [[]])],
      [AS_VAR_SET([$1],gflags)
       found=yes
       ],
       [found=no]
    )

  if test "$found" = no; then
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([[
#include <gflags/gflags.h>
int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
}
/* override the definition of main, since we do not want to interfere with
   ours */
#define main test
]],
        [[]])],
        [AS_VAR_SET([$1],google)
         found=yes
         ],
         [found=no]
    )
  fi
  AC_LANG_POP([C++])
  AC_MSG_RESULT($[$1])
])
