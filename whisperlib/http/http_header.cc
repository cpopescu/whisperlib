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

#include <time.h>
#include "whisperlib/http/http_header.h"
#include "whisperlib/io/buffer/memory_stream.h"
#include "whisperlib/base/core_errno.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/io/util/base64.h"

namespace http {
Header::Header(bool is_strict)
  : is_strict_(is_strict) {
  Clear();
}
Header::~Header() {
}

void Header::Clear() {
  bytes_parsed_ = 0;
  parse_error_ = READ_INIT;
  last_parse_error_ = READ_INIT;
  crt_parsing_field_name_.clear();
  crt_parsing_field_content_.clear();

  http_version_ = VERSION_UNKNOWN;
  method_ = METHOD_UNKNOWN;
  uri_.clear();
  status_code_ = UNKNOWN;
  reason_.clear();
  first_line_type_ = UNKNOWN_LINE;

  fields_.clear();
  verbatim_.clear();
}

const char* Header::ParseErrorName(ParseError err) {
  switch (err) {
    CONSIDER(READ_INIT);
    CONSIDER(READ_OK);
    CONSIDER(READ_NO_DATA);
    CONSIDER(READ_BAD_FIELD_SPEC);
    CONSIDER(READ_NO_FIELD);
    CONSIDER(READ_NO_STATUS_REASON);
    CONSIDER(READ_NO_REQUEST_VERSION);
    CONSIDER(READ_INVALID_STATUS_CODE);
    CONSIDER(READ_NO_STATUS_CODE);
    CONSIDER(READ_NO_REQUEST_URI);
  }
  return "UNKNOWN";
}


string Header::NormalizeFieldName(const char* field_name, int32 len) {
  const char* last = field_name + len - 1;
  while ( last >= field_name && IsLwfChar(*last) ) {
    --last;
    --len;
  }
  while ( IsLwfChar(*field_name) ) {
    ++field_name;
    --len;
  }

  string s(field_name, len);
  char* p = const_cast<char*>(s.c_str());
  bool should_upcase = true;
  while ( *p ) {
    if ( isalpha(*p) ) {
      if ( should_upcase ) {
        should_upcase = false;
        if ( islower(*p) ) {
          *p = toupper(*p);
        }
      } else if ( isupper(*p) ) {
        *p = tolower(*p);
      }
    } else {
      should_upcase = true;
      if ( IsWhiteSpace(*p) ) {
        *p = '-';
      }
    }
    ++p;
  }
  return s;
}

void Header::PrepareStatusLine(HttpReturnCode code,
                               HttpVersion version) {
  first_line_type_ = STATUS_LINE;
  status_code_ = code;
  reason_ = GetHttpReturnCodeDescription(code);
  http_version_ = version;
}

void Header::PrepareRequestLine(const char* uri,
                                HttpMethod method,
                                HttpVersion version) {
  first_line_type_ = REQUEST_LINE;
  uri_ = uri;
  method_ = method;
  http_version_ = version;
}

bool Header::AddField(const char* field_name, int32 field_name_len,
                      const char* field_content, int32 field_content_len,
                      bool replace, bool as_is) {
  if ( !IsValidFieldName(field_name, field_name_len) ||
       !IsValidFieldContent(field_content, field_content_len) ) {
    return false;
  }
  string normalized_name(as_is ?
      NormalizeFieldName(field_name, field_name_len) :
      string(field_name, field_name_len));
  const FieldMap::iterator it = fields_.find(normalized_name);
  if ( it == fields_.end() ) {
    fields_.insert(make_pair(normalized_name, string(field_content)));
  } else if ( replace || it->second.empty() ) {
    it->second = field_content;
  } else {
    it->second += ", ";
    it->second += field_content;
  }
  return true;
}

// Removes the field alltogether from the field map
bool Header::ClearField(const char* field_name, int32 len, bool as_is) {
  string normalized_name(as_is ?
      NormalizeFieldName(field_name, len) :
      string(field_name, len));
  return fields_.erase(normalized_name) > 0;
}

bool Header::IsValidFieldName(const char* field_name, int32 len) {
  const char* p = field_name;
  bool valid = false;  // ensures we do have something else beside
                       // spaces
  while ( len-- > 0 ) {
    if ( !IsTokenChar(*p) &&
         (!IsWhiteSpace(*p) || p == field_name) ) {
      return false;
    } else {
      valid = true;
    }
    ++p;
  }
  return valid && p > field_name;
}

bool Header::IsValidFieldContent(const char* field_content, int32 len) {
  const char* p = field_content;
  while ( len-- > 0 ) {
    if ( IsCtlChar(*p) && !IsLwfChar(*p) )
      return false;
    ++p;
  }
  return true;   // empty content fine..
}

int32 Header::CopyHeaderFields(const Header& src, bool replace) {
  int32 num = 0;
  for ( FieldMap::const_iterator it = src.fields().begin();
        it != src.fields().end(); ++it ) {
    if ( AddField(it->first, it->second, replace) ) {
      ++num;
    }
  }
  return num;
}

int32 Header::CopyHeaders(const Header& src, bool replace) {
  const int32 num = CopyHeaderFields(src, replace);
  http_version_ = src.http_version_;
  method_ = src.method_;
  status_code_ = src.status_code_;
  uri_ = src.uri_;
  reason_ = src.reason_;
  first_line_type_ = src.first_line_type_;

  return num;
}

const char* Header::FindField(const string& field_name,
                              int32* len) const {
  const FieldMap::const_iterator it = fields_.find(field_name);
  if ( it == fields_.end() ) {
    return NULL;
  }
  *len = it->second.size();
  return it->second.data();
}

void Header::AppendToStream(io::MemoryStream* io) const {
  io->Write(ComposeFirstLine());
  for ( FieldMap::const_iterator it = fields_.begin();
        it != fields_.end(); ++it ) {
    io->Write(it->first);
    io->Write(": ");
    io->Write(it->second);
    io->Write("\r\n");
  }
  if ( !verbatim_.empty() ) {
    io->Write(verbatim_);
  }
  io->Write("\r\n");
}
string Header::ToString() const {
  io::MemoryStream tmp;
  AppendToStream(&tmp);
  return tmp.ToString();
}


string Header::ComposeFirstLine() const {
  if ( first_line_type_ == REQUEST_LINE ) {
    //   Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
    return (string(GetHttpMethodName(method_)) + " " + uri_ + " " +
            GetHttpVersionName(http_version_) + "\r\n");
  } else if ( first_line_type_ == STATUS_LINE ) {
    //   Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    return (GetHttpVersionName(http_version_) +
            strutil::StringPrintf(" %d ", static_cast<int>(status_code_)) +
            reason_ + "\r\n");
  }
  return string(strutil::StringPrintf("--- %d UNKNOWN\r\n", first_line_type_));
}

