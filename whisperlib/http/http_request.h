// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
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


#ifndef __NET_HTTP_HTTP_REQUEST_H__
#define __NET_HTTP_HTTP_REQUEST_H__

#include <string>

#include "whisperlib/base/types.h"
#include "whisperlib/http/http_consts.h"
#include "whisperlib/http/http_header.h"
#include "whisperlib/io/buffer/memory_stream.h"
#include "whisperlib/io/zlib/zlibwrapper.h"
#include "whisperlib/url/url.h"

namespace whisper {
namespace http {

//
// This class encompasses a request to an HTTP server (or from an HTTP server)
//
// client_* members - things set by the client, the HttpClientConnection would
//                    send them (header / content) to the server. The server
//                    connection set these as per client request.
// server_* members - things set by the server - the HttpServerConnection
//                    would sent these to the client.
//

class Request {
 public:
  Request(bool strict_headers = true,
          int32 client_block_size = io::DataBlock::kDefaultBufferSize,
          int32 server_block_size = io::DataBlock::kDefaultBufferSize);
  ~Request();

  // Normal accessors
  io::MemoryStream* client_data() { return &client_data_; }
  http::Header* client_header() { return &client_header_; }
  io::MemoryStream* server_data() { return &server_data_; }
  http::Header* server_header() { return &server_header_; }
  const io::MemoryStream* client_data() const { return &client_data_; }
  const http::Header* client_header() const { return &client_header_; }
  const io::MemoryStream* server_data() const { return &server_data_; }
  const http::Header* server_header() const { return &server_header_; }

  URL* url() { return url_; }   // NULL -> Invalid
  const URL* url() const { return url_; }   // NULL -> Invalid

  // This initializes the internal URL from the client request and returns it
  // (protocol is the protocol to use, unless specified in the URI)
  URL* InitializeUrlFromClientRequest(const URL& absolute_root);

  // Appends the client request to the given memory stream
  // (does chunking / gzip encoding as required).
  //
  // * You need to set the proper headers for what you want to do
  // w/ the request (e.g. Content-Encoding, Transfer-Encoding,
  // before hand) - These are also affected by the version specified
  // in the protocol.
  //
  // This *may* modify the header (e.g. fields as Content-Length,
  // Transfer-Encoding or Content-Encoding).
  // Will also clear the client_data_ :)
  //
  void AppendClientRequest(io::MemoryStream* out, int64 max_chunk_size = -1);

  // Appends the current content of client_data_ as another http chunk
  // to the given string (and clears the client_data_).
  // Called with an empty client_data_, this will close the chunk stream.
  //
  // IMPORTANT NOTE: do not try to break the protocol specification w/
  //   this call (i.e. no chunks if Content-Lenght is set and Transfer-Encoding
  //   is not chunked)
  //
  // NOTE / TODO: we do not append trailing http headers..
  bool AppendClientChunk(io::MemoryStream* out, int64 max_chunk_size = -1);

  // Appends the server reply to the given memory stream
  // (does chunking / gzip encoding as required).
  // This *may* modify the header (e.g. fields as Content-Length,
  //  or Content-Encoding) - These are also affected by the version specified
  // in the protocol.
  //
  // NOTE: if you want to encode chunked stuff, you need to enable
  //       it by setting the Transfer-Encoding to chunked. In this
  //       case Content-Length is not sent (normally).
  // If the client accepts gzip encoding and you enable it w/
  // server_use_gzip_encoding we use it and set the proper header.
  void AppendServerReply(io::MemoryStream* out,
                         bool streaming,
                         bool do_chunks,
                         int64 max_chunk_size = -1);

  // Appends the current content of server_data_ as another http chunk
  // to the given memory stream (and clears the server_data_).
  // Called with an empty client_data_, this will close the chunk stream.
  //
  // IMPORTANT NOTE: do not try to break the protocol specification w/
  //   this call (i.e. no chunks if Content-Lenght is set and Transfer-Encoding
  //   is not chunked in server header or if this http status is set to
  //   NO_CONTENT or other stuff - or in response to a HEAD request
  //   (we check the client_header_ !) this will result in an assertion
  //   failure !)
  //
  // This does not affect the headers.
  // NOTE: Chunked encoding needs to be enabled in the server_header.
  bool AppendServerChunk(io::MemoryStream* out,
                         bool do_chunks,
                         int64 max_chunk_size = -1);

