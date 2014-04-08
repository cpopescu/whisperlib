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

#ifndef __NET_HTTP_HTTP_CLIENT_PROTOCOL_H__
#define __NET_HTTP_HTTP_CLIENT_PROTOCOL_H__

#include <deque>
#include <utility>
#include <string>
#include <vector>

#include <whisperlib/base/types.h>
#include WHISPER_HASH_MAP_HEADER

#include <whisperlib/io/buffer/memory_stream.h>
#include <whisperlib/http/http_request.h>
#include <whisperlib/net/selector.h>
#include <whisperlib/net/timeouter.h>
#include <whisperlib/net/connection.h>
#include <whisperlib/net/address.h>
#include <whisperlib/url/url.h>

namespace http {

//////////////////////////////////////////////////////////////////////
//
// This determines how we behave in our communication w/ the server
//
struct ClientParams {
  ClientParams();
  ClientParams(const string& user_agent,
               bool dlog_level,
               int32 max_header_size,
               int64 max_body_size,
               int64 max_chunk_size,
               int64 max_num_chunks,
               bool  accept_no_content_length,
               int32 max_concurrent_requests,
               int32 max_waiting_requests,
               int32 default_request_timeout_ms,
               int32 connect_timeout_ms,
               int32 write_timeout_ms,
               int32 read_timeout_ms,
               int32 max_output_buffer_size,
               int32 keep_alive_sec);

  // HTTP version to use
  http::HttpVersion version_;

  // We send this as user agent for all requests
  string user_agent_;

  // Log in detail ?
  bool dlog_level_;

  // How long the acceptable HTTP header can be ?
  int32 max_header_size_;
  // For non chunked body, how long this can be ?
  int64 max_body_size_;
  // For chunked body, how big one chunk can be ?
  int64 max_chunk_size_;
  // For chunked body, how many chuncks can we accept in a request / reply ?
  // (-1 => no limit)
  int64 max_num_chunks_;
  // We read replies to end of connection ..
  bool accept_no_content_length_;

  // How many concurrent requests can we send ?
  int32 max_concurrent_requests_;
  // How many requests can be in the waiting queue ?
  int32 max_waiting_requests_;
  // Timeout for a request (begin to end - unless is a streaming server answer)
  // We use this if request->request_timeout_ms_ is 0
  int32 default_request_timeout_ms_;
  // Timeout for connecting to the server
  int32 connect_timeout_ms_;
  // Writing timeout - from us to server
  int32 write_timeout_ms_;
  // Reading timeout - after starting to receive an aswer how long we
  // wait for the next bytes
  int32 read_timeout_ms_;
  // How much client buffering we hold when streaming to servers
  int32 max_output_buffer_size_;

  // Do we want to keep alive the connection ?
  int32 keep_alive_sec_;
};

//////////////////////////////////////////////////////////////////////

// Errors that can appear in the communication w/ the server
// (for http protocol errors check the server response)
enum ClientError {
  // No error - just signals that the request is during processing
  CONN_INCOMPLETE         = 0,
  // everything finished OK
  CONN_OK                 = 1,
  // error in connecting to the server (probably none listening)
  CONN_CONNECT_ERROR      = 2,
  // timeout in connecting to the server
  CONN_CONNECT_TIMEOUT    = 3,
  // timeout while sending the request to the server
  CONN_WRITE_TIMEOUT      = 4,
  // timeout while reading the response from the server
  CONN_READ_TIMEOUT       = 5,
  // the server closed the connection while we were talking w/ it
  CONN_CONNECTION_CLOSED  = 6,
  // the request timed out (application layer)
  CONN_REQUEST_TIMEOUT    = 7,
  // some request on the line before this one failed..
  CONN_DEPENDENCY_FAILURE = 8,
  // we got too many waiting requests for this server
  CONN_TOO_MANY_REQUESTS  = 9,
  // we got some parsing error for the http protocol
  CONN_HTTP_PARSING_ERROR = 10,
  // the client requested closing of connection
  CONN_CLIENT_CLOSE       = 11,
  // the client tried too many times
  CONN_TOO_MANY_RETRIES   = 12,
};

const char* ClientErrorName(ClientError err);

class BaseClientConnection;
class ClientRequest;

class BaseClientProtocol {
 public:
  // Creates a client - we start owning the connection !!
  BaseClientProtocol(const ClientParams* params,
                     http::BaseClientConnection* connection,
                     net::HostPort server);
  virtual ~BaseClientProtocol();

  ClientError conn_error() const { return conn_error_; }
  const char* conn_error_name() const { return ClientErrorName(conn_error_); }

