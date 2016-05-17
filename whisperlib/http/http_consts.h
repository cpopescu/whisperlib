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
// Author: Catalin Popescu
//
// Various HTTP specific constants


#ifndef __NET_HTTP_HTTP_CONSTS_H__
#define __NET_HTTP_HTTP_CONSTS_H__

#include "whisperlib/base/types.h"

namespace whisper {
namespace http {

//////////////////////////////////////////////////////////////////////
//
// HTTP Return codes
//

enum HttpReturnCode {
  UNKNOWN                          = 0,
  // Continuation codes
  CONTINUE                         = 100,
  SWITCHING_PROTOCOLS              = 101,

  // OK codes
  OK                               = 200,
  CREATED                          = 201,
  ACCEPTED                         = 202,
  NON_AUTHORITATIVE_INFORMATION    = 203,
  NO_CONTENT                       = 204,
  RESET_CONTENT                    = 205,
  PARTIAL_CONTENT                  = 206,

  // Redirections / choices
  MULTIPLE_CHOICES                 = 300,
  MOVED_PERMANENTLY                = 301,
  FOUND                            = 302,
  SEE_OTHER                        = 303,
  NOT_MODIFIED                     = 304,
  USE_PROXY                        = 305,
  TEMPORARY_REDIRECT               = 307,

  // Client error codes
  BAD_REQUEST                      = 400,
  UNAUTHORIZED                     = 401,
  PAYMENT_REQUIRED                 = 402,
  FORBIDDEN                        = 403,
  NOT_FOUND                        = 404,
  METHOD_NOT_ALLOWED               = 405,
  NOT_ACCEPTABLE                   = 406,
  PROXY_AUTHENTICATION_REQUIRED    = 407,
  REQUEST_TIME_OUT                 = 408,
  CONFLICT                         = 409,
  GONE                             = 410,
  LENGTH_REQUIRED                  = 411,
  PRECONDITION_FAILED              = 412,
  REQUEST_ENTITY_TOO_LARGE         = 413,
  REQUEST_URI_TOO_LARGE            = 414,
  UNSUPPORTED_MEDIA_TYPE           = 415,
  REQUESTED_RANGE_NOT_SATISFIABLE  = 416,
  EXPECTATION_FAILED               = 417,