  // When this is turned on we will try to use gzip encoding whenever possible
  bool server_use_gzip_encoding() const {
    return server_use_gzip_encoding_;
  }
  // Sets the use of gzip encoding - the drain buffer should be used when
  // the message in the buffer is intended to be drained, and not left
  // for further reads. You probably want to set it to true unless you
  // have a big-stream, no back / forth conversation going on, like
  // (i.e. would set it to false for something like video streaming or so).
  void set_server_use_gzip_encoding(bool use_gzip_encoding, bool gzip_drain_buffer) {
    server_use_gzip_encoding_ = use_gzip_encoding;
    server_gzip_drain_buffer_ = gzip_drain_buffer;
  }

  // In these cases no body must be transmitted
  bool NoServerBodyTransmitted() {
    const HttpReturnCode code = server_header_.status_code();
    return (client_header_.method() == METHOD_HEAD ||
            (code >= 100 && code < 200) ||
            code == NO_CONTENT ||
            code == NOT_MODIFIED);
  }

  struct RequestStats {
    int64 client_size_;
    int64 server_size_;
    int64 client_raw_size_;
    int64 server_raw_size_;
    RequestStats() {
      Clear();
    }
    void Clear() {
      client_size_ = 0;
      server_size_ = 0;
      client_raw_size_ = 0;
      server_raw_size_ = 0;
    }
  };
  const RequestStats& stats() const { return stats_; }
  RequestStats* mutable_stats() { return &stats_; }
  void ResetStats() { stats_.Clear(); }
 private:
  // Utility function that really does the chunk appending. Returns true
  // iff the last chunk and trailer was appended.
  bool AppendChunkHelper(const http::Header* src_header,
                         io::MemoryStream* src_data,
                         io::MemoryStream* out,
                         bool add_decorations,
                         int64 max_chunk_size = -1);

  // The data sent / to be sent by the client (normally the payload of
  // a POST or a PUT)
  io::MemoryStream client_data_;
  // The HTTP header sent / to be sent by the client
  http::Header client_header_;

  // The data sent / to be sent by the server (the reply body)
  io::MemoryStream server_data_;
  // The HTTP header sent / to be sent by the server
  http::Header server_header_;

  // The URL of the request
  URL* url_;

  // We were expecting some end of chunk encoding
  bool in_chunk_encoding_;

  // Used for compression;
  io::ZlibDeflateWrapper* deflate_zwrapper_;
  bool gzip_state_begin_;
  io::ZlibGzipEncodeWrapper* gzip_zwrapper_;

  bool server_use_gzip_encoding_;
  bool server_gzip_drain_buffer_;
  enum CompressOption {
    COMPRESS_NONE,
    COMPRESS_GZIP,
    COMPRESS_DEFLATE
  };
  CompressOption compress_option_;

  RequestStats stats_;

  DISALLOW_EVIL_CONSTRUCTORS(Request);
};

////////////////////////////////////////////////////////////////////////////////
//
// Class that knows to parse requests from a memory stream. Can be used
// 'hand-in-hand' with the requests. We expect for a connection to instantiate
// a single RequestParser and parse the incoming requests, while using the
// Request serialization methods to put the output data on the wire.
//
class RequestParser {
 public:
  RequestParser(
      const char* name,
      int32 max_header_size       = 16384,
      int64 max_body_size         = 4 << 20,
      int64 max_chunk_size        = 1 << 20,
      int64 max_num_chunks        = -1,
      bool accept_wrong_method    = false,
      bool accept_wrong_version   = false,
      bool accept_no_content_length = false,
      http::Header::ParseError
      worst_accepted_header_error = Header::READ_NO_STATUS_REASON);
  ~RequestParser();

  enum ParseState {
    // Fresh new parsing state
    STATE_INITIALIZED                           = 0,

    // Not states, but state range separators
    FIRST_FINAL_STATE                           = 100,
    FIRST_ERROR_STATE                           = 200,

    // In process of reading the headers
    STATE_HEADER_READING                        = 1,
    // Got the header fully
    STATE_END_OF_HEADER                         = 2,
    // Got the header fully and we do not need to read the body
    // (e.g. HEAD request etc)
    STATE_END_OF_HEADER_FINAL                   = 100,