bool Header::ParseHeader(io::MemoryStream* io,
                         FirstLineType expected_first_line) {
  CHECK_NE(last_parse_error_, READ_OK);
  if ( bytes_parsed_ == 0 ) {
    CHECK(parse_error_ == READ_INIT ||
          parse_error_ == READ_NO_DATA)
            << "Parse Error: " << ParseErrorName();
    if ( !ReadFirstLine(io, expected_first_line) ) {
      return false;
    }
  } else {
    CHECK_NE(parse_error_, READ_INIT);
  }
  return ReadHeaderFields(io);
}

//////////////////////////////////////////////////////////////////////
//
// Parsing for the first line
//
bool Header::ReadFirstLine(io::MemoryStream* io,
                           FirstLineType expected_first_line) {
  string line;
  if ( !io->ReadCRLFLine(&line) ) {
    set_parse_error(READ_NO_DATA);
    return false;
  }
  VLOG(5) << "HTTP first line parse: " << line.size() << " ["
          << strutil::JsonStrEscape(line.data(),
                                    min(static_cast<size_t>(line.size()),
                                        static_cast<size_t>(0x100U))) << "]";
  DCHECK_GE(line.size(), 2);  // at least CRLF
  bytes_parsed_ += line.size();

  // Cut the last CRLF
  line.resize(line.size() - 2);

  // We have three ' ' separated tokens. The meaning of each depend
  // on the type of line (status vs. request)

  //   Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
  //   Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

  //////////////////////////////////////////////////////////////////////
  //
  // 1nd Token
  //
  // Pull the first token - method / http_version
  size_t method_pos = line.find(' ');
  if ( method_pos == string::npos ) {
    // Error at the first step - save whatever is to be saved and return
    first_line_type_ = ERROR_LINE;
    if ( expected_first_line == REQUEST_LINE ) {
      set_parse_error(READ_NO_REQUEST_URI);
      method_ = GetHttpMethod(line.c_str());
    } else {
      set_parse_error(READ_NO_STATUS_CODE);
      http_version_ = GetHttpVersion(line.c_str());
    }
    return true;  // we can parse more
  }
  // Got a good first token !
  if ( expected_first_line == REQUEST_LINE ) {
    method_ = GetHttpMethod(line.substr(0, method_pos).c_str());
  } else {
    http_version_ = GetHttpVersion(line.substr(0, method_pos).c_str());
  }
  // Pass through any following spaces
  while ( line[method_pos] == ' ' ) {
    method_pos++;
  }

  //////////////////////////////////////////////////////////////////////
  //
  // 2nd Token
  //
  // Pull the second token - uri / status code (right after the first space)
  DCHECK_LE(method_pos, line.size());
  size_t uri_pos = line.find(' ', method_pos);
  if ( uri_pos == string::npos ) {
    // Bad - no next space - set the error
    first_line_type_ = ERROR_LINE;
    if ( expected_first_line == REQUEST_LINE ) {
      set_parse_error(READ_NO_REQUEST_VERSION);
      uri_ = line.substr(method_pos);
    } else {
      // This is not an error for 1.0 or less..
      if ( http_version_ <= VERSION_1_0 ) {
        first_line_type_ = expected_first_line;
      } else {
        set_parse_error(READ_NO_STATUS_REASON);
      }
      ParseStatusCode(line.substr(method_pos));
    }
    return true;  // we can parse more
  }

  // Got the second token !!
  if ( expected_first_line == REQUEST_LINE ) {
    uri_ = line.substr(method_pos, uri_pos - method_pos);
  } else {
    ParseStatusCode(line.substr(method_pos, uri_pos - method_pos));
  }
  // Pass through any following spaces
  while ( line[uri_pos] == ' ' ) {
    uri_pos++;
  }

  //////////////////////////////////////////////////////////////////////
  //
  // 3rd Token
  //

  // Pull the third token - if any - http version / Reason
  if ( expected_first_line == REQUEST_LINE ) {
    http_version_ = GetHttpVersion(line.substr(uri_pos).c_str());
  } else {
    reason_ = line.substr(uri_pos);
  }
  // We met our expectation !
  first_line_type_ = expected_first_line;

  return true;
}