  // Server error codes
  INTERNAL_SERVER_ERROR            = 500,
  NOT_IMPLEMENTED                  = 501,
  BAD_GATEWAY                      = 502,
  SERVICE_UNAVAILABLE              = 503,
  GATEWAY_TIME_OUT                 = 504,
  HTTP_VERSION_NOT_SUPPORTED       = 505,
};
const char* GetHttpReturnCodeDescription(HttpReturnCode code);
const char* GetHttpCodeDescription(int32 code);
const char* GetHttpReturnCodeName(HttpReturnCode code);

//////////////////////////////////////////////////////////////////////
//
// Various standard HTTP header names
//


// General headers
static const char kHeaderCacheControl[] = "Cache-Control";
static const char kHeaderConnection[] = "Connection";
static const char kHeaderDate[] = "Date";
static const char kHeaderPragma[] = "Pragma";
static const char kHeaderTrailer[] = "Trailer";
static const char kHeaderTransferEncoding[] = "Transfer-Encoding";
static const char kHeaderUpgrade[] = "Upgrade";
static const char kHeaderVia[] = "Via";
static const char kHeaderWarning[] = "Warning";

// Request headers
static const char kHeaderAccept[] = "Accept";
static const char kHeaderAcceptCharset[] = "Accept-Charset";
static const char kHeaderAcceptEncoding[] = "Accept-Encoding";
static const char kHeaderAcceptLanguage[] = "Accept-Language";
static const char kHeaderAuthorization[] = "Authorization";
static const char kHeaderExpect[] = "Expect";
static const char kHeaderFrom[] = "From";
static const char kHeaderHost[] = "Host";
static const char kHeaderIfMatch[] = "If-Match";
static const char kHeaderIfModifiedSince[] = "If-Modified-Since";
static const char kHeaderIfNoneMatch[] = "If-None-Match";
static const char kHeaderIfRange[] = "If-Range";
static const char kHeaderIfUnmodifiedSince[] = "If-Unmodified-Since";
static const char kHeaderMaxForwards[] = "Max-Forwards";
static const char kHeaderProxyAuthorization[] = "Proxy-Authorization";
static const char kHeaderRange[] = "Range";
static const char kHeaderReferer[] = "Referer";
static const char kHeaderTE[] = "TE";
static const char kHeaderUserAgent[] = "User-Agent";

// Response headers
static const char kHeaderAcceptRanges[] = "Accept-Ranges";
static const char kHeaderAge[] = "Age";
static const char kHeaderETag[] = "ETag";
static const char kHeaderKeepAlive[] = "Keep-Alive";
static const char kHeaderProxyAuthenticate[] = "Proxy-Authenticate";
static const char kHeaderRetryAfter[] = "Retry-After";
static const char kHeaderServer[] = "Server";
static const char kHeaderVary[] = "Vary";
static const char kHeaderWWWAuthenticate[] = "WWW-Authenticate";

// Entity header
static const char kHeaderAllow[] = "Allow";
static const char kHeaderContentEncoding[] = "Content-Encoding";
static const char kHeaderContentLanguage[] = "Content-Language";
static const char kHeaderContentLength[] = "Content-Length";
static const char kHeaderContentLocation[] = "Content-Location";
static const char kHeaderContentMD5[] = "Content-MD5";
static const char kHeaderContentRange[] = "Content-Range";
static const char kHeaderContentType[] = "Content-Type";
static const char kHeaderExpires[] = "Expires";
static const char kHeaderLastModified[] = "Last-Modified";

// Non standard
static const char kHeaderLocation[] = "Location";

// Cookie headers (non standard)
static const char kHeaderCookie[] = "Cookie";
static const char kHeaderSetCookie[] = "Set-Cookie";

// Nonstandard - whispercast use internal
static const char kHeaderXRequestId[] = "X-Request-Id";

//////////////////////////////////////////////////////////////////////
//
// HTTP request methods
//
static const char kMethodOptions[]       = "OPTIONS";
static const char kMethodGet[]           = "GET";
static const char kMethodHead[]          = "HEAD";
static const char kMethodPost[]          = "POST";
static const char kMethodPut[]           = "PUT";
static const char kMethodDelete[]        = "DELETE";
static const char kMethodTrace[]         = "TRACE";
static const char kMethodConnect[]       = "CONNECT";

enum HttpMethod {
  METHOD_UNKNOWN,
  METHOD_OPTIONS,
  METHOD_GET,
  METHOD_HEAD,
  METHOD_POST,
  METHOD_PUT,
  METHOD_DELETE,
  METHOD_TRACE,
  METHOD_CONNECT,
};
HttpMethod GetHttpMethod(const char* methoda);
const char* GetHttpMethodName(HttpMethod method);


//////////////////////////////////////////////////////////////////////
//
// HTTP versions
//
static const char kHttpVersion0_9[]       = "HTTP/0.9";
static const char kHttpVersion1_0[]       = "HTTP/1.0";
static const char kHttpVersion1_1[]       = "HTTP/1.1";
enum HttpVersion {
  VERSION_UNKNOWN = 0,
  VERSION_0_9     = 1,
  VERSION_1_0     = 2,
  VERSION_1_1     = 3,
};
HttpVersion GetHttpVersion(const char* ver);
const char* GetHttpVersionName(HttpVersion ver);

//////////////////////////////////////////////////////////////////////
//
// NOTE: For the next piece of code (pulled from google-url library:)
//
// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
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
//
// Character type discrimination functions
//
//
//   CTL            = <any US-ASCII control character
//                    (octets 0 - 31) and DEL (127)>
//   CHAR           = <any US-ASCII character (octets 0 - 127)>
//   CR             = <US-ASCII CR, carriage return (13)>
//   LF             = <US-ASCII LF, linefeed (10)>
//   SP             = <US-ASCII SP, space (32)>
//   HT             = <US-ASCII HT, horizontal-tab (9)>
//
//   CRLF           = CR LF
//   LWS            = [CRLF] 1*( SP | HT )
//
//   token          = 1*<any CHAR except CTLs or separators>
//   separators     = "(" | ")" | "<" | ">" | "@"
//                    | "," | ";" | ":" | "\" | <">
//                    | "/" | "[" | "]" | "?" | "="
//                    | "{" | "}" | SP | HT

// Bits that identify different character types. These types identify different
// bits that are set for each 8-bit character in the kSharedCharTypeTable.
enum SharedCharTypes {
  // Characters can be HTTP separators
  CHAR_SEPARATOR = 1,

  // Characters can be in a LWF
  CHAR_LWF = 2,

  // Valid in an ASCII-representation of a hex digit (as in %-escaped).
  CHAR_HEX = 4,

  // Valid in an ASCII-representation of a decimal digit.
  CHAR_DEC = 8,

  // Valid in an ASCII-representation of an octal digit.
  CHAR_OCT = 16,
};
extern const unsigned char kSharedCharTypeTable[0x100];

// More readable wrappers around the character type lookup table.
inline bool IsCharOfType(unsigned char c, SharedCharTypes type) {
  return !!(kSharedCharTypeTable[c] & type);
}

inline bool IsCtlChar(unsigned char c) {
  return c <= 31 || c == 127;
}
inline bool IsAsciiChar(unsigned char c) {
  return c <= 127;
}
inline bool IsWhiteSpace(unsigned char c) {
  return c == 32 || c == 9;
}
inline bool IsHexChar(unsigned char c) {
  return IsCharOfType(c, CHAR_HEX);
}
inline bool IsSeparatorChar(unsigned char c) {
  return IsCharOfType(c, CHAR_SEPARATOR);
}
inline bool IsLwfChar(unsigned char c) {
  return IsCharOfType(c, CHAR_LWF);
}
inline bool IsTokenChar(unsigned char c) {
  return IsAsciiChar(c) && !IsCtlChar(c) && !IsSeparatorChar(c);
}
//
// END NOTE (google-url library)
//
//////////////////////////////////////////////////////////////////////
}  // namespace http
}  // namespace whisper

#endif  // __NET_HTTP_HTTP_CONSTS_H__