    // In the process of reading a normal (not chunked) message body
    STATE_BODY_READING                          = 10,
    // Fully got the body.
    STATE_BODY_END                              = 110,

    // Waiting to read a chunk header (size)
    STATE_CHUNK_HEAD_READING                    = 21,
    // In the process of reading chunk data
    STATE_CHUNK_READING                         = 22,
    // At end of chunk data - waiting for the \r\n at end of chunk
    STATE_END_OF_CHUNK                          = 23,
    // Aftre the last (empty) chunk - need to read the trailing header
    STATE_LAST_CHUNK_READ                       = 24,
    // Final state at the end of all chunks and chunk trail header
    STATE_END_OF_TRAIL_HEADER                   = 120,

    // Errors that can appear in the processing of the message header
    ERROR_HEADER_BAD                            = 200,
    ERROR_HEADER_BAD_CONTENT_LEN                = 201,
    ERROR_HEADER_TOO_LONG                       = 202,
    ERROR_HEADER_LINE                           = 203,

    // Errors that can appear in the processing of regular message body
    ERROR_CONTENT_TOO_LONG                      = 210,
    ERROR_TRANSFER_ENCODING_UNKNOWN             = 211,
    ERROR_CONTENT_ENCODING_UNKNOWN              = 212,
    ERROR_CONTENT_GZIP_TOO_LONG                 = 213,
    ERROR_CONTENT_GZIP_ERROR                    = 214,
    ERROR_CONTENT_GZIP_UNFINISHED               = 215,

    // Errors that can appear in the processing of chunks
    ERROR_CHUNK_HEADER_TOO_LONG                 = 220,
    ERROR_CHUNK_TOO_LONG                        = 221,
    ERROR_CHUNK_TOO_MANY                        = 222,
    ERROR_CHUNK_TRAIL_HEADER                    = 223,
    ERROR_CHUNK_BAD_CHUNK_LENGTH                = 224,
    ERROR_CHUNK_BAD_CHUNK_TERMINATION           = 225,
    ERROR_CHUNK_BIGGER_THEN_DECLARED            = 226,
    ERROR_CHUNK_UNFINISHED_GZIP_CONTENT         = 227,
    ERROR_CHUNK_CONTINUED_FINISHED_GZIP_CONTENT = 228,
    ERROR_CHUNK_CONTENT_GZIP_TOO_LONG           = 229,
    ERROR_CHUNK_CONTENT_GZIP_ERROR              = 230,
    ERROR_CHUNK_TRAILER_TOO_LONG                = 231,
  };
  const char* ParseStateName() { return ParseStateName(parse_state_); }
  static const char* ParseStateName(ParseState state);

  // Call this before starting to parse a new request - and you better do it !
  void Clear();

  enum ReadState {
    HEADER_READ  = 1,   // header fully parsed
    BODY_READING = 2,   // in the state of reading the body (some already in
    // the body data MemoryStream - and valid.
    // Attention - we don't turn this if body needs
    // post-processing (like gzip decoding)
    CHUNKED_BODY_READING = 4,    // same as BODY_READING - but for chunked body.
    CHUNKED_TRAILER_READING = 8,  // reading the traing header of a chunked body
    BODY_FINISHED = 16,        // Body is fully finished.
    CHUNKS_FINISHED = 32,      // Chunked body is fully finished.
    REQUEST_FINISHED = 64,     // The whole request is fully finished
    CONTINUE = 128,            // informs the caller to call us again soon
  };
  static string ReadStateName(int32 read_state);

  // VERY IMPORTANT: once started the parsing of a request / reply - continue
  // it with subsequent calls
  //
  // Example of use (you may want to add some timeout provisions):
  //
  // bool HttpClientConnection::HandleRead() {
  //   if ( !BufferedConnection::HandleRead() ) {
  //     return false;
  //   }
  //   if ( !parser_.ParseServerReply(&req_) &
  //        http::Parser::REQUEST_FINISHED ) {
  //     // Wait for more data ...
  //     return true;
  //   }
  //   if ( parser_.InErrorState() ) {
  //     LOG_WARNING << "Error parsing server response";
  //     parser_.Clear();
  //     return false;
  //   }
  //   parser_.Clear();
  //   return ProcessValidRequest(req_);
  // }
  //

