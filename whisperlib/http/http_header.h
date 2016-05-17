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

#ifndef __NET_HTTP_HTTP_HEADER_H__
#define __NET_HTTP_HTTP_HEADER_H__

#include <string>
#include <map>
#include <algorithm>

#include "whisperlib/base/types.h"
#include "whisperlib/http/http_consts.h"

using std::string;

// This class can be used to parse and compose HTTP message headers
// From protocol:
//   http://tools.ietf.org/html/rfc2616
//
//   generic-message = start-line
//                     *(message-header CRLF)
//                     CRLF
//                     [ message-body ]
//   start-line      = Request-Line | Status-Line
//
//   Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
//   Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
//
//   message-header = field-name ":" [ field-value ]
//   field-name     = token
//   field-value    = *( field-content | LWS )
//   field-content  = <the OCTETs making up the field-value
//                    and consisting of either *TEXT or combinations
//                    of token, separators, and quoted-string>
//
//   quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
//   qdtext         = <any TEXT except <">>

//
// Since crap in this kind of business can happen at all time and
// in all forms, we pass for this class all strings (and buffers)
// with size protection (no C-style strings here ..)
//

namespace whisper {
namespace io { class MemoryStream; }

namespace http {
class Header {
 public:
  explicit Header(bool is_strict = true);
  ~Header();

  typedef std::map<string, string> FieldMap;
  // Errors that can appear during parsing. They are more severe as
  // the number increases.
  // In general we can continue parsing in all states, however
  // the validity of the message can be dubious at least..
  enum ParseError {
    READ_INIT = 0,             // In clean - sheet state
    READ_OK,                   // Read finished - we got to the last CRLF
                               // and everything is fine at this point
    READ_NO_DATA,              // We probably got some stuff, but we are not in
                               // in the final stage..
    READ_BAD_FIELD_SPEC,       // Got a field with a really bad name or content
    READ_NO_FIELD,             // Got a line in which we could not identify
                               // the field name (basically no ':')
    READ_NO_STATUS_REASON,     // No status descriotion in reply
    READ_NO_REQUEST_VERSION,   // No version in the first line of request
    READ_INVALID_STATUS_CODE,  // Some bad string was given for HTTP code
    READ_NO_STATUS_CODE,       // No HTTP code in the reply
    READ_NO_REQUEST_URI,       // No URI in the first line of request
  };

  // The type of 'first line' that we got (can be request or status line
  // - see above)
  enum FirstLineType {
    UNKNOWN_LINE = 0,   // basically we did not parse it
    REQUEST_LINE,       // is a request !
    STATUS_LINE,        // is a status line !
    ERROR_LINE          // passed over, but bad stuff was in there ..
  };
  const char* ParseErrorName() const {
    return ParseErrorName(parse_error_);
  }
  static const char* ParseErrorName(ParseError err);

  // How many bytes were parsed so far (used to limit really bad
  // behaving peers).
  size_t bytes_parsed() const { return bytes_parsed_; }

  // At any point this will return the 'worst' (ie. the highest)
  // encountered error
  ParseError parse_error() const { return parse_error_; }

  // What we got at the last call of Parse
  ParseError last_parse_error() const { return last_parse_error_; }

  // Direct access to fields
  const FieldMap& fields() const { return fields_; }

  // Things that can appear in the first line..
  HttpVersion http_version() const { return http_version_; }
  HttpMethod method() const { return method_; }
  const char* method_name() const { return GetHttpMethodName(method()); }
  const string& uri() const { return uri_; }
  HttpReturnCode status_code() const { return status_code_; }
  const string reason() const { return reason_; }
  FirstLineType first_line_type() const { return first_line_type_; }

  void set_http_version(HttpVersion ver) { http_version_ = ver; }
  void set_method(HttpMethod method) { method_ = method; }
  void set_uri(const string& uri) { uri_ = uri; }
  void set_status_code(HttpReturnCode code) { status_code_ = code; }
  void set_reason(const string& reason) { reason_ = reason; }
  void set_first_line_type(FirstLineType flt) { first_line_type_ = flt; }

  // Prepares a normal status line to be returned, given an error code
  void PrepareStatusLine(HttpReturnCode code,
                         HttpVersion version = VERSION_1_1);

  // Prepares a relatively normal request line
  void PrepareRequestLine(const char* uri,
                          HttpMethod method = METHOD_GET,
                          HttpVersion version = VERSION_1_1);