void Header::ParseStatusCode(const string& status_code_str) {
  errno = 0;  // essential !
  const int status_code_int = strtol(status_code_str.c_str(), NULL, 10);
  if ( errno ) {
    set_parse_error(READ_INVALID_STATUS_CODE);
    status_code_ = UNKNOWN;
  } else {
    status_code_ = HttpReturnCode(status_code_int);
  }
}

bool Header::AddCrtParsingData() {
  bool ret = true;
  if ( !crt_parsing_field_name_.empty() ) {
    if ( !IsValidFieldName(crt_parsing_field_name_) ||
         !IsValidFieldContent(crt_parsing_field_content_) ) {
      set_parse_error(READ_BAD_FIELD_SPEC);
      ret = false;
    }
    if ( !is_strict_ || ret ) {
      string normalized_name(
        NormalizeFieldName(crt_parsing_field_name_));
      const FieldMap::iterator it = fields_.find(normalized_name);
      if ( it == fields_.end() ) {
        fields_.insert(make_pair(normalized_name,
                                 crt_parsing_field_content_));
      } else {
        it->second.append(", ");
        it->second.append(crt_parsing_field_content_);
      }
    }
  }
  crt_parsing_field_name_.clear();
  crt_parsing_field_content_.clear();
  return ret;
}

bool Header::ReadHeaderFields(io::MemoryStream* io) {
  string line;
  while ( io->ReadCRLFLine(&line) ) {
    DCHECK_GE(line.size(), 2);  // at least CRLF
    bytes_parsed_ += line.size();
    // Cut the last CRLF
    line.resize(line.size() - 2);

    // Process the line:
    if ( line.empty() ) {
      // The last empty line w/ CRLF
      if ( AddCrtParsingData() )
        set_parse_error(READ_OK);
      return true;
    }
    // Is a continuation header ?
    int32 lwf_end = 0;
    while ( IsLwfChar(line[lwf_end]) )
      ++lwf_end;
    if ( lwf_end > 0 ) {
      // Continuation field
      if ( crt_parsing_field_name_.empty() ) {
        // continuation w/ no previous field - discard
        set_parse_error(READ_NO_FIELD);
      } else {
        // continue field content..
        crt_parsing_field_content_.append(" ");
        crt_parsing_field_content_.append(line.substr(lwf_end));
      }
    } else {
      // Save whatever is in for us ..
      AddCrtParsingData();
      // Parse the current line ..
      size_t field_pos = line.find(':');
      if ( field_pos == string::npos ) {
        set_parse_error(READ_NO_FIELD);
      } else {
        crt_parsing_field_name_ = line.substr(0, field_pos++);
        // Pass over leading spaces before field content ..
        while ( IsLwfChar(line[field_pos]) )
          ++field_pos;
        crt_parsing_field_content_ = line.substr(field_pos);
      }
    }
  }
  set_parse_error(READ_NO_DATA);
  return false;
}