  // Closes all pending requests and underlying connection.
  void Clear();

  // called when the connection got connected to the server
  virtual bool NotifyConnected();
  // called to more data from connection_->inbuf()
  virtual bool NotifyConnectionRead();
  // called when the connection handled a write
  virtual void NotifyConnectionWrite();

  // called when the connection is deleted
  virtual void NotifyConnectionDeletion();

  // called when a timeout happended - return true if not handled
  // and connection should continue.
  virtual bool HandleTimeout(int64 timeout_id);

  // Flow control functions:
  void PauseReading();
  void ResumeReading();
  void PauseWriting();
  void ResumeWriting();

  // How much data can be written in the output buffer of
  // a request (request()->client_data()) to satisfy the
  // flow control of the application.
  int32 available_output_size() const {
    return available_output_size_;
  }
  // Returns if the connection is (still) alive
  bool IsAlive() const {
    return connection_ != NULL;
  }
  BaseClientConnection* connection() {
    return connection_;
  }
 protected:
  // Helper for writing a request out to the server
  void SendRequestToServer(ClientRequest* req);

 private:
  void HandleTimeoutEvent(int64 timeout_id);
 protected:
  // Timeout ids - should be used by all
  static const int64 kConnectTimeout = 1;
  static const int64 kWriteTimeout   = 2;
  static const int64 kReadTimeout    = 3;
  static const int64 kRequestTimeout = 4;

  const string name_;
  const ClientParams* params_;
  const net::HostPort server_;

  int32 available_output_size_;
  ClientError conn_error_;

  BaseClientConnection* connection_;
  ClientRequest* current_request_;

  net::Timeouter timeouter_;          // We set timeouts using this guy

  int parser_read_state_;             // The last parser status returned
  RequestParser parser_;              // parses replies for us
 private:
  DISALLOW_EVIL_CONSTRUCTORS(BaseClientProtocol);
};

//////////////////////////////////////////////////////////////////////
//
// Derive your server connection from here. You need to override the
// virtual functions declared in here.. Make sure that your protocol
// sees the payload data you receive !! (called on NotifyConnectionRead());
// The ClientProtocol must exist for all the duration of a connection.
// The connection must inform (on deletion) that is going away..
//
class BaseClientConnection {
 public:
  BaseClientConnection(net::Selector* selector,
                       const net::NetFactory& net_factory,
                       net::PROTOCOL net_protocol);
  explicit BaseClientConnection(net::Selector* selector);
  virtual ~BaseClientConnection();

  // Called when protocol puts some data on the wire
  virtual void NotifyWrite();

  net::Selector* selector() {
    return selector_;
  }
  const http::BaseClientProtocol* protocol() const {
    return protocol_;
  }
  void  set_protocol(http::BaseClientProtocol* protocol) {
    protocol_ = protocol;
  }

  void Connect(const net::HostPort& addr) {
    if ( !net_connection_->Connect(addr) ) {
      LOG_ERROR << "BaseClientConnection failed to connect to: " << addr;
      ConnectionCloseHandler(net_connection_->last_error_code(),
          net::NetConnection::CLOSE_READ_WRITE);
    }
    // if Connect fails, it calls ConnectionCloseHandler(errno, ..)
  }
  net::NetConnection::State state() const {
    return net_connection_->state();
  }
  const net::HostPort& remote_address() const {
    return net_connection_->remote_address();
  }
  const net::HostPort& local_address() const {
    return net_connection_->local_address();
  }
  io::MemoryStream* inbuf() {
    return net_connection_->inbuf();
  }
  io::MemoryStream* outbuf() {
    return net_connection_->outbuf();
  }
  int64 count_bytes_written() const {
    return net_connection_->count_bytes_written();
  }
  int64 count_bytes_read() const {
    return net_connection_->count_bytes_read();
  }
  void FlushAndClose() {
    net_connection_->FlushAndClose();
  }
  void ForceClose() {
    net_connection_->ForceClose();
  }
  void RequestReadEvents(bool enable) {
    net_connection_->RequestReadEvents(enable);
  }
  void RequestWriteEvents(bool enable) {
    net_connection_->RequestWriteEvents(enable);
  }
 private:
  void Initialize(const net::NetFactory& net_factory,
                  net::PROTOCOL net_protocol);
  void ConnectionConnectHandler();
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);

 private:
   net::Selector * selector_;

   // underlying TcpConnection or SslConnection
   net::NetConnection* net_connection_;

  // This guy needs to be informed about all data received and contains
  // the communication state (when needed)
  http::BaseClientProtocol* protocol_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(BaseClientConnection);
};