  // Parses a client request from memory stream and updates the data from req.
  // Returns an OR on the values in ReadState.
  // (e.g. HEADER_READ | REQUEST_FINISHED may be for a normal GET request,
  //  while HEADER_READ | BODY_READING | BODY_FINISHED may be at the end
  //  of a parsed POST request).
  int32 ParseClientRequest(io::MemoryStream* out, Request* req);

  // Parses a server reply from memory stream and updates the data from req.
  // Returns an OR on the values in ReadState.
  int32 ParseServerReply(io::MemoryStream* out, Request* req);

  ParseState parse_state() const {
    return parse_state_;
  }
  bool InFinalState() const {
    return parse_state_ >= FIRST_FINAL_STATE;
  }
  bool InErrorState() const {
    return parse_state_ >= FIRST_ERROR_STATE;
  }
  bool dlog_level() const {
    return dlog_level_;
  }
  void set_dlog_level(bool dlog_level) {
    dlog_level_ = dlog_level;
  }
  const string& name() const {
    return name_;
  }
  void set_name(const string& s) {
    name_ = s;
  }

  // Returns true if we know to parse the content encoding specified in the
  // given header (we know only identity and gzip)
  static bool IsKnownContentEncoding(const http::Header* header);

  // Returns true if we know to parse the transfer encoding specified in the
  // given header (we know only identity and chunked)
  static bool IsKnownTransferEncoding(const http::Header* header);

  // We may need to change this parameter as we can accept different #
  // of chunks depending on request
  void set_max_num_chunks(int64 max_num_chunks) {
    max_num_chunks_ = max_num_chunks;
  }
  void set_max_body_size(int64 max_body_size) {
    max_body_size_ = max_body_size;
  }
 private:
  void set_parse_state(ParseState state) {
    if ( dlog_level_ ) {
      LOG_INFO << name() << " State change: " << ParseStateName()
               << " => " << ParseStateName(state);
    }
    parse_state_ = state;
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Various parse helper functions
  //

  // Parses the payload of a message (body..)
  int32 ParsePayloadInternal(io::MemoryStream* in, http::Header* header,
                             io::MemoryStream* out);
  // Parses the body of a normally encoded transmission
  int32 ParseBodyInternal(io::MemoryStream* in, http::Header* header,
                          io::MemoryStream* out);
  // Parses the chunks of a chunked encoded transmission
  int32 ParseChunksInternal(io::MemoryStream* in, http::Header* header,
                            io::MemoryStream* out);
  // Parses the trail header of a chunked encoded transmission
  int32 ParseTrailHeader(io::MemoryStream* in, http::Header* header,
                         io::MemoryStream* out);

  // Protocol limits:
  const int32 max_header_size_;
  int64 max_body_size_;
  const int64 max_chunk_size_;
  int64 max_num_chunks_;
  const bool accept_wrong_method_;
  const bool accept_wrong_version_;
  const bool accept_no_content_length_;
  const http::Header::ParseError worst_accepted_header_error_;

  // A name for this parser (good to distinguish at log time)
  string name_;
  // Shall this parser log more ?
  bool dlog_level_;


  // The next members hold the current parsing state. Call Clear() before
  // starting a new parsing "session"
  enum NextChunkExpectation {
    EXPECT_CHUNK_NONE,       // we hace no expectaion for the next chunk
    EXPECT_CHUNK_EMPTY,      // we want an empty chunk next (i.e. eos)
    EXPECT_CHUNK_NON_EMPTY,  // we want a non-empty chunk next (ie. NO eos)
  };
  ParseState parse_state_;          // the state we are in (do not continue
                                    // parsing from a final state
  int64 body_size_to_read_;         // how much body is left to be read ?
  int64 chunk_size_to_read_;        // hom much data is left to be read in the
                                    // current chunk ?
  int64 num_chunks_read_;           // how many chuncks were read so far ?
  NextChunkExpectation next_chunk_expectation_;
                                    // what to expect from the next chunk ?
                                    // (depends on gzip decompression state).
  io::MemoryStream partial_data_;   // intermediate data holder
  http::Header trail_header_;       // we parse the chunk trailing header here

  // Used for decompression:
  io::ZlibInflateWrapper* inflate_zwrapper_;
  io::ZlibGzipDecodeWrapper* gzip_zwrapper_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(RequestParser);
};
}  // namespace http
}  // namespace whisper

#endif  // __NET_HTTP_HTTP_REQUEST_H__
