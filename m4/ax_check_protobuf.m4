# Author: Ovidiu Predescu
# Date: April 8, 2014
#
# Check if the Google protobuf library is installed and usable.
#
# AX_CHECK_PROTOBUF(action-if, action-if-not)
#
AC_DEFUN([AX_CHECK_PROTOBUF], [
  ax_save_LIBS="$LIBS"
  LIBS="-lprotobuf"

  AC_LANG_PUSH([C++])
  AC_MSG_CHECKING([for protobuf library])
AC_TRY_LINK([
#include <google/protobuf/compiler/parser.h>
google::protobuf::compiler::Parser parser;
         ],
  [],
  [have_protobuf=yes
   ],
  [have_protobuf=no])
  AC_MSG_RESULT($have_protobuf)
  AC_LANG_POP([C++])

  if test "$have_protobuf" = "no"; then
    LIBS=$ax_save_LIBS
    ifelse([$2], , :, [$2])
  else
    LIBS="-lprotobuf $ax_save_LIBS"
    ifelse([$1], , :, [$1])
  fi
])
