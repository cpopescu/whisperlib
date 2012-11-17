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
// Various HTTP specific constants

#include <ctype.h>
#include <strings.h>
#include "whisperlib/http/http_consts.h"

namespace http {

const char* GetHttpReturnCodeName(HttpReturnCode code) {
  switch ( code ) {
    CONSIDER(UNKNOWN);
    CONSIDER(CONTINUE);
    CONSIDER(SWITCHING_PROTOCOLS);
    CONSIDER(OK);
    CONSIDER(CREATED);
    CONSIDER(ACCEPTED);
    CONSIDER(NON_AUTHORITATIVE_INFORMATION);
    CONSIDER(NO_CONTENT);
    CONSIDER(RESET_CONTENT);
    CONSIDER(PARTIAL_CONTENT);
    CONSIDER(MULTIPLE_CHOICES);
    CONSIDER(MOVED_PERMANENTLY);
    CONSIDER(FOUND);
    CONSIDER(SEE_OTHER);
    CONSIDER(NOT_MODIFIED);
    CONSIDER(USE_PROXY);
    CONSIDER(TEMPORARY_REDIRECT);
    CONSIDER(BAD_REQUEST);
    CONSIDER(UNAUTHORIZED);
    CONSIDER(PAYMENT_REQUIRED);
    CONSIDER(FORBIDDEN);
    CONSIDER(NOT_FOUND);
    CONSIDER(METHOD_NOT_ALLOWED);
    CONSIDER(NOT_ACCEPTABLE);
    CONSIDER(PROXY_AUTHENTICATION_REQUIRED);
    CONSIDER(REQUEST_TIME_OUT);
    CONSIDER(CONFLICT);
    CONSIDER(GONE);
    CONSIDER(LENGTH_REQUIRED);
    CONSIDER(PRECONDITION_FAILED);
    CONSIDER(REQUEST_ENTITY_TOO_LARGE);
    CONSIDER(REQUEST_URI_TOO_LARGE);
    CONSIDER(UNSUPPORTED_MEDIA_TYPE);
    CONSIDER(REQUESTED_RANGE_NOT_SATISFIABLE);
    CONSIDER(EXPECTATION_FAILED);
    CONSIDER(INTERNAL_SERVER_ERROR);
    CONSIDER(NOT_IMPLEMENTED);
    CONSIDER(BAD_GATEWAY);
    CONSIDER(SERVICE_UNAVAILABLE);
    CONSIDER(GATEWAY_TIME_OUT);
    CONSIDER(HTTP_VERSION_NOT_SUPPORTED);
  }
  return "UNKNOWN";
}

const char* GetHttpCodeDescription(int32 code) {
  switch ( code ) {
  case 100: return "Continue";
  case 101: return "Switching Protocols";
  case 200: return "OK";
  case 201: return "Created";
  case 202: return "Accepted";
  case 203: return "Non-Authoritative Information";
  case 204: return "No Content";
  case 205: return "Reset Content";
  case 206: return "Partial Content";
  case 300: return "Multiple Choices";
  case 301: return "Moved Permanently";
  case 302: return "Found";
  case 303: return "See Other";
  case 304: return "Not Modified";
  case 305: return "Use Proxy";
  case 307: return "Temporary Redirect";
  case 400: return "Bad Request";
  case 401: return "Unauthorized";
  case 402: return "Payment Required";
  case 403: return "Forbidden";
  case 404: return "Not Found";
  case 405: return "Method Not Allowed";
  case 406: return "Not Acceptable";
  case 407: return "Proxy Authentication Required";
  case 408: return "Request Time-out";
  case 409: return "Conflict";
  case 410: return "Gone";
  case 411: return "Length Required";
  case 412: return "Precondition Failed";
  case 413: return "Request Entity Too Large";
  case 414: return "Request-URI Too Large";
  case 415: return "Unsupported Media Type";
  case 416: return "Requested range not satisfiable";
  case 417: return "Expectation Failed";
  case 500: return "Internal Server Error";
  case 501: return "Not Implemented";
  case 502: return "Bad Gateway";
  case 503: return "Service Unavailable";
  case 504: return "Gateway Time-out";
  case 505: return "HTTP Version not supported";
  }
  return "Unknown";
}

const char* GetHttpReturnCodeDescription(HttpReturnCode code) {
  return GetHttpCodeDescription(static_cast<int32>(code));
}


HttpVersion GetHttpVersion(const char* ver) {
  if ( !strcasecmp(ver, kHttpVersion1_1) ) return VERSION_1_1;
  if ( !strcasecmp(ver, kHttpVersion1_0) ) return VERSION_1_0;
  if ( !strcasecmp(ver, kHttpVersion0_9) ) return VERSION_0_9;
  return VERSION_UNKNOWN;
}

const char* GetHttpVersionName(HttpVersion ver) {
  switch ( ver ) {
  case VERSION_0_9: return kHttpVersion0_9;
  case VERSION_1_0: return kHttpVersion1_0;
  case VERSION_1_1: return kHttpVersion1_1;
  case VERSION_UNKNOWN: break;
  }
  return "HTTP/unknown";
}

HttpMethod GetHttpMethod(const char* method) {
  switch ( toupper(*method) ) {
  case 'O':
    if ( !strcasecmp(method + 1, "PTIONS") ) return METHOD_OPTIONS;
    break;
  case 'G':
    if ( !strcasecmp(method + 1, "ET") )     return METHOD_GET;
    break;
  case 'H':
    if ( !strcasecmp(method + 1, "EAD") )    return METHOD_HEAD;
    break;
  case 'P':
    if ( !strcasecmp(method + 1, "OST") )    return METHOD_POST;
    if ( !strcasecmp(method + 1, "UT") )     return METHOD_PUT;
    break;
  case 'D':
    if ( !strcasecmp(method + 1, "ELETE") )  return METHOD_DELETE;
    break;
  case 'T':
    if ( !strcasecmp(method + 1, "RACE") )   return METHOD_TRACE;
    break;
  case 'C':
    if ( !strcasecmp(method + 1, "ONNECT") ) return METHOD_CONNECT;
    break;
  }
  return METHOD_UNKNOWN;
}
const char* GetHttpMethodName(HttpMethod method) {
  switch ( method ) {
  case METHOD_UNKNOWN: return "UNKNOWN";
  case METHOD_OPTIONS: return "OPTIONS";
  case METHOD_GET: return "GET";
  case METHOD_HEAD: return "HEAD";
  case METHOD_POST: return "POST";
  case METHOD_PUT: return "PUT";
  case METHOD_DELETE: return "DELETE";
  case METHOD_TRACE: return "TRACE";
  case METHOD_CONNECT: return "CONNECT";
  }
  return "UNKNOWN";
}


// See the header file for this array's declaration.
const unsigned char kSharedCharTypeTable[0x100] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,       // 0x00 - 0x08
    CHAR_LWF | CHAR_SEPARATOR,       // 0x08 \t
    CHAR_LWF,                        // 0x0a \n
    0,                               // 0x0b
    0,                               // 0x0c
    CHAR_LWF,                        // 0x0d \r
    0,                               // 0x0e
    0,                               // 0x0f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x10 - 0x1f
    CHAR_LWF | CHAR_SEPARATOR,       // 0x20  ' '
    0,                               // 0x21  !
    CHAR_SEPARATOR,                  // 0x22  "
    0,                               // 0x23  #
    0,                               // 0x24  $
    0,                               // 0x25  %
    0,                               // 0x26  &
    0,                               // 0x27  '
    CHAR_SEPARATOR,                  // 0x28  (
    CHAR_SEPARATOR,                  // 0x29  )
    0,                               // 0x2a  *
    0,                               // 0x2b  +
    CHAR_SEPARATOR,                  // 0x2c  ,
    0,                               // 0x2d  -
    0,                               // 0x2e  .
    CHAR_SEPARATOR,                  // 0x2f  /
    CHAR_HEX | CHAR_DEC | CHAR_OCT,  // 0x30  0
    CHAR_HEX | CHAR_DEC | CHAR_OCT,  // 0x31  1
    CHAR_HEX | CHAR_DEC | CHAR_OCT,  // 0x32  2
    CHAR_HEX | CHAR_DEC | CHAR_OCT,  // 0x33  3
    CHAR_HEX | CHAR_DEC | CHAR_OCT,  // 0x34  4
    CHAR_HEX | CHAR_DEC | CHAR_OCT,  // 0x35  5
    CHAR_HEX | CHAR_DEC | CHAR_OCT,  // 0x36  6
    CHAR_HEX | CHAR_DEC | CHAR_OCT,  // 0x37  7
    CHAR_HEX | CHAR_DEC,             // 0x38  8
    CHAR_HEX | CHAR_DEC,             // 0x39  9
    CHAR_SEPARATOR,                  // 0x3a  :
    CHAR_SEPARATOR,                  // 0x3b  ;
    CHAR_SEPARATOR,                  // 0x3c  <
    CHAR_SEPARATOR,                  // 0x3d  =
    CHAR_SEPARATOR,                  // 0x3e  >
    CHAR_SEPARATOR,                  // 0x3f  ?
    CHAR_SEPARATOR,                  // 0x40  @
    CHAR_HEX,                        // 0x41  A
    CHAR_HEX,                        // 0x42  B
    CHAR_HEX,                        // 0x43  C
    CHAR_HEX,                        // 0x44  D
    CHAR_HEX,                        // 0x45  E
    CHAR_HEX,                        // 0x46  F
    0,                               // 0x47  G
    0,                               // 0x48  H
    0,                               // 0x49  I
    0,                               // 0x4a  J
    0,                               // 0x4b  K
    0,                               // 0x4c  L
    0,                               // 0x4d  M
    0,                               // 0x4e  N
    0,                               // 0x4f  O
    0,                               // 0x50  P
    0,                               // 0x51  Q
    0,                               // 0x52  R
    0,                               // 0x53  S
    0,                               // 0x54  T
    0,                               // 0x55  U
    0,                               // 0x56  V
    0,                               // 0x57  W
    0,                               // 0x58  X
    0,                               // 0x59  Y
    0,                               // 0x5a  Z
    CHAR_SEPARATOR,  // 0x5b  [
    CHAR_SEPARATOR,  // 0x5c  '\'
    CHAR_SEPARATOR,  // 0x5d  ]
    0,                               // 0x5e  ^
    0,                               // 0x5f  _
    0,                               // 0x60  `
    CHAR_HEX,                        // 0x61  a
    CHAR_HEX,                        // 0x62  b
    CHAR_HEX,                        // 0x63  c
    CHAR_HEX,                        // 0x64  d
    CHAR_HEX,                        // 0x65  e
    CHAR_HEX,                        // 0x66  f
    0,                               // 0x67  g
    0,                               // 0x68  h
    0,                               // 0x69  i
    0,                               // 0x6a  j
    0,                               // 0x6b  k
    0,                               // 0x6c  l
    0,                               // 0x6d  m
    0,                               // 0x6e  n
    0,                               // 0x6f  o
    0,                               // 0x70  p
    0,                               // 0x71  q
    0,                               // 0x72  r
    0,                               // 0x73  s
    0,                               // 0x74  t
    0,                               // 0x75  u
    0,                               // 0x76  v
    0,                               // 0x77  w
    0,                               // 0x78  x
    0,                               // 0x79  y
    0,                               // 0x7a  z
    CHAR_SEPARATOR,                  // 0x7b  {
    CHAR_SEPARATOR,                  // 0x7c  |
    0,                               // 0x7d  }
    0,                               // 0x7e  ~
    0,                               // 0x7f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x80 - 0x8f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x90 - 0x9f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xa0 - 0xaf
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xb0 - 0xbf
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xc0 - 0xcf
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xd0 - 0xdf
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xe0 - 0xef
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xf0 - 0xff
};
}
