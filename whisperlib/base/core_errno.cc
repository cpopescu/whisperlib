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

#include <string.h>
#include <errno.h>

#include "whisperlib/base/core_errno.h"

#define ESUCCESS 0
// One annoying feature of the POSIX 1003.1 specification is the lack of a
// no error value. When errno is set to 0, you've encountered no problems,
// except you can't refer to this with a standard symbolic constant.

#define STRINGISIZE(x) #x
#define ERROR_NO(x) "[" #x " " STRINGISIZE(x) "] "

const char* GetSystemErrorDescription(int sys_err) {
  switch ( sys_err ) {
#ifdef ESUCCESS
  case ESUCCESS:
    return ERROR_NO(ESUCCESS) "No error";
#endif
#ifdef EPERM
  case EPERM:
    return ERROR_NO(EPERM) "Operation not permitted";
#endif
#ifdef ENOENT
  case ENOENT:
    return ERROR_NO(ENOENT) "No such file or directory";
#endif
#ifdef ESRCH
  case ESRCH:
    return ERROR_NO(ESRCH) "No such process";
#endif
#ifdef EINTR
  case EINTR:
    return ERROR_NO(EINTR) "Interrupted system call";
#endif
#ifdef EIO
  case EIO:
    return ERROR_NO(EIO) "I/O error";
#endif
#ifdef ENXIO
  case ENXIO:
    return ERROR_NO(ENXIO) "No such device or address";
#endif
#ifdef E2BIG
  case E2BIG:
    return ERROR_NO(E2BIG) "Argument list too long";
#endif
#ifdef ENOEXEC
  case ENOEXEC:
    return ERROR_NO(ENOEXEC) "Exec format error";
#endif
#ifdef EBADF
  case EBADF:
    return ERROR_NO(EBADF) "Bad file number";
#endif
#ifdef ECHILD
  case ECHILD:
    return ERROR_NO(ECHILD) "No child processes";
#endif
#ifdef EAGAIN
  case EAGAIN:
    return ERROR_NO(EAGAIN) "Try again";
#endif
#ifdef ENOMEM
  case ENOMEM:
    return ERROR_NO(ENOMEM) "Out of memory";
#endif
#ifdef EACCES
  case EACCES:
    return ERROR_NO(EACCES) "Permission denied";
#endif
#ifdef EFAULT
  case EFAULT:
    return ERROR_NO(EFAULT) "Bad address";
#endif
#ifdef ENOTBLK
  case ENOTBLK:
    return ERROR_NO(ENOTBLK) "Block device required";
#endif
#ifdef EBUSY
  case EBUSY:
    return ERROR_NO(EBUSY) "Device or resource busy";
#endif
#ifdef EEXIST
  case EEXIST:
    return ERROR_NO(EEXIST) "File exists";
#endif
#ifdef EXDEV
  case EXDEV:
    return ERROR_NO(EXDEV) "Cross-device link";
#endif
#ifdef ENODEV
  case ENODEV:
    return ERROR_NO(ENODEV) "No such device";
#endif
#ifdef ENOTDIR
  case ENOTDIR:
    return ERROR_NO(ENOTDIR) "Not a directory";
#endif
#ifdef EISDIR
  case EISDIR:
    return ERROR_NO(EISDIR) "Is a directory";
#endif
#ifdef EINVAL
  case EINVAL:
    return ERROR_NO(EINVAL) "Invalid argument";
#endif
#ifdef ENFILE
  case ENFILE:
    return ERROR_NO(ENFILE) "File table overflow";
#endif
#ifdef EMFILE
  case EMFILE:
    return ERROR_NO(EMFILE) "Too many open files";
#endif
#ifdef ENOTTY
  case ENOTTY:
    return ERROR_NO(ENOTTY) "Not a typewriter";
#endif
#ifdef ETXTBSY
  case ETXTBSY:
    return ERROR_NO(ETXTBSY) "Text file busy";
#endif
#ifdef EFBIG
  case EFBIG:
    return ERROR_NO(EFBIG) "File too large";
#endif
#ifdef ENOSPC
  case ENOSPC:
    return ERROR_NO(ENOSPC) "No space left on device";
#endif
#ifdef ESPIPE
  case ESPIPE:
    return ERROR_NO(ESPIPE) "Illegal seek";
#endif
#ifdef EROFS
  case EROFS:
    return ERROR_NO(EROFS) "Read-only file system";
#endif
#ifdef EMLINK
  case EMLINK:
    return ERROR_NO(EMLINK) "Too many links";
#endif
#ifdef EPIPE
  case EPIPE:
    return ERROR_NO(EPIPE) "Broken pipe";
#endif
#ifdef EDOM
  case EDOM:
    return ERROR_NO(EDOM) "Math argument out of domain of func";
#endif
#ifdef ERANGE
  case ERANGE:
    return ERROR_NO(ERANGE) "Math result not representable";
#endif
#ifdef EDEADLK
  case EDEADLK:
    return ERROR_NO(EDEADLK) "Resource deadlock would occur";
#endif
#ifdef ENAMETOOLONG
  case ENAMETOOLONG:
    return ERROR_NO(ENAMETOOLONG) "File name too long";
#endif
#ifdef ENOLCK
  case ENOLCK:
    return ERROR_NO(ENOLCK) "No record locks available";
#endif
#ifdef ENOSYS
  case ENOSYS:
    return ERROR_NO(ENOSYS) "Function not implemented";
#endif
#ifdef ENOTEMPTY
  case ENOTEMPTY:
    return ERROR_NO(ENOTEMPTY) "Directory not empty";
#endif
#ifdef ELOOP
  case ELOOP:
    return ERROR_NO(ELOOP) "Too many symbolic links encountered";
#endif
  //  #define EWOULDBLOCK EAGAIN  // Operation would block
#ifdef ENOMSG
  case ENOMSG:
    return ERROR_NO(ENOMSG) "No message of desired type";
#endif
#ifdef EIDRM
  case EIDRM:
    return ERROR_NO(EIDRM) "Identifier removed";
#endif
#ifdef ECHRNG
  case ECHRNG:
    return ERROR_NO(ECHRNG) "Channel number out of range";
#endif
#ifdef EL2NSYNC
  case EL2NSYNC:
    return ERROR_NO(EL2NSYNC) "Level 2 not synchronized";
#endif
#ifdef EL3HLT
  case EL3HLT:
    return ERROR_NO(EL3HLT) "Level 3 halted";
#endif
#ifdef EL3RST
  case EL3RST:
    return ERROR_NO(EL3RST) "Level 3 reset";
#endif
#ifdef ELNRNG
  case ELNRNG:
    return ERROR_NO(ELNRNG) "Link number out of range";
#endif
#ifdef EUNATCH
  case EUNATCH:
    return ERROR_NO(EUNATCH) "Protocol driver not attached";
#endif
#ifdef ENOCSI
  case ENOCSI:
    return ERROR_NO(ENOCSI) "No CSI structure available";
#endif
#ifdef EL2HLT
  case EL2HLT:
    return ERROR_NO(EL2HLT) "Level 2 halted";
#endif
#ifdef EBADE
  case EBADE:
    return ERROR_NO(EBADE) "Invalid exchange";
#endif
#ifdef EBADR
  case EBADR:
    return ERROR_NO(EBADR) "Invalid request descriptor";
#endif
#ifdef EXFULL
  case EXFULL:
    return ERROR_NO(EXFULL) "Exchange full";
#endif
#ifdef ENOANO
  case ENOANO:
    return ERROR_NO(ENOANO) "No anode";
#endif
#ifdef EBADRQC
  case EBADRQC:
    return ERROR_NO(EBADRQC) "Invalid request code";
#endif
#ifdef EBADSLT
  case EBADSLT:
    return ERROR_NO(EBADSLT) "Invalid slot";
#endif
  //  #define EDEADLOCK EDEADLK
#ifdef EBFONT
  case EBFONT:
    return ERROR_NO(EBFONT) "Bad font file format";
#endif
#ifdef ENOSTR
  case ENOSTR:
    return ERROR_NO(ENOSTR) "Device not a stream";
#endif
#ifdef ENODATA
  case ENODATA:
    return ERROR_NO(ENODATA) "No data available";
#endif
#ifdef ETIME
  case ETIME:
    return ERROR_NO(ETIME) "Timer expired";
#endif
#ifdef ENOSR
  case ENOSR:
    return ERROR_NO(ENOSR) "Out of streams resources";
#endif
#ifdef ENONET
  case ENONET:
    return ERROR_NO(ENONET) "Machine is not on the network";
#endif
#ifdef ENOPKG
  case ENOPKG:
    return ERROR_NO(ENOPKG) "Package not installed";
#endif
#ifdef EREMOTE
  case EREMOTE:
    return ERROR_NO(EREMOTE) "Object is remote";
#endif
#ifdef ENOLINK
  case ENOLINK:
    return ERROR_NO(ENOLINK) "Link has been severed";
#endif
#ifdef EADV
  case EADV:
    return ERROR_NO(EADV) "Advertise error";
#endif
#ifdef ESRMNT
  case ESRMNT:
    return ERROR_NO(ESRMNT) "Srmount error";
#endif
#ifdef ECOMM
  case ECOMM:
    return ERROR_NO(ECOMM) "Communication error on send";
#endif
#ifdef EPROTO
  case EPROTO:
    return ERROR_NO(EPROTO) "Protocol error";
#endif
#ifdef EMULTIHOP
  case EMULTIHOP:
    return ERROR_NO(EMULTIHOP) "Multihop attempted";
#endif
#ifdef EDOTDOT
  case EDOTDOT:
    return ERROR_NO(EDOTDOT) "RFS specific error";
#endif
#ifdef EBADMSG
  case EBADMSG:
    return ERROR_NO(EBADMSG) "Not a data message";
#endif
#ifdef EOVERFLOW
  case EOVERFLOW:
    return ERROR_NO(EOVERFLOW) "Value too large for defined data type";
#endif
#ifdef ENOTUNIQ
  case ENOTUNIQ:
    return ERROR_NO(ENOTUNIQ) "Name not unique on network";
#endif
#ifdef EBADFD
  case EBADFD:
    return ERROR_NO(EBADFD) "File descriptor in bad state";
#endif
#ifdef EREMCHG
  case EREMCHG:
    return ERROR_NO(EREMCHG) "Remote address changed";
#endif
#ifdef ELIBACC
  case ELIBACC:
    return ERROR_NO(ELIBACC) "Can not access a needed shared library";
#endif
#ifdef ELIBBAD
  case ELIBBAD:
    return ERROR_NO(ELIBBAD) "Accessing a corrupted shared library";
#endif
#ifdef ELIBSCN
  case ELIBSCN:
    return ERROR_NO(ELIBSCN) ".lib section in a.out corrupted";
#endif
#ifdef ELIBMAX
  case ELIBMAX:
    return ERROR_NO(ELIBMAX) "Attempting to link in too many shared libraries";
#endif
#ifdef ELIBEXEC
  case ELIBEXEC:
    return ERROR_NO(ELIBEXEC) "Cannot exec a shared library directly";
#endif
#ifdef EILSEQ
  case EILSEQ:
    return ERROR_NO(EILSEQ) "Illegal byte sequence";
#endif
#ifdef ERESTART
  case ERESTART:
    return ERROR_NO(ERESTART) "Interrupted system call should be restarted";
#endif
#ifdef ESTRPIPE
  case ESTRPIPE:
    return ERROR_NO(ESTRPIPE) "Streams pipe error";
#endif
#ifdef EUSERS
  case EUSERS:
    return ERROR_NO(EUSERS) "Too many users";
#endif
#ifdef ENOTSOCK
  case ENOTSOCK:
    return ERROR_NO(ENOTSOCK) "Socket operation on non-socket";
#endif
#ifdef EDESTADDRREQ
  case EDESTADDRREQ:
    return ERROR_NO(EDESTADDRREQ) "Destination address required";
#endif
#ifdef EMSGSIZE
  case EMSGSIZE:
    return ERROR_NO(EMSGSIZE) "Message too long";
#endif
#ifdef EPROTOTYPE
  case EPROTOTYPE:
    return ERROR_NO(EPROTOTYPE) "Protocol wrong type for socket";
#endif
#ifdef ENOPROTOOPT
  case ENOPROTOOPT:
    return ERROR_NO(ENOPROTOOPT) "Protocol not available";
#endif
#ifdef EPROTONOSUPPORT
  case EPROTONOSUPPORT:
    return ERROR_NO(EPROTONOSUPPORT) "Protocol not supported";
#endif
#ifdef ESOCKTNOSUPPORT
  case ESOCKTNOSUPPORT:
    return ERROR_NO(ESOCKTNOSUPPORT) "Socket type not supported";
#endif
#ifdef EOPNOTSUPP
  case EOPNOTSUPP:
    return ERROR_NO(EOPNOTSUPP) "Operation not supported on transport endpoint";
#endif
#ifdef EPFNOSUPPORT
  case EPFNOSUPPORT:
    return ERROR_NO(EPFNOSUPPORT) "Protocol family not supported";
#endif
#ifdef EAFNOSUPPORT
  case EAFNOSUPPORT:
    return ERROR_NO(EAFNOSUPPORT) "Address family not supported by protocol";
#endif
#ifdef EADDRINUSE
  case EADDRINUSE:
    return ERROR_NO(EADDRINUSE) "Address already in use";
#endif
#ifdef EADDRNOTAVAIL
  case EADDRNOTAVAIL:
    return ERROR_NO(EADDRNOTAVAIL) "Cannot assign requested address";
#endif
#ifdef ENETDOWN
  case ENETDOWN:
    return ERROR_NO(ENETDOWN) "Network is down";
#endif
#ifdef ENETUNREACH
  case ENETUNREACH:
    return ERROR_NO(ENETUNREACH) "Network is unreachable";
#endif
#ifdef ENETRESET
  case ENETRESET:
    return ERROR_NO(ENETRESET) "Network dropped connection because of reset";
#endif
#ifdef ECONNABORTED
  case ECONNABORTED:
    return ERROR_NO(ECONNABORTED) "Software caused connection abort";
#endif
#ifdef ECONNRESET
  case ECONNRESET:
    return ERROR_NO(ECONNRESET) "Connection reset by peer";
#endif
#ifdef ENOBUFS
  case ENOBUFS:
    return ERROR_NO(ENOBUFS) "No buffer space available";
#endif
#ifdef EISCONN
  case EISCONN:
    return ERROR_NO(EISCONN) "Transport endpoint is already connected";
#endif
#ifdef ENOTCONN
  case ENOTCONN:
    return ERROR_NO(ENOTCONN) "Transport endpoint is not connected";
#endif
#ifdef ESHUTDOWN
  case ESHUTDOWN:
    return ERROR_NO(ESHUTDOWN) "Cannot send after transport endpoint shutdown";
#endif
#ifdef ETOOMANYREFS
  case ETOOMANYREFS:
    return ERROR_NO(ETOOMANYREFS) "Too many references: cannot splice";
#endif
#ifdef ETIMEDOUT
  case ETIMEDOUT:
    return ERROR_NO(ETIMEDOUT) "Connection timed out";
#endif
#ifdef ECONNREFUSED
  case ECONNREFUSED:
    return ERROR_NO(ECONNREFUSED) "Connection refused";
#endif
#ifdef EHOSTDOWN
  case EHOSTDOWN:
    return ERROR_NO(EHOSTDOWN) "Host is down";
#endif
#ifdef EHOSTUNREACH
  case EHOSTUNREACH:
    return ERROR_NO(EHOSTUNREACH) "No route to host";
#endif
#ifdef EALREADY
  case EALREADY:
    return ERROR_NO(EALREADY) "Operation already in progress";
#endif
#ifdef EINPROGRESS
  case EINPROGRESS:
    return ERROR_NO(EINPROGRESS) "Operation now in progress";
#endif
#ifdef ESTALE
  case ESTALE:
    return ERROR_NO(ESTALE) "Stale NFS file handle";
#endif
#ifdef EUCLEAN
  case EUCLEAN:
    return ERROR_NO(EUCLEAN) "Structure needs cleaning";
#endif
#ifdef ENOTNAM
  case ENOTNAM:
    return ERROR_NO(ENOTNAM) "Not a XENIX named type file";
#endif
#ifdef ENAVAIL
  case ENAVAIL:
    return ERROR_NO(ENAVAIL) "No XENIX semaphores available";
#endif
#ifdef EISNAM
  case EISNAM:
    return ERROR_NO(EISNAM) "Is a named type file";
#endif
#ifdef EREMOTEIO
  case EREMOTEIO:
    return ERROR_NO(EREMOTEIO) "Remote I/O error";
#endif
#ifdef EDQUOT
  case EDQUOT:
    return ERROR_NO(EDQUOT) "Quota exceeded";
#endif
#ifdef ENOMEDIUM
  case ENOMEDIUM:
    return ERROR_NO(ENOMEDIUM) "No medium found";
#endif
#ifdef EMEDIUMTYPE
  case EMEDIUMTYPE:
    return ERROR_NO(EMEDIUMTYPE) "Wrong medium type";
#endif
#ifdef ECANCELED
  case ECANCELED:
    return ERROR_NO(ECANCELED) "Operation Canceled";
#endif
#ifdef ENOKEY
  case ENOKEY:
    return ERROR_NO(ENOKEY) "Required key not available";
#endif
#ifdef EKEYEXPIRED
  case EKEYEXPIRED:
    return ERROR_NO(EKEYEXPIRED) "Key has expired";
#endif
#ifdef EKEYREVOKED
  case EKEYREVOKED:
    return ERROR_NO(EKEYREVOKED) "Key has been revoked";
#endif
#ifdef EKEYREJECTED
  case EKEYREJECTED:
    return ERROR_NO(EKEYREJECTED) "Key was rejected by service";
#endif
  // for robust mutexes
#ifdef EOWNERDEAD
  case EOWNERDEAD:
    return ERROR_NO(EOWNERDEAD) "Owner died";
#endif
#ifdef ENOTRECOVERABLE
  case ENOTRECOVERABLE:
    return ERROR_NO(ENOTRECOVERABLE) "State not recoverable";
#endif
}
  return "Unknown error";
}