//////////////////////////////////////////////////////////////////////
//
// SimpleClientConnection - a simple implementation of
//     BaseClientConnection for communication over a simple, unprotected
//     TCP connection.
//
class SimpleClientConnection : public BaseClientConnection {
 public:
  SimpleClientConnection(net::Selector* selector,
                         const net::NetFactory& net_factory,
                         net::PROTOCOL net_protocol)
      : BaseClientConnection(selector, net_factory, net_protocol) {
  }
  explicit SimpleClientConnection(net::Selector* selector)
      : BaseClientConnection(selector) {
  }
  virtual ~SimpleClientConnection() {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(SimpleClientConnection);
};

#ifndef WHISPER_DISABLE_STREAMING_HTTP_CLIENTS

//////////////////////////////////////////////////////////////////////
//
// ClientStreamingProtocol - A client that streams data to server
//
class ClientStreamingProtocol : public BaseClientProtocol {
 public:
  // A function for streaming more data - the ClientStreamingProtocol
  // call this one w/ an int32 - size that it can accept to be written.
  // If int32 is negative - is a signal that the request ended. (also the
  // given request has some kind of finalization error set
  typedef ResultCallback1<bool, int32> StreamingCallback;

  // Creates a client - we start owning the connection !!
  // We begin the tcp/ssl connection NOW.
  ClientStreamingProtocol(const ClientParams* params,
                          http::BaseClientConnection* connection,
                          net::HostPort server);
  virtual ~ClientStreamingProtocol();

  // INTERFACE FUNCTION:

  // Starts streaming - the request should contain the client request
  // and maybe the first chunk of data. Upon more data available in the
  // output buffer, the client protocol calls the provided function
  // for the user to dump more data into request->request()->client_
  // streaming_callback should be permanent.
  // *IMPORTANT*  We never own the request or the callback
  void BeginStreaming(ClientRequest* request,
                      StreamingCallback* streaming_callback);


  // INTERNAL FUNCTIONS:

  // called to more data from connection_->inbuf()
  virtual bool NotifyConnectionRead();
  // called when the connection handled a write
  virtual void NotifyConnectionWrite();
  // called when the connection is deleted
  virtual void NotifyConnectionDeletion();

 private:
  bool source_stopped_;
  StreamingCallback* streaming_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(ClientStreamingProtocol);
};

//////////////////////////////////////////////////////////////////////
//
// ClientStreamReceiverProtocol - A client that sends a request to the
//       server, then receives the reply as a stream for the server.
//
class ClientStreamReceiverProtocol : public BaseClientProtocol {
 public:
  // Creates a client - we start owning the connection.
  // We begin the tcp/ssl connection NOW.
  ClientStreamReceiverProtocol(const ClientParams* params,
                               http::BaseClientConnection* connection,
                               net::HostPort server);
  virtual ~ClientStreamReceiverProtocol();

  // INTERFACE FUNCTION:

  // Starts the given request. Upon receiving some data or on some error
  // we call the provided callback (which should be permanent)
  // *IMPORTANT*  We never own the request or the callback
  void BeginStreamReceiving(ClientRequest* request,
                            Closure* new_data_callback);

  // INTERNAL FUNCTIONS:

  // called by the connection to read more data from connection_->inbuf()
  virtual bool NotifyConnectionRead();
  // called when the connection is deleted
  virtual void NotifyConnectionDeletion();

 private:
  ClientRequest* request_;
  Closure* streaming_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(ClientStreamReceiverProtocol);
};

#endif  // WHISPER_DISABLE_STREAMING_HTTP_CLIENTS

///////////////////////////////////////////////////////////////////////
//
// A "normal" client - for sending and receiving full described
// requests in one buffer shot.
//
class ClientProtocol : public BaseClientProtocol  {
 public:
  ClientProtocol(const ClientParams* params,
                 http::BaseClientConnection* connection,
                 net::HostPort server);
  // NOTE: Be sure you call Clear() to complete all pending queries,
  //       before deleting the ClientProtocol !
  virtual ~ClientProtocol();

  // INTERFACE FUNCTION:

  // The normal request / reply communication paradigm - we start this
  // request and call once the done callback when the request is done.
  void SendRequest(ClientRequest* request, Closure* done_callback);

  int32 num_active_requests() const { return active_requests_.size(); }
  int32 num_waiting_requests() const { return waiting_requests_.size(); }

  // INTERNAL FUNCTIONS:


  // called when the connection got connected to the server
  virtual bool NotifyConnected();
  // called to more data from connection_->inbuf()
  virtual bool NotifyConnectionRead();
  // called when the connection is deleted
  virtual void NotifyConnectionDeletion();

  // We want to expire the proper connection
  bool HandleTimeout(int64 timeout_id);


  // Calls are resolve callbacks and closes the connection with the current
  // connection error.
  void ResolveAllRequestsWithError();
  // Calls are resolve callbacks and closes the connection with the provided
  // connection error.
  void ResolveAllRequestsWithProvidedError(ClientError err) {
    conn_error_ = err;
    ResolveAllRequestsWithError();
  }

  string StatusString() const;

 private:
  // Finds which active request is currently in reading process (based
  // on the request_id header
  bool IdentifyReadingRequest();

  // Writes some requests to the server, until the active queue is full
  void WriteRequestsToServer();


  // each new request gets a unique ID assigned as crt_id_ (after which
  // crt_id_ is incremented)
  int64 crt_id_;

  // The active requests are those sent to the server. This is at most
  // of size params_->max_concurrent_requests_ (in normal HTTP
  // conversation this would be 1, but we know to interleave the requests.
  typedef hash_map<int64, ClientRequest*> RequestMap;
  RequestMap active_requests_;

  // Requests waiting for processing (to become active)
  typedef deque<ClientRequest*> RequestsQueue;
  RequestsQueue waiting_requests_;

  // What closures to call upon completion
  typedef hash_map<ClientRequest*, Closure*> CallbackMap;
  CallbackMap callback_map_;

  // Request currently in reading ..
  ClientRequest* reading_request_;

  DISALLOW_EVIL_CONSTRUCTORS(ClientProtocol);
};

//////////////////////////////////////////////////////////////////////

class ClientRequest  {
 public:
  // One empty request
  ClientRequest();

  // Constructs a request w/ a *good* URL
  ClientRequest(HttpMethod http_method, const URL* url);

  // Constructs a request w/ a *good* escaped URI path/query
  // (e.g. "/a%20b?x=%25-%3D" not "/a b?x=%-="
  ClientRequest(HttpMethod http_method,
                const string& escaped_query_path);
  // Constructs a request w/ an unescaped URI path / query.
  // (e.g. unescaped_path = "/a b", unescaped_query_comp = [("x", "%-=")]
  //  will result in a request URI of "/a%20b?x=%25-%3D"
  // If query component is null - no query component is given
  ClientRequest(HttpMethod http_method,
                const string& unescaped_path,
                const vector< pair<string, string> >* unescaped_query_comp);
  // Same as abive, but you can also specify a fragment (the piece after #)
  ClientRequest(HttpMethod http_method,
                const string& unescaped_path,
                const vector< pair<string, string> >* unescaped_query_comp,
                const string& fragment);

  // Given a set of unescaped query components returns the escaped well formed
  // query.
  // (e.g. unescaped_query_comp = [("x", "%-=")] => "x=%25-%3D"
  // Use this to set the body of a POST with form parameters !
  static string EscapeQueryParameters(
      const vector< pair<string, string> >& unescaped_query_comp);

  http::Request* request() { return &request_; }
  http::ClientError error() const { return error_; }
  const char* error_name() const {
    return http::ClientErrorName(error_);
  }
  bool is_finalized() const {
    return error_ >= CONN_OK;
  }

  int32 request_timeout_ms() const { return request_timeout_ms_; }
  int64 request_id() const { return request_id_; }
  bool is_pure_dumping() const { return is_pure_dumping_; }

  void set_error(http::ClientError error) { error_ = error; }
  void set_request_timeout_ms(int32 t) { request_timeout_ms_ = t; }
  void set_request_id(int64 request_id) { request_id_ = request_id; }
  void set_is_pure_dumping(bool val) { is_pure_dumping_ = val; }

  string name() const {
    return strutil::StrTrim(
        request_.client_header()->ComposeFirstLine()) +
        strutil::StringPrintf(" req_id: %lld", (long long int) request_id_);
  }
 private:
  http::Request request_;
  http::ClientError error_;
  // timeout for the completion of the request (begin to end).
  // If 0 we use the default timeout from protocol params.
  int32 request_timeout_ms_;
  // An id for a request in a conversation with multiple concurrent
  // request
  int64 request_id_;
  // If this is on we just dump the request - w/ no content length and
  // other checks..
  bool is_pure_dumping_;

  friend class ClientProtocol;

  DISALLOW_EVIL_CONSTRUCTORS(ClientRequest);
};
}

#endif  // __NET_HTTP_HTTP_CLIENT_PROTOCOL_H__