// Acceptable HTTP date formats
//
static const char* kHttpDateFormats[] = {
  //    Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
  "%a, %d %b %Y %H:%M:%S %Z",
  //    Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
  "%A, %d-%b-%y %H:%M:%S %Z",
  //    Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
  "%a %b %d %H:%M:%S %Y",
};

#ifdef MACOSX
// On macosx - no %z - so we assume gmt, and parse nothing
static const char* kHttpDateFormatsGetOsX[] = {
  //    Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
  "%a, %d %b %Y %H:%M:%S",
  //    Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
  "%A, %d-%b-%y %H:%M:%S",
  //    Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
  "%a %b %d %H:%M:%S %Y",
};
#endif


time_t Header::GetDateField(const char* field_name) {
  int32 len;
  const char* s = FindField(field_name, &len);
  if ( !s ) {
    return time_t(0);
  }
  if ( *(s+len) != '\0' )
    return time_t(0);
#ifdef MACOSX
  for ( int32 i = 0; i < NUMBEROF(kHttpDateFormats); ++i ) {
      struct tm t;
      if ( strptime(s, kHttpDateFormatsGetOsX[i], &t) != NULL ) {
          time_t orig_time = mktime(&t);
          struct tm t_converted;
          time_t orig_time_copy = orig_time;  // need a copy as gmtime_r modifies arguments
          gmtime_r(&orig_time_copy, &t_converted);
          return orig_time + (orig_time - mktime(&t_converted));
      }
  }
#else
  for ( int32 i = 0; i < NUMBEROF(kHttpDateFormats); ++i ) {
    struct tm t;
    if ( strptime(s, kHttpDateFormats[i], &t) != NULL ) {
      return mktime(&t);
    }
  }
#endif
  return time_t(0);
}

bool Header::SetDateField(const string& field_name, time_t t) {
  char buffer[128];
  struct tm tstruct;
  if ( NULL == gmtime_r(&t, &tstruct) ) {
    return false;
  }
  if ( strftime(buffer, sizeof(buffer), kHttpDateFormats[0], &tstruct) == 0 ) {
    return false;
  }
  return AddField(field_name, string(buffer), true);
}

bool Header::GetAuthorizationField(string* user, string* passwd) {
  int32 len;
  const char* s = FindField(kHeaderAuthorization, &len);
  if ( !s || len == 0 ) {
    return false;
  }
  const char* p = strchr(strutil::StrFrontTrim(s), ' ');
  len -= p - s;
  char* const decoded_field = new char[len];
  base64::Decoder decoder;
  int32 decoded_len = decoder.Decode(p, len,
                                     reinterpret_cast<uint8*>(decoded_field));
  if ( decoded_len == 0 ) {
    delete [] decoded_field;
    return false;
  }
  decoded_field[decoded_len] = '\0';
  const char* found_colon = strchr(decoded_field, ':');
  if ( found_colon == NULL ) {
    delete [] decoded_field;
    return false;
  }
  *user = string(decoded_field, found_colon - decoded_field);
  *passwd = string(found_colon + 1,
                   (decoded_field + decoded_len - found_colon - 1));
  delete [] decoded_field;
  return true;
}


// Sets the Authorization header and sets the corresponding header
// according to the specified parametes.
// Returns false if an error occured (like the user contains : and other stuff)
bool Header::SetAuthorizationField(const string& user,
                                   const string& passwd) {
  if ( user.find(':') != string::npos ) return false;
  const string to_encode(user + ":" + passwd);
  const int32 buflen = 2 * to_encode.size() + 4;
  char* encoded_header = new char[buflen];
  base64::Encoder encoder;
  const int32 len = encoder.Encode(to_encode.c_str(), to_encode.size(),
                                   encoded_header, buflen);
  const int32 len2 = encoder.EncodeEnd(encoded_header + len);
  *(encoded_header + len + len2) = '\0';
  bool ret = AddField(kHeaderAuthorization, sizeof(kHeaderAuthorization) - 1,
                      string("Basic ") + encoded_header,
                      true);
  delete [] encoded_header;
  return ret;
}