  // Tries to add a field to the internal set of fields.
  // If the field content or field name is invalid it will refuse it.
  // NOTE: parsing headers *may* propagate invalid field content
  //       in the field map
  // The replace field specifies if the new content replaces the old
  // content. If not, it is appended  (with ', ') to the previous
  // content, unless the previous content is empty
  // Check out what protocol says about this situation
  //
  // Multiple message-header fields with the same field-name MAY be
  // present in a message if and only if the entire field-value for that
  // header field is defined as a comma-separated list [i.e., #(values)].
  // It MUST be possible to combine the multiple header fields into one
  // "field-name: field-value" pair, without changing the semantics of the
  // message, by appending each subsequent field-value to the first, each
  // separated by a comma.
  bool AddField(const char* field_name, int32 field_name_len,
                const char* field_content, int32 field_name_content,
                bool replace, bool as_is = false);
  bool AddField(const char* field_name, int32 field_name_len,
                const string& field_content,
                bool replace, bool as_is = false) {
    return AddField(field_name, field_name_len,
                    field_content.data(), field_content.size(),
                    replace, as_is);
  }
  bool AddField(const string& field_name, const string& field_content,
                bool replace, bool as_is = false) {
    return AddField(field_name.data(), field_name.size(),
                    field_content.data(), field_content.size(),
                    replace, as_is);
  }

  // Sets some verbatim specified headers (we basically append this at the
  // very end of encription).
  void SetVerbatim(const string& verbatim) {
    verbatim_ = verbatim;
  }

  // Removes the field alltogether from the field map
  bool ClearField(const char* field_name, int32 len, bool as_is = false);
  bool ClearField(const string& field_name /*, bool as_is = false*/) {
    return ClearField(field_name.c_str(), field_name.size());
  }

  // Given a (valid) field name it normalizes it. If the field name
  // has errors (ie. invalid chars) it will propagate them to
  // the output.
  // Normalization example:
  //  content-lengTH -> Content-Length
  static string NormalizeFieldName(const char* field_name, int32 len);
  static string NormalizeFieldName(const string& field_name) {
    return NormalizeFieldName(field_name.data(), field_name.size());
  }

  // Field lookup function - in two flavours:
  // const char* returns NULL on not found (watch out this version as
  //    returns a pointer to the internal buffer, which may go away
  //    at the very next non-const call !).
  // string* one returns false and makes a copy.
  const char* FindField(const string& field_name, int32* len) const;
  bool FindField(const string& field_name, string* field_content) const {
    const FieldMap::const_iterator it = fields_.find(field_name);
    if ( it == fields_.end() ) {
      return false;
    }
    *field_content = it->second;
    return true;
  }
  // A more natural field getting - here returns "" if not found by
  // default
  string FindField(const string& field_name) const {
    const FieldMap::const_iterator it = fields_.find(field_name);
    if ( it == fields_.end() ) {
      return "";
    }
    return it->second;
  }


  // Just confirms the field existence
  bool HasField(const string& field_name) const {
    const FieldMap::const_iterator it = fields_.find(field_name);
    return it != fields_.end();
  }

  // Copies the fields from the source Header object to this one,
  // performing for each field as instructed by the replace flag.
  // We skip over the headers over which AddField would skip.
  // Returns the number of copied fields.
  int CopyHeaders(const Header& src, bool replace);
  // Same as above but copies only the fields
  int32 CopyHeaderFields(const Header& src, bool replace);

  // Puts the object in a fresh-new state (call always befor starting
  // parsing a header)
  void Clear();

  // Parses a http request from the given MemoryStream. Designed
  // to be called first time from a Clear state and subsequently
  // as data becomes available until this function returns true.
  // The read pointer in io is advanced according to the parsed
  // data.
  bool ParseHttpRequest(io::MemoryStream* io) {
    return ParseHeader(io, REQUEST_LINE);
  }

  // Same as above, but parses a http reply (differs on how the
  // first line is parsed and considered). Same as above, call until
  // you get a true answer (or you get tired / passed through too many
  // bytes).
  // The read pointer in io is advanced according to the parsed
  // data.
  bool ParseHttpReply(io::MemoryStream* io) {
    return ParseHeader(io, STATUS_LINE);
  }

  // Reads the next header fields. Returns true when we are at the end of
  // the headers. May set in the meantime parsing_error and saves a number
  // of encountered fields into the fields_ map. The read pointer in io
  // will be set after the headers, or advanced to the point where some
  // fields are processed.
  // Can be used to read some fields without a leading line (like the fields
  // which may appear at teh end of a chunked transfer.
  bool ReadHeaderFields(io::MemoryStream* io);

  // Produces a representation of this header and appends it to the
  // provided io stream.
  // The first_line_type determines how the first line would look.
  // For UNKNOWN or ERROR cases, no first line is appended
  void AppendToStream(io::MemoryStream* io) const;

