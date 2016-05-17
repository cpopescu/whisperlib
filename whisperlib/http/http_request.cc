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

#include "whisperlib/http/http_request.h"
#include "whisperlib/base/core_errno.h"
#include "whisperlib/base/gflags.h"

using namespace std;

DEFINE_bool(http_log_detail_errors, true, "Log internal http errors");

namespace whisper {
namespace http {

////////////////////////////////////////////////////////////////////////////////

Request::Request(bool strict_headers,
                 int32 client_block_size,
                 int32 server_block_size)
  : client_data_(client_block_size),
    client_header_(strict_headers),
    server_data_(server_block_size),
    server_header_(strict_headers),
    url_(NULL),
    in_chunk_encoding_(false),
    deflate_zwrapper_(NULL),
    gzip_state_begin_(true),
    gzip_zwrapper_(NULL),
    server_use_gzip_encoding_(true),
    server_gzip_drain_buffer_(true),
    compress_option_(COMPRESS_NONE) {
}

Request::~Request() {
  delete gzip_zwrapper_;
  delete deflate_zwrapper_;
  delete url_;
}

URL* Request::InitializeUrlFromClientRequest(const URL& absolute_root) {
  if ( url_ != NULL ) {
    delete url_;
    url_ = NULL;
  }
  if ( client_header_.parse_error() < http::Header::READ_NO_REQUEST_URI ) {
    url_ = new URL(absolute_root.Resolve(client_header_.uri()));
  }
  return url_;
}

void Request::AppendClientRequest(io::MemoryStream* out, int64 max_chunk_size) {
  CHECK(!in_chunk_encoding_);
  // Make sure you prepared your request properly !
  CHECK_EQ(client_header_.first_line_type(), http::Header::REQUEST_LINE);
  DCHECK_NE(client_header_.method(), METHOD_UNKNOWN);
  DCHECK_NE(client_header_.http_version(), VERSION_UNKNOWN);

  const bool zippable_content = client_header_.IsZippableContentType();
  if ( client_header_.IsGzipContentEncoding() && zippable_content ) {
    CHECK_GE(client_header_.http_version(), VERSION_1_0);
    delete gzip_zwrapper_;
    gzip_state_begin_ = true;
    gzip_zwrapper_ = new io::ZlibGzipEncodeWrapper();
    client_header_.SetContentEncoding("gzip");
    compress_option_ = COMPRESS_GZIP;
  } else if ( client_header_.IsDeflateContentEncoding() && zippable_content ) {
    CHECK_GE(client_header_.http_version(), VERSION_1_0);
    delete deflate_zwrapper_;
    deflate_zwrapper_ = new io::ZlibDeflateWrapper();
    client_header_.SetContentEncoding("deflate");
    compress_option_ = COMPRESS_DEFLATE;
  } else {
    client_header_.SetContentEncoding(NULL);
    compress_option_ = COMPRESS_NONE;
  }

  if ( server_use_gzip_encoding_ ) {
    // We can accept gzip, and deflate but not much else..
    client_header_.AddField(kHeaderAcceptEncoding, "gzip, deflate", true);
  } else {
    client_header_.ClearField(kHeaderAcceptEncoding);
  }

  const int32 out_size = out->Size();
  const int32 client_data_size = client_data_.Size();
  if ( client_header_.IsChunkedTransfer() ) {
    in_chunk_encoding_ = true;
    CHECK_GE(client_header_.http_version(), VERSION_1_1);
    VLOG(5) << "http::Request - Streaming header to server: "
            << client_header_.ToString();
    client_header_.AppendToStream(out);
    while ( !client_data_.IsEmpty() ) {
      AppendClientChunk(out, max_chunk_size);
    }
  } else {
    // We do not know anything else - clear the transfer encoding
    client_header_.SetChunkedTransfer(false);
    // Append the body (compressed if necessary) to a temp
    io::MemoryStream ms;
    io::MemoryStream* source = &ms;
    switch ( compress_option_ ) {
    case COMPRESS_GZIP:
      source = new io::MemoryStream();
      gzip_zwrapper_->Encode(&client_data_, source);
      break;
    case COMPRESS_DEFLATE:
      source = new io::MemoryStream();
      deflate_zwrapper_->Deflate(&client_data_, source);
      break;
    case COMPRESS_NONE:
      source = &client_data_;
      break;   // no temp compression
    }
    // Set the content length from the data size
    if ( !client_data_.IsEmpty() ||
         client_header_.method() == METHOD_POST ||
         client_header_.method() == METHOD_PUT ) {
      client_header_.AddField(
          kHeaderContentLength,
          strutil::IntToString(source->Size()),
          true);  // replace !
    }
    VLOG(5) << "http::Request - Sending header to server: "
            << client_header_.ToString();
    // Append the header data and the body
    client_header_.AppendToStream(out);
    out->MarkerSet();
    out->MarkerRestore();
    out->AppendStream(source);
    if (compress_option_ != COMPRESS_NONE) {
      delete source;
    }
  }
  stats_.client_raw_size_ += out->Size() - out_size;
  stats_.client_size_ += client_data_size - client_data_.Size();
}

void Request::AppendServerReply(io::MemoryStream* out,
                                bool streaming,
                                bool do_chunks,
                                int64 max_chunk_size) {
  CHECK(!in_chunk_encoding_);
  // Make sure you prepared your request properly !
  CHECK_EQ(server_header_.first_line_type(), http::Header::STATUS_LINE);
  DCHECK_NE(server_header_.status_code(), UNKNOWN);
  if ( server_header_.http_version() == VERSION_UNKNOWN ) {
    server_header_.set_http_version(VERSION_1_1);
  }

  if ( server_header_.http_version() > client_header_.http_version() ) {
    LOG_WARNING << " Downgrading server verion: "
                << GetHttpVersionName(client_header_.http_version());
    server_header_.set_http_version(VERSION_1_0);
  }
  const bool zippable_content = server_header_.IsZippableContentType();
  if ( server_use_gzip_encoding_ && zippable_content ) {
    if ( client_header_.IsGzipAcceptableEncoding() ) {
      delete gzip_zwrapper_;
      gzip_zwrapper_ = new io::ZlibGzipEncodeWrapper();
      server_header_.SetContentEncoding("gzip");
      compress_option_ = COMPRESS_GZIP;
    } else if ( client_header_.IsDeflateAcceptableEncoding() ) {
      delete deflate_zwrapper_;
      deflate_zwrapper_ = new io::ZlibDeflateWrapper();
      server_header_.SetContentEncoding("deflate");
      compress_option_ = COMPRESS_DEFLATE;
    } else {
      server_header_.SetContentEncoding(NULL);
    }
  } else {
    server_header_.SetContentEncoding(NULL);
    compress_option_ = COMPRESS_NONE;
  }

  const int32 out_size = out->Size();
  const int32 server_data_size = server_data_.Size();
  if ( streaming ) {
    if (do_chunks && client_header_.http_version() >= VERSION_1_1) {
      server_header_.SetChunkedTransfer(true);
      CHECK_GT(client_header_.http_version(), VERSION_1_0);
    } else {
      server_header_.SetChunkedTransfer(false);
    }
    VLOG(5) << "http::Request - Streaming header to client: "
            << server_header_.ToString();
    server_header_.AppendToStream(out);
    if ( !NoServerBodyTransmitted() ) {
      in_chunk_encoding_ = true;
      while ( !server_data_.IsEmpty() ) {
        // Let them close the chunks by themselves
        AppendServerChunk(out, do_chunks, max_chunk_size);
      }
    }
  } else {
    server_header_.SetChunkedTransfer(false);
    const HttpReturnCode code = server_header_.status_code();
    io::MemoryStream ms;
    io::MemoryStream* source = &ms;
    // Append the body (compressed if necessary) to a temp
    switch ( compress_option_ ) {
    case COMPRESS_GZIP:
      gzip_zwrapper_->Encode(&server_data_, &ms);
      break;
    case COMPRESS_DEFLATE:
      deflate_zwrapper_->Deflate(&server_data_, &ms);
      break;
    case COMPRESS_NONE:
      source = &server_data_;
      break;
    }
    // Set the content length from the compressed stuff.
    if ( (code < 100 || code >= 200) &&
         code != NO_CONTENT &&
         code != NOT_MODIFIED &&
         client_header_.method() != http::METHOD_HEAD) {
      server_header_.AddField(kHeaderContentLength,
                              strutil::IntToString(source->Size()),
                              true);  // replace !
    }
    // Append the header data and the body
    server_header_.AppendToStream(out);
    VLOG(5) << "http::Request - Sending to client: "
            << server_header_.ToString();
    out->AppendStream(source);
  }
  stats_.server_raw_size_ += out->Size() - out_size;
  stats_.server_size_ += server_data_size - server_data_.Size();
}

namespace {
static void BufferAppendChunk(io::MemoryStream* in,
                              io::MemoryStream* out,
                              bool add_decorations,
                              int32 max_size) {
  int32 size = min(max_size, in->Size());
  if ( add_decorations ) {
    string first_line = strutil::StringPrintf("0%x\r\n",
                                              static_cast<int>(size));
    out->Write(first_line);
  }
  out->AppendStream(in, size);
  if ( add_decorations ) {
    out->Write("\r\n");
  }
}
}

bool Request::AppendClientChunk(io::MemoryStream* out,
                                int64 max_chunk_size) {
  return AppendChunkHelper(
    &client_header_, &client_data_, out,
    client_header_.http_version() >= VERSION_1_1, max_chunk_size);
}
bool Request::AppendServerChunk(io::MemoryStream* out,
                                bool do_chunks,
                                int64 max_chunk_size) {
  CHECK(!NoServerBodyTransmitted())
    << " Code:" << server_header_.status_code()
    << " Method:" << client_header_.method();
  return AppendChunkHelper(&server_header_, &server_data_, out,
      do_chunks && (client_header_.http_version() >= VERSION_1_1),
                           max_chunk_size);
}

bool Request::AppendChunkHelper(const http::Header* /*src_header*/,
                                io::MemoryStream* src_data,
                                io::MemoryStream* out,
                                bool add_decorations,
                                int64 max_chunk_size) {
  const bool is_empty = src_data->IsEmpty();
  if ( compress_option_ != COMPRESS_NONE ) {
    io::MemoryStream tmp;
    if ( compress_option_ == COMPRESS_GZIP ) {
      if ( gzip_state_begin_ ) {
        gzip_zwrapper_->BeginEncoding(&tmp);
        gzip_state_begin_ = false;
      }
      gzip_zwrapper_->ContinueEncoding(src_data, &tmp);
      if ( is_empty || server_gzip_drain_buffer_ ) {
        gzip_zwrapper_->EndEncoding(&tmp);
        gzip_state_begin_ = true;
      }
    } else {
      int32 size = src_data->Size();
      if ( !is_empty ) size++;   // basically do not flush the state ..
      deflate_zwrapper_->DeflateSize(src_data, &tmp, &size);
      CHECK(size == 1 || (size == 0 && is_empty))
        << "Strange zlib behaviour. Left size: " << size;
    }
    if ( tmp.IsEmpty() ) {
      if ( is_empty ) {
        // Turns out that the flush resulted in an empty body..
        in_chunk_encoding_ = false;
        if ( add_decorations ) {
          out->Write("0\r\n\r\n");
        }
        return true;
      } else {
        // The zlib did not produce anything this call - just skip it for
        // the next time ...
        return false;
      }
    } else {
      max_chunk_size = (max_chunk_size <= 0) ? kMaxInt32 : max_chunk_size;
      while (!tmp.IsEmpty()) {
        BufferAppendChunk(&tmp, out, add_decorations, max_chunk_size);
      }
      if ( is_empty ) {
        // If it was a flush, put the end of stream out..
        in_chunk_encoding_ = false;
        if ( add_decorations ) {
          out->Write("0\r\n\r\n");
        }
        return true;
      }
    }
  } else {
    // The last chunk (empty) => append empty trailed
    if ( is_empty ) {
      in_chunk_encoding_ = false;
      if ( add_decorations ) {
        out->Write("0\r\n\r\n");
      }
      return true;
    } else {
      max_chunk_size = (max_chunk_size <= 0) ? kMaxInt32 : max_chunk_size;
      while (!src_data->IsEmpty()) {
        BufferAppendChunk(src_data, out, add_decorations, max_chunk_size);
      }
      return false;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////

#define LOG_HTTP LOG_INFO_IF(dlog_level_) << name() << ": "
#define LOG_HTTP_ERR LOG_WARNING_IF(FLAGS_http_log_detail_errors)  \
  << name() << ": "

RequestParser::RequestParser(
  const char* name,
  int32 max_header_size,
  int64 max_body_size,
  int64 max_chunk_size,
  int64 max_num_chunks,
  bool accept_wrong_method,
  bool accept_wrong_version,
  bool accept_no_content_length,
  http::Header::ParseError worst_accepted_header_error)
  : max_header_size_(max_header_size),
    max_body_size_(max_body_size),
    max_chunk_size_(max_chunk_size),
    max_num_chunks_(max_num_chunks),
    accept_wrong_method_(accept_wrong_method),
    accept_wrong_version_(accept_wrong_version),
    accept_no_content_length_(accept_no_content_length),
    worst_accepted_header_error_(worst_accepted_header_error),
    name_(name),
    dlog_level_(false),
    inflate_zwrapper_(NULL),
    gzip_zwrapper_(NULL) {
  Clear();
}

RequestParser::~RequestParser() {
  Clear();
}

const char* RequestParser::ParseStateName(ParseState state) {
  switch ( state ) {
    CONSIDER(STATE_INITIALIZED);
    CONSIDER(STATE_HEADER_READING);
    CONSIDER(STATE_END_OF_HEADER);
    CONSIDER(STATE_END_OF_HEADER_FINAL);
    CONSIDER(STATE_BODY_READING);
    CONSIDER(STATE_BODY_END);
    CONSIDER(STATE_CHUNK_HEAD_READING);
    CONSIDER(STATE_CHUNK_READING);
    CONSIDER(STATE_END_OF_CHUNK);
    CONSIDER(STATE_LAST_CHUNK_READ);
    CONSIDER(STATE_END_OF_TRAIL_HEADER);
    CONSIDER(ERROR_HEADER_BAD);
    CONSIDER(ERROR_HEADER_BAD_CONTENT_LEN);
    CONSIDER(ERROR_HEADER_TOO_LONG);
    CONSIDER(ERROR_HEADER_LINE);
    CONSIDER(ERROR_CONTENT_TOO_LONG);
    CONSIDER(ERROR_TRANSFER_ENCODING_UNKNOWN);
    CONSIDER(ERROR_CONTENT_ENCODING_UNKNOWN);
    CONSIDER(ERROR_CONTENT_GZIP_TOO_LONG);
    CONSIDER(ERROR_CONTENT_GZIP_ERROR);
    CONSIDER(ERROR_CONTENT_GZIP_UNFINISHED);
    CONSIDER(ERROR_CHUNK_HEADER_TOO_LONG);
    CONSIDER(ERROR_CHUNK_TOO_LONG);
    CONSIDER(ERROR_CHUNK_TOO_MANY);
    CONSIDER(ERROR_CHUNK_TRAIL_HEADER);
    CONSIDER(ERROR_CHUNK_BAD_CHUNK_LENGTH);
    CONSIDER(ERROR_CHUNK_BAD_CHUNK_TERMINATION);
    CONSIDER(ERROR_CHUNK_BIGGER_THEN_DECLARED);
    CONSIDER(ERROR_CHUNK_UNFINISHED_GZIP_CONTENT);
    CONSIDER(ERROR_CHUNK_CONTINUED_FINISHED_GZIP_CONTENT);
    CONSIDER(ERROR_CHUNK_CONTENT_GZIP_TOO_LONG);
    CONSIDER(ERROR_CHUNK_CONTENT_GZIP_ERROR);
    CONSIDER(ERROR_CHUNK_TRAILER_TOO_LONG);
    // Not real states:
    // CONSIDER(FIRST_FINAL_STATE);
    // CONSIDER(FIRST_ERROR_STATE);
  default:
    return "INVALD_STATE";
  }
  return "INVALID_STATE";  // keep g++ happy
}

string RequestParser::ReadStateName(int32 read_state) {
  vector<string> states;
  if ( read_state & HEADER_READ ) {
    states.push_back("HEADER_READ");
  }
  if ( read_state & BODY_READING ) {
    states.push_back("BODY_READING");
  }
  if ( read_state & CHUNKED_BODY_READING ) {
    states.push_back("CHUNKED_BODY_READING");
  }
  if ( read_state & CHUNKED_TRAILER_READING ) {
    states.push_back("CHUNKED_TRAILER_READING");
  }
  if ( read_state & BODY_FINISHED ) {
    states.push_back("BODY_FINISHED");
  }
  if ( read_state & CHUNKS_FINISHED ) {
    states.push_back("CHUNKS_FINISHED");
  }
  if ( read_state & REQUEST_FINISHED ) {
    states.push_back("REQUEST_FINISHED");
  }
  if ( states.empty() ) return "NONE";
  return strutil::JoinStrings(states, " | ");
}

// Call this before starting to parse a new request - and you better do it !
void RequestParser::Clear() {
  LOG_HTTP << " Clear parse state.";
  parse_state_ = STATE_INITIALIZED;
  body_size_to_read_ = 0LL;
  chunk_size_to_read_ = 0LL;
  num_chunks_read_ = 0;
  next_chunk_expectation_ = EXPECT_CHUNK_NONE;
  partial_data_.Clear();
  trail_header_.Clear();
  delete inflate_zwrapper_;
  inflate_zwrapper_ = NULL;
  delete gzip_zwrapper_;
  gzip_zwrapper_ = NULL;
}

//////////////////////////////////////////////////////////////////////
//
// ParseClientRequest
//
int32 RequestParser::ParseClientRequest(io::MemoryStream* in, Request* req) {
  CHECK(!InFinalState());
  if ( parse_state_ == STATE_INITIALIZED ) {
    // CONTINUE - Initial state
    req->client_data()->Clear();
    req->client_header()->Clear();
    set_parse_state(STATE_HEADER_READING);
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Header reading for client
  //
  if ( parse_state_ == STATE_HEADER_READING ) {
    if ( !req->client_header()->ParseHttpRequest(in) ) {
      if ( in->Size() + req->client_header()->bytes_parsed() >
           max_header_size_ ) {
        // ERROR - header too big
        LOG_HTTP_ERR << " Header too long. Got at least: "
                     << in->Size() + req->client_header()->bytes_parsed()
                     << " max: " << max_header_size_;
        set_parse_state(ERROR_HEADER_TOO_LONG);
        return REQUEST_FINISHED;
      }
      // Clear any error w/ the first line
      req->server_header()->set_first_line_type(Header::REQUEST_LINE);
      // CONTINUE - with header parsing next call
      return 0;
    } else if ( req->client_header()->bytes_parsed() > max_header_size_ ) {
      // ERROR - header too big
      LOG_HTTP_ERR << " Header too long. Parsed: "
                   << req->client_header()->bytes_parsed()
                   << " max: " << max_header_size_;
      set_parse_state(ERROR_HEADER_TOO_LONG);
      return REQUEST_FINISHED;
    }
    // Header parsed at this point (w/ errors or not..)
    if ( (req->client_header()->http_version() == VERSION_UNKNOWN &&
          !accept_wrong_version_) ||
         (req->client_header()->method() == METHOD_UNKNOWN &&
          !accept_wrong_method_) ) {
      // ERROR - Invalid data in the first line (and unaccepted)
      LOG_HTTP_ERR << " Invalid Header line. Method: "
                   << req->client_header()->method()
                   << " Version: " << req->client_header()->http_version();
      set_parse_state(ERROR_HEADER_LINE);
      return HEADER_READ | REQUEST_FINISHED;
    }
    if ( req->client_header()->parse_error() > worst_accepted_header_error_ ) {
      // ERROR - unacceptale error in header parsing
      LOG_HTTP_ERR << " Bad Header Found: "
                   << req->client_header()->ParseErrorName();
      set_parse_state(ERROR_HEADER_BAD);
      return HEADER_READ | REQUEST_FINISHED;
    }
    // CONTINUE - header parsed good enough
    set_parse_state(STATE_END_OF_HEADER);
    // Give guys a chance to check things after the header was parsed..
    return HEADER_READ | CONTINUE;
  }
  // CONTINUE - w/ the parsing of the message payload
  return ParsePayloadInternal(in, req->client_header(), req->client_data());
}


//////////////////////////////////////////////////////////////////////
//
// ParseServerReply
//
int32 RequestParser::ParseServerReply(io::MemoryStream* in, Request* req) {
  CHECK(!InFinalState());
  if ( parse_state_ == STATE_INITIALIZED ) {
    // CONTINUE - Initial state
    req->server_data()->Clear();
    req->server_header()->Clear();
    set_parse_state(STATE_HEADER_READING);
  }
  //////////////////////////////////////////////////////////////////////
  //
  // Header reading for server
  //
  if ( parse_state_ == STATE_HEADER_READING ) {
    if ( !req->server_header()->ParseHttpReply(in) ) {
      if ( in->Size() + req->server_header()->bytes_parsed() >
           max_header_size_ ) {
        // ERROR - header too big
        LOG_HTTP_ERR << " Header too long. Got at least: "
                     << in->Size() + req->server_header()->bytes_parsed()
                     << " on max: " << max_header_size_;
        set_parse_state(ERROR_HEADER_TOO_LONG);
        return REQUEST_FINISHED;
      }
      // Clear any error w/ the first line
      req->server_header()->set_first_line_type(Header::STATUS_LINE);
      // CONTINUE - with header parsing next call
      return 0;
    } else if ( req->server_header()->bytes_parsed() > max_header_size_ ) {
      // ERROR - header too big
      LOG_HTTP_ERR << " Header too long. Parsed: "
                   << req->server_header()->bytes_parsed()
                   << " on max: " << max_header_size_;
      set_parse_state(ERROR_HEADER_TOO_LONG);
      return REQUEST_FINISHED;
    }
    if ( req->server_header()->parse_error() > worst_accepted_header_error_ ) {
      // ERROR - unacceptable error in the header
      LOG_HTTP_ERR << " Error in headers found: "
                   << req->client_header()->ParseErrorName();
      set_parse_state(ERROR_HEADER_BAD);
      return HEADER_READ | REQUEST_FINISHED;
    }
    if ( req->NoServerBodyTransmitted() ) {
      // FINAL - We don't have a body in these cases !
      set_parse_state(STATE_END_OF_HEADER_FINAL);
      return HEADER_READ | REQUEST_FINISHED;
    }
    // TODO(cpopescu):  "multipart/byteranges"

    // CONTINUE - header parsed is good enough
    set_parse_state(STATE_END_OF_HEADER);

    // Give guys a chance to check things after the header was parsed..
    return HEADER_READ | CONTINUE;
  }
  // CONTINUE - w/ the parsing of the message payload
  return ParsePayloadInternal(in, req->server_header(), req->server_data());
}

//////////////////////////////////////////////////////////////////////
//
// ParsePayloadInternal - parses the message payload (body)
//
int32 RequestParser::ParsePayloadInternal(io::MemoryStream* in,
                                          http::Header* header,
                                          io::MemoryStream* out) {
  CHECK_GE(parse_state_, STATE_END_OF_HEADER);

  //////////////////////////////////////////////////////////////////////
  //
  // Detrmine the trasmission mode for the body
  //
  if ( parse_state_ == STATE_END_OF_HEADER ) {
    if ( !IsKnownContentEncoding(header) ) {
      // ERROR - we don't know this content encoding (identity & gzip accepted)
      LOG_HTTP_ERR << " Unknown content encoding in header: "
                   << header->ToString();
      set_parse_state(ERROR_CONTENT_ENCODING_UNKNOWN);
      return HEADER_READ | REQUEST_FINISHED;
    }
    if ( !IsKnownTransferEncoding(header) ) {
      // ERROR - we don't know this transfer encoding
      // (identity & chunk accepted)
      LOG_HTTP_ERR << " Unknown transfer encoding in header: "
                   << header->ToString();
      set_parse_state(ERROR_TRANSFER_ENCODING_UNKNOWN);
      return HEADER_READ | REQUEST_FINISHED;
    }
    if ( !header->IsChunkedTransfer() ) {
      // Identity transder encoding => expect Content Lenght (in this state)
      string content_length_str;
      if ( !header->FindField(kHeaderContentLength, &content_length_str) ) {
        body_size_to_read_ = header->DefaultBodyLen();
        if ( !accept_no_content_length_ && body_size_to_read_ > 0 ) {
          // ERROR - There is no content length header
          set_parse_state(STATE_END_OF_HEADER_FINAL);
          return HEADER_READ | REQUEST_FINISHED;
        }
        if (max_body_size_ >= 0) {
          if ( body_size_to_read_ > max_body_size_ ) {
            body_size_to_read_ = max_body_size_;
          }
        }
        // CONTINUE -> read as much as we consider it makes sense..
        set_parse_state(STATE_BODY_READING);
      } else {
        errno = 0;  // essential as strtol would not set a 0 errno
        const int32 content_length = strtol(content_length_str.c_str(),
                                            NULL, 10);
        if ( errno || content_length < 0 ) {
          LOG_HTTP << " Content-Length found: [" << content_length_str << "].";
          // ERROR - Badly specified content length
          LOG_HTTP_ERR << " Bad Content Length in header: ["
                       << content_length_str << "]";
          set_parse_state(ERROR_HEADER_BAD_CONTENT_LEN);
          return HEADER_READ | REQUEST_FINISHED;
        }
        // CONTINUE -> expect identity encoded transfer w/ a content length
        body_size_to_read_ = static_cast<int64>(content_length);
        set_parse_state(STATE_BODY_READING);
      }
    } else {
      // CONTINUE -> expect chunked encoding transfer
      set_parse_state(STATE_CHUNK_HEAD_READING);
    }
  }

  if ( parse_state_ == STATE_BODY_READING ) {
    // CONTINUE - Body reading - regular - Content-Length specified
    const int32 result = ParseBodyInternal(in, header, out);
    return result;
  }
  // We better be in a chunks reading state :)
  CHECK(parse_state_ >= STATE_CHUNK_HEAD_READING)
    << " In error state - this is a bug: " << ParseStateName();
  return ParseChunksInternal(in, header, out);
}

//////////////////////////////////////////////////////////////////////
//
// ParseBodyInternal - parses the body for non chunked transfers
//
int32 RequestParser::ParseBodyInternal(io::MemoryStream* in,
                                       http::Header* header,
                                       io::MemoryStream* out) {
  CHECK_EQ(parse_state_, STATE_BODY_READING);
  if (max_body_size_ >= 0) {
    if ( body_size_to_read_ > max_body_size_ ) {
      // ERROR - body too big ..
      LOG_HTTP_ERR << " Content too long. Got " << body_size_to_read_
                   << "  on a max of: " << max_body_size_;
      set_parse_state(ERROR_CONTENT_TOO_LONG);
      return HEADER_READ | REQUEST_FINISHED;
    }
  }
  // copy the payload from in to our internal buffer.
  const int64 to_read = min(body_size_to_read_,
                            static_cast<int64>(in->Size()));
  partial_data_.AppendStreamNonDestructive(in, to_read);
  in->Skip(to_read);
  body_size_to_read_ -= to_read;
  bool is_gzipped = header->IsGzipContentEncoding();
  bool is_deflated = header->IsDeflateContentEncoding();
  if ( inflate_zwrapper_ != NULL && is_gzipped ) {
    // One converted gzip to deflate (declared deflate but is gzip..
    is_deflated = true;
    is_gzipped = false;
  }

  LOG_HTTP << " Reading body data:" << to_read
           << " left:" << body_size_to_read_
           << " partial_data_.Size()=" << partial_data_.Size()
           << " is_gzipped: " << is_gzipped;

  if ( is_gzipped ) {
    ///////////////////////////////////////////////////////////////////
    //
    // GZIP - content encoding
    //
    CHECK(!is_deflated);

    bool maybe_try_deflate = gzip_zwrapper_ == NULL;
    if ( gzip_zwrapper_ == NULL ) {
      gzip_zwrapper_ = new io::ZlibGzipDecodeWrapper();
    }
    int32 consumed_size = 0;
    do {
      partial_data_.MarkerSet();
      io::MemoryStream tmp;
      const int32 initial_size = partial_data_.Size();
      const int zerr = gzip_zwrapper_->Decode(&partial_data_, &tmp);
      consumed_size = initial_size - partial_data_.Size();
      // TODO(cosming) TODO: this is a test, remove
      //DLOG_INFO << "GZip Decoder zerr="
      //          << zerr << " consumed_size=" << consumed_size
      //          << " remaining=" << partial_data_.Size();
      if ( zerr == Z_STREAM_END ) {
        partial_data_.MarkerClear();
        out->AppendStream(&tmp);
        if ( body_size_to_read_ == 0 && partial_data_.IsEmpty() ) {
          // FINAL - state w/ body decompressed ok
          set_parse_state(STATE_BODY_END);
          return HEADER_READ | BODY_FINISHED | REQUEST_FINISHED;
        }
        // CONTINUE
      } else if ( zerr != Z_OK ) {
        if ( !maybe_try_deflate ) {
          LOG_HTTP << " ZLib error found in gzip encoding: " << zerr;
          partial_data_.MarkerClear();
          // ERROR - error data in compressed body
          LOG_HTTP_ERR << " Error in gzipped content: " << zerr;
          return HEADER_READ | BODY_READING | REQUEST_FINISHED;
        }
        LOG_HTTP << " ZLib error found in gzip encoding - trying deflate.";
        partial_data_.MarkerRestore();
        is_deflated = true;
        // CONTINUE  w/ deflate
      } else {
        partial_data_.MarkerClear();
        out->AppendStream(&tmp);
        if ( body_size_to_read_ == 0 ) {
          // ERROR - no more data left in body, but zip did not finish
          LOG_HTTP_ERR << " Error in gzipped content: Unfinished gzip";
          set_parse_state(ERROR_CONTENT_GZIP_UNFINISHED);
          return HEADER_READ | BODY_FINISHED | REQUEST_FINISHED;
        }
      }
    } while ( !is_deflated && consumed_size > 0 && !partial_data_.IsEmpty() );
  }
  if ( is_deflated ) {
    ///////////////////////////////////////////////////////////////////
    //
    // DEFLATE - content encoding
    //
    if ( inflate_zwrapper_ == NULL ) {
      inflate_zwrapper_ = new io::ZlibInflateWrapper();
    }
    const int zerr = inflate_zwrapper_->InflateSize(&partial_data_, out);
    if ( zerr == Z_STREAM_END ) {
      if ( body_size_to_read_ > 0 ) {
        LOG_HTTP_ERR << " ZLib EOS when left in body: " << body_size_to_read_;
        // ERROR - End of gzipped data, but body continues
        set_parse_state(ERROR_CONTENT_GZIP_TOO_LONG);
        return HEADER_READ | BODY_READING | REQUEST_FINISHED;
      }
      // FINAL - state w/ body decompressed ok
      set_parse_state(STATE_BODY_END);
      return HEADER_READ | BODY_FINISHED | REQUEST_FINISHED;
    }
    if ( zerr != Z_OK ) {
      LOG_HTTP_ERR << " ZLib error found: " << zerr;
      // ERROR - error data in compressed body
      set_parse_state(ERROR_CONTENT_GZIP_ERROR);
      return HEADER_READ | BODY_READING | REQUEST_FINISHED;
    }
  } else if ( !is_gzipped ) {
    ///////////////////////////////////////////////////////////////////
    //
    // IDENTITY - content encoding
    //
    out->AppendStream(&partial_data_);
    if ( body_size_to_read_ == 0 ) {
      // FINAL - state w/ body set OK
      set_parse_state(STATE_BODY_END);
      return HEADER_READ | BODY_FINISHED | REQUEST_FINISHED;
    }
  }
  // CONTINUE - with the compressed body
  DCHECK_EQ(parse_state_, STATE_BODY_READING);
  return HEADER_READ | BODY_READING;
}


//////////////////////////////////////////////////////////////////////
//
// ParseChunksInternal - parses the body for chunked transfers
//

// A small cut-and-paste from the protocol RFC:
//
//  Chunked-Body   = *chunk
//                   last-chunk
//                   trailer
//                   CRLF
//
//  chunk          = chunk-size [ chunk-extension ] CRLF
//                   chunk-data CRLF
//  chunk-size     = 1*HEX
//  last-chunk     = 1*("0") [ chunk-extension ] CRLF
//
//  chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
//  chunk-ext-name = token
//  chunk-ext-val  = token | quoted-string
//  chunk-data     = chunk-size(OCTET)
//  trailer        = *(entity-header CRLF)
//
int32 RequestParser::ParseChunksInternal(io::MemoryStream* in,
                                         http::Header* header,
                                         io::MemoryStream* out) {
  do {
    //////////////////////////////////////////////////////////////////////
    //
    // STATE_CHUNK_HEAD_READING ----------> Read the chunk header
    //
    if ( parse_state_ == STATE_CHUNK_HEAD_READING ) {
      string line;
      if ( !in->ReadCRLFLine(&line) ) {
        if ( in->Size() > max_header_size_ ) {
          // ERROR - got too much in the first chunk line
          LOG_HTTP_ERR << " Chumk header too long. Have at least: "
                       << in->Size() << "  on a max of: " << max_header_size_;
          set_parse_state(ERROR_CHUNK_HEADER_TOO_LONG);
          return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
        }
        // CONTINUE - waiting for more data in the first chunk line
        return HEADER_READ | CHUNKED_BODY_READING;
      }
      // Got one more chunk header !! - Cut the last CRLF
      DCHECK_GE(line.size(), 2);  // at least CRLF
      line.resize(line.size() - 2);
      // We ignore the chunk extension (if any)
      errno = 0;  // essential as strtol would not set a 0 errno
      const int32 chunk_length = strtol(line.c_str(), NULL, 16);
      if ( errno || chunk_length < 0 ) {
        LOG_HTTP_ERR << " Invalid chunk len specification : " << line;
        // ERROR - Badly specified chunk lenght
        set_parse_state(ERROR_CHUNK_BAD_CHUNK_LENGTH);
        return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
      }
      if ( chunk_length == 0 ) {
        if ( next_chunk_expectation_ == EXPECT_CHUNK_NON_EMPTY ) {
          // ERROR - wanted a non empty chunk - got an empty one ..
          LOG_HTTP_ERR << " Unfinished gzip content. When expecting chunk "
                       << " got end of stream";
          set_parse_state(ERROR_CHUNK_UNFINISHED_GZIP_CONTENT);
          return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
        }
        next_chunk_expectation_ = EXPECT_CHUNK_NONE;
        // Last chunk signaled !!
        set_parse_state(STATE_LAST_CHUNK_READ);
        continue;
      } else {
        LOG_HTTP << " Got a chunk size of: " << chunk_length
                 << " original hex: " << line;
        if ( next_chunk_expectation_ == EXPECT_CHUNK_EMPTY ) {
          // ERROR - wanted an end of body (empty chunk) - got some content
          LOG_HTTP_ERR << " Long chunk content. When expecting no chunk "
                       << " got extra data.";
          set_parse_state(ERROR_CHUNK_CONTINUED_FINISHED_GZIP_CONTENT);
          return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
        }
        if ( chunk_length > max_chunk_size_ ) {
          // ERROR - Chunk too long signaled
          LOG_HTTP_ERR << " Chunk to long obtained: " << chunk_length
                       << "  when max is: " << max_chunk_size_;
          set_parse_state(ERROR_CHUNK_TOO_LONG);
          return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
        }
        next_chunk_expectation_ = EXPECT_CHUNK_NONE;
        num_chunks_read_++;
        if ( max_num_chunks_ >= 0 && num_chunks_read_ > max_num_chunks_ ) {
          // ERROR - Too many chunks transmitted
          LOG_HTTP_ERR << " Too many chunks in request. Got: "
                       << num_chunks_read_
                       << "  when max is: " << max_num_chunks_;
          set_parse_state(ERROR_CHUNK_TOO_MANY);
          return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
        }
        // CONTINUE - reading the chunk data !
        chunk_size_to_read_ = chunk_length;
        set_parse_state(STATE_CHUNK_READING);
      }
    } else if ( parse_state_ == STATE_END_OF_CHUNK ) {
      //////////////////////////////////////////////////////////////////////
      //
      //  STATE_END_OF_CHUNK -----------> Read the \r\n chunk termiantion
      //
      string line;
      if ( !in->ReadCRLFLine(&line) ) {
        if ( in->Size() > strlen("\r\n") ) {
          LOG_HTTP_ERR << " Got at the end of chunks invalid leftover data: "
                       << in->Size();
          set_parse_state(ERROR_CHUNK_BAD_CHUNK_TERMINATION);
          return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
        }
        // CONTINUE - waiting for more data in the chunk termination
        return HEADER_READ | CHUNKED_BODY_READING;
      }
      // Got one more chunk header !! - Cut the last CRLF
      DCHECK_GE(line.size(), 2);  // at least CRLF
      line.resize(line.size() - 2);
      if ( !line.empty() ) {
        LOG_HTTP_ERR
            << " Got a non empty line at end of chunk: "
            << strutil::PrintableDataBuffer(
                reinterpret_cast<const uint8*>(line.data()), line.size());
        // ERROR: got some chars before chunk termination..
        set_parse_state(ERROR_CHUNK_BIGGER_THEN_DECLARED);
        return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
      }
      // CONTINUE - read the next chunck header
      set_parse_state(STATE_CHUNK_HEAD_READING);
    } else if ( parse_state_ == STATE_CHUNK_READING ) {
      //////////////////////////////////////////////////////////////////////
      //
      //  STATE_CHUNK_READING -----------> Pull Chunk content
      //
      if ( in->IsEmpty() ) {
        return HEADER_READ | CHUNKED_BODY_READING;
      }
      const int64 to_read = min(chunk_size_to_read_,
                                static_cast<int64>(in->Size()));
      partial_data_.AppendStreamNonDestructive(in, to_read);
      in->Skip(to_read);

      bool is_gzipped = header->IsGzipContentEncoding();
      bool is_deflated = header->IsDeflateContentEncoding();
      if ( is_gzipped && inflate_zwrapper_ != NULL ) {
        // one gzip converted to deflate (normally bad server)
        is_deflated = true;
        is_gzipped = false;
      }

      chunk_size_to_read_ -= to_read;
      LOG_HTTP << " Reading chunk data: " << to_read
               << " left: " << chunk_size_to_read_
               << " is_gzipped: " << is_gzipped
               << " is_deflate: " << is_deflated;

      if ( chunk_size_to_read_ == 0 ) {
        set_parse_state(STATE_END_OF_CHUNK);
      }
      if ( is_gzipped ) {
        ///////////////////////////////////////////////////////////////////
        //
        // GZIP - content encoding
        //
        //
        // TODO(cpopescu): protect from chunks too long after
        //                 decompression !!!
        //
        CHECK(!is_deflated);
        if ( partial_data_.Size() <
             io::ZlibGzipDecodeWrapper::kMinGzipDataSize ) {
          if ( in->Size() > io::ZlibGzipDecodeWrapper::kMinGzipDataSize )
            continue;
          return HEADER_READ | BODY_READING;
        }
        bool maybe_try_deflate = gzip_zwrapper_ == NULL;
        if ( gzip_zwrapper_ == NULL ) {
          gzip_zwrapper_ = new io::ZlibGzipDecodeWrapper();
        }
        int32 consumed_size = 0;
        do {
          partial_data_.MarkerSet();
          io::MemoryStream tmp;
          const int32 initial_size = partial_data_.Size();
          const int zerr = gzip_zwrapper_->Decode(&partial_data_, &tmp);
          consumed_size = initial_size - partial_data_.Size();
          if ( zerr == Z_STREAM_END ) {
            partial_data_.MarkerClear();
            out->AppendStream(&tmp);
            // CONTINUE if ( partial_data_.IsEmpty() ) {
            // We have more data to process..
          }  else if ( zerr != Z_OK ) {
            if ( !maybe_try_deflate ) {
              LOG_HTTP_ERR << " ZLib error found in gzip encoding: " << zerr;
              partial_data_.MarkerClear();
              // ERROR - error data in compressed body
              set_parse_state(ERROR_CHUNK_CONTENT_GZIP_ERROR);
              return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
            }
            LOG_HTTP << " ZLib error found in gzip encoding - trying deflate.";
            partial_data_.MarkerRestore();
            is_deflated = true;
          } else {
            partial_data_.MarkerClear();
            out->AppendStream(&tmp);
            if ( chunk_size_to_read_ == 0 ) {
              // CONTINUE - no more data left in the chunk,
              //            but zip did not finish -  we expect to continue !!
              next_chunk_expectation_ = EXPECT_CHUNK_NON_EMPTY;
            }
          }
        } while ( !is_deflated && consumed_size > 0 &&
                  !partial_data_.IsEmpty() );
      }
      if ( is_deflated ) {
        ///////////////////////////////////////////////////////////////////
        //
        // DEFLATE - content encoding
        //
        if ( inflate_zwrapper_ == NULL ) {
          inflate_zwrapper_ = new io::ZlibInflateWrapper();
        }
        const int zerr = inflate_zwrapper_->InflateSize(&partial_data_, out);
        if ( zerr == Z_STREAM_END ) {
          if ( chunk_size_to_read_ > 0 ) {
            LOG_HTTP_ERR << " ZLib EOS when left in chunk: "
                         << chunk_size_to_read_;
            // ERROR - End of gzipped data, but chunk continues
            set_parse_state(ERROR_CHUNK_CONTENT_GZIP_TOO_LONG);
            return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
          }
          // FINAL - state w/ body decompressed ok - but we also expect
          //         no chunks to follow
          next_chunk_expectation_ = EXPECT_CHUNK_EMPTY;
        } else if ( zerr != Z_OK ) {
          // ERROR - error data in compressed body
          LOG_HTTP_ERR << " ZLib error found: " << zerr;
          set_parse_state(ERROR_CHUNK_CONTENT_GZIP_ERROR);
          return HEADER_READ | CHUNKED_BODY_READING | REQUEST_FINISHED;
        } else if ( chunk_size_to_read_ == 0 ) {
          // CONTINUE - no more data left in the chunk, but zip did not finish
          //         - we expect to continue !!
          next_chunk_expectation_ = EXPECT_CHUNK_NON_EMPTY;
        }
      } else if ( !is_gzipped ) {
        ///////////////////////////////////////////////////////////////////
        //
        // IDENTITY - content encoding
        //
        out->AppendStream(&partial_data_);
      }
    } else if ( parse_state_ == STATE_LAST_CHUNK_READ ) {
      //////////////////////////////////////////////////////////////////////
      //
      //  STATE_LAST_CHUNK_READ -----------> Parse the trail header
      //
      return ParseTrailHeader(in, header, out);
    } else {
      LOG_FATAL << " Got in a wrong state - this is a bug: "
                << ParseStateName();
    }
  } while (true);  // should exit w/ a return in the code above..
  return 0;  // keep g++ happy
}

//////////////////////////////////////////////////////////////////////
//
// ParseTrailHeader - helper for parsing the trailer of a chunked
//                    transmission
//
int32 RequestParser::ParseTrailHeader(io::MemoryStream* in,
                                      http::Header* header,
                                      io::MemoryStream* /*out*/) {
  CHECK_EQ(parse_state_, STATE_LAST_CHUNK_READ);
  if ( !trail_header_.ReadHeaderFields(in) ) {
    if ( trail_header_.bytes_parsed() + in->Size() > max_header_size_ ) {
      // ERROR : trail header too long.
      LOG_HTTP_ERR << " Chunk trailer has header too long. Got at least: "
                   << trail_header_.bytes_parsed() + in->Size()
                   << " on a max of: " << max_header_size_;
      set_parse_state(ERROR_CHUNK_TRAILER_TOO_LONG);
      return HEADER_READ | CHUNKED_TRAILER_READING | REQUEST_FINISHED;
    }
    return HEADER_READ | CHUNKED_TRAILER_READING;
  }
  // CONTINUE - got the trailing header
  if ( trail_header_.parse_error() > worst_accepted_header_error_ ) {
    // ERROR - unacceptale error in trailed header parsing
    LOG_HTTP_ERR << " Error in trail headers found: "
                 << trail_header_.ParseErrorName();
    set_parse_state(ERROR_CHUNK_TRAIL_HEADER);
    return HEADER_READ | CHUNKED_TRAILER_READING | REQUEST_FINISHED;
  }
  // Copy all trailing header in the provided header
  //
  // TODO(cpopescu): it is correct this way ??
  //
  header->CopyHeaderFields(trail_header_, false);
  set_parse_state(STATE_END_OF_TRAIL_HEADER);
  return HEADER_READ | CHUNKS_FINISHED | REQUEST_FINISHED;
}

//////////////////////////////////////////////////////////////////////


static const char kGzip[] = "gzip";
static const char kDeflate[] = "deflate";
static const char kIdentity[] = "identity";
static const char kChunked[] = "chunked";

bool RequestParser::IsKnownContentEncoding(const http::Header* header) {
  int len;
  const char* s = header->FindField(kHeaderContentEncoding, &len);
  if ( !s ) return true;
  string s_trim = strutil::StrTrim(string(s, len));
  if ( s_trim.empty() ||
       strutil::StrCasePrefix(s_trim.c_str(), kGzip) ||
       strutil::StrCasePrefix(s_trim.c_str(), kDeflate) ||
       strutil::StrCasePrefix(s_trim.c_str(), kIdentity) ) {
    return true;
  }
  LOG_WARNING << " Unknown Content Encoding found: [" << s_trim << "].";
  return false;
}

bool RequestParser::IsKnownTransferEncoding(const http::Header* header) {
  int len;
  const char* s = header->FindField(kHeaderTransferEncoding, &len);
  if ( !s ) return true;
  string s_trim = strutil::StrTrim(string(s, len));
  if ( s_trim.empty() ||
       strutil::StrCasePrefix(s_trim.c_str(), kChunked) ||
       strutil::StrCasePrefix(s_trim.c_str(), kIdentity) ) {
    return true;
  }
  LOG_WARNING << " Unknown Transfer Encoding found: [" << s_trim << "].";
  return false;
}
}  // namespace http
}  // namespace whisper