static const char kChunked[] = "chunked";
bool Header::IsChunkedTransfer() const {
  return strutil::StrCasePrefix(
    strutil::StrTrim(FindField(kHeaderTransferEncoding)).c_str(),
    kChunked);
}
void Header::SetChunkedTransfer(bool is_chunked) {
  if ( is_chunked ) {
    AddField(kHeaderTransferEncoding, kChunked, true);
  } else {
    ClearField(kHeaderTransferEncoding);
  }
}

static const char kGzip[] = "gzip";
static const char kDeflate[] = "deflate";
bool Header::IsGzipContentEncoding() const {
  return strutil::StrCasePrefix(
    strutil::StrTrim(FindField(kHeaderContentEncoding)).c_str(),
    kGzip);
}
bool Header::IsDeflateContentEncoding() const {
  return strutil::StrCasePrefix(
    strutil::StrTrim(FindField(kHeaderContentEncoding)).c_str(),
    kDeflate);
}
void Header::SetContentEncoding(const char* encoding) {
  if ( encoding ) {
    AddField(kHeaderContentEncoding, encoding, true);
  } else {
    ClearField(kHeaderContentEncoding);
  }
}

bool Header::IsKeepAliveConnection() const {
  return strutil::StrCasePrefix(
    strutil::StrTrim(FindField(kHeaderConnection)).c_str(),
    "keep-alive");
}


// For format of these things please check:
//
// http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.3
//
//     Accept-Charset: iso-8859-5, unicode-1-1;q=0.8
//     Accept: text/plain; q=0.5, text/html, text/x-dvi; q=0.8, text/x-c
//     Accept-Encoding: gzip;q=1.0, identity; q=0.5, *;q=0
//
float Header::GetHeaderAcceptance(const string& field,
                                  const string& value,
                                  const string& local_wildcard_value,
                                  const string& global_wildcard_value) const {
  int32 len;
  const char* s = FindField(field, &len);
  if ( !s ) {
    return 0.0f;
  }
  vector<string> components;
  strutil::SplitString(strutil::StrTrimCompress(string(s, len)), ",",
                       &components);

  bool found_local_wildcard = false;
  float local_wildcard_pref = 0.0f;
  bool found_global_wildcard = false;
  float global_wildcard_pref = 0.0f;

  for ( size_t i = 0; i < components.size(); i++ ) {
    vector<string> specs;
    strutil::SplitString(components[i], ";", &specs);
    float crt_quality = 1.0f;
    for ( int i = 1; i < specs.size(); i++ ) {
      if ( strutil::StrPrefix(specs[i].c_str(), "q=") ) {
        crt_quality = strtof(specs[i].c_str() + 2, NULL);
      }
    }
    if ( strutil::StrCaseEqual(specs[0], value) ) {
      // We found the value - this is definitive ..
      return crt_quality;
    } else if ( !local_wildcard_value.empty() &&
                strutil::StrCaseEqual(specs[0], local_wildcard_value) ) {
      found_local_wildcard = true;
      local_wildcard_pref = crt_quality;
    } else if ( !global_wildcard_value.empty() &&
                strutil::StrCaseEqual(specs[0], global_wildcard_value) ) {
      found_global_wildcard = true;
      global_wildcard_pref = crt_quality;
    }
  }
  if ( found_local_wildcard ) return local_wildcard_pref;
  if ( found_global_wildcard ) return global_wildcard_pref;
  return 0.0f;
}

bool Header::IsGzipAcceptableEncoding() const {
  if ( http_version_ < VERSION_1_0 ) {
    return false;
  }
  return GetHeaderAcceptance(kHeaderAcceptEncoding, kGzip, "", "*") > 0.0f;
}

bool Header::IsDeflateAcceptableEncoding() const {
  if ( http_version_ < VERSION_1_0 ) {
    return false;
  }
  return GetHeaderAcceptance(kHeaderAcceptEncoding, kDeflate, "", "*") > 0.0f;
}

bool Header::IsZippableContentType() const {
  int len;
  const char* s = FindField(kHeaderContentType, &len);
  if ( !s ) {
    return false;
  }
  if ( strutil::StrCasePrefix(s, "text/") )
    return true;
  if ( strutil::StrCasePrefix(s, "application/") )
    return true;
  return false;
}

int64 Header::DefaultBodyLen() const {
  if ( first_line_type_ == REQUEST_LINE ) {
    if ( method_ == METHOD_PUT ||
         method_ == METHOD_POST ) {
      return kMaxInt64;
    }
  } else if ( first_line_type_ == STATUS_LINE ) {
    return kMaxInt64;
  }
  return 0;
}
}