  // Produces a string w. the header data - not that fast
  string ToString() const;

  // Returns the first line as a string by composing it from components,
  // according to first_line_type_ (on invalid type returns an empty CRLF
  string ComposeFirstLine() const;

  // Checker for a field name or a field content..
  static bool IsValidFieldName(const char* name, int len);
  static bool IsValidFieldContent(const char* content, int len);
  static bool IsValidFieldName(const string& name) {
    return IsValidFieldName(name.data(), name.size());
  }
  static bool IsValidFieldContent(const string& content) {
    return IsValidFieldContent(content.data(), content.size());
  }

  // Returns if a keep-alive connection was signaled.
  bool IsKeepAliveConnection() const;

  // Returns how much would make sense to read from the buffer
  // if no Content-Lenght is provided.
  int64 DefaultBodyLen() const;

  //////////////////////////////////////////////////////////////////////
  //
  // Utility functions:

  // Returns the time_t value for an http encoded field content. On error
  // returns time_t(0)
  time_t GetDateField(const char* field_name);

  // Adds a properly formatted date in the corresponding field
  bool SetDateField(const string& field_name, time_t t);

  // Checks ig the header includes a Authorization: header, and if it does,
  // decodes the user and password and sets them in the specific parameters.
  // Returns: true if the header includs the Authorization header
  bool GetAuthorizationField(string* user, string* passwd);

  // Sets the Authorization header and sets the corresponding header
  // according to the specified parametes.
  // Returns false if an error occured (like the user contains :
  // and other stuff)
  bool SetAuthorizationField(const string& user, const string& passwd);

  // Returns true if the headers specifies a chunked transfer coding
  bool IsChunkedTransfer() const;
  // Sets a chunked transfer coding
  void SetChunkedTransfer(bool is_chunked);

  // Returns true if the headers specifies a gzipped content encoding
  bool IsGzipContentEncoding() const;
  // Returns true if the headers specifies a deflate content encoding
  bool IsDeflateContentEncoding() const;
  // Sets a chunked transfer coding
  void SetContentEncoding(const char* encoding);

  // Returns true if gzip is an acceptable encoding
  bool IsGzipAcceptableEncoding() const;
  // Returns true if deflate is an acceptable encoding
  bool IsDeflateAcceptableEncoding() const;

  // Does it make sense to compress this content type ?
  bool IsZippableContentType() const;

  // Returns the acceptance level for a value as specified in a field.
  //
  // For format of these things please check:
  // http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
  //
  // Example:
  // GetHeaderAcceptance(kHeaderAccept, "audio/mp3", "audio/*", "*/*");
  // GetHeaderAcceptance(kHeaderAcceptEncoding, "gzip", "", "*");
  //
  float GetHeaderAcceptance(const string& field,
                            const string& value,
                            const string& local_wildcard_value,
                            const string& global_wildcard_value) const;

 private:
  void set_parse_error(ParseError error) {
    last_parse_error_ = error;
    parse_error_ = std::max(error, parse_error_);
  }

  // Parsing helpers:
  bool ParseHeader(io::MemoryStream* io, FirstLineType expected_first_line);

  // Tries to parse the first line of a request/reply (according to the
  // given expectation). Sets the appropriate first line members (http_version_
  // and others) and possibly sets a parsing error.
  // Returns true if passed after the first line in io (the read pointer
  // in io will be set after that consumed first line)
  bool ReadFirstLine(io::MemoryStream* io, FirstLineType expected_first_line);
  // Extracts a http status code (and saves it to status_code_) from the
  // given name. On error sets the parsing error
  void ParseStatusCode(const string& status_code_str);
  // Appends the current intermediate values as a field name.
  bool AddCrtParsingData();

  // If this is on we refuse a bunch of field name/values. Else, we are more
  // permissive
  const bool is_strict_;
  // Through how many bytes we advanced our parsing
  int bytes_parsed_;
  // The worst parse error encountered so far
  ParseError parse_error_;
  // The error encountered in the last call to Parse* functions
  ParseError last_parse_error_;

  // Intermadiate values for
  string crt_parsing_field_name_;
  string crt_parsing_field_content_;

  // Things that can appear in the first line..
  HttpVersion http_version_;
  HttpMethod method_;
  HttpReturnCode status_code_;
  string uri_;
  string reason_;

  // Status from parsing the first line
  FirstLineType first_line_type_;

  // Field name -> field content stuff
  FieldMap fields_;

  // A verbatim text that we can add at the end of headers
  string verbatim_;

  DISALLOW_EVIL_CONSTRUCTORS(Header);
};
}  // namespace http
}  // namespace whisper

#endif  //  __NET_HTTP_HTTP_HEADER_H__
