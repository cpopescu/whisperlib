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
// Here are classes that you need for having an http server. The main idea
// is to decouple the protocol detail (server-client conversation) from
// the connection details, and the processing of request from anything else
// possible.
//
// The main classes you need to care here are:
//   - http::ServerParams - a bunch of parameters that define how the server
//              behaves (self describing)
//   - http::Server - this manages the server state. It has a name, a set
//              of ServerParams and dispatches the request processing.
//              You need to register a processing function for your desired
//              path and your function will be called when a request comes on
//              that path.
//   - http::ServerProtocol - this manages a conversation with a client:
//              it reads the requests and writes out the response out (is
//              one per connection) - the idea is to sepparate the
//              protocol details from the communication part.
//   - http::ServerConnection - a connection for talking http over. It
//              provides some interface functions and is required to call
//              functions into the protocol.
//   - http::ServerRequest - this is what needs to be processed by the
//              processing function. Upon return it should contain what the
//              client should receive. There are two modes of operation from
//              server's point of view: streaming data and normal data. For
//              the streaming data, the response data is produced in multiple
//              chuncks that are sent over a longer period of time. For normal
//              data, the entire response is produced at once.
//
// Please check http_server_test.cc for an example of how to use it. Also
// check the other comments in this .h file.
//

#ifndef __NET_HTTP_HTTP_SERVER_PROTOCOL_H__
#define __NET_HTTP_HTTP_SERVER_PROTOCOL_H__

#include <map>
#include <set>
#include <string>

#include <whisperlib/base/types.h>

#include WHISPER_HASH_SET_HEADER
#include WHISPER_HASH_MAP_HEADER

#include <whisperlib/sync/mutex.h>
#include <whisperlib/http/http_request.h>
#include <whisperlib/net/selector.h>
#include <whisperlib/net/timeouter.h>
#include <whisperlib/net/connection.h>
#include <whisperlib/net/address.h>
#include <whisperlib/url/url.h>
#include <whisperlib/net/user_authenticator.h>

namespace http {

//////////////////////////////////////////////////////////////////////
//
// ServerParams - Parameters for the behavior of the http server
//
struct ServerParams {
  // What to do when we reach the limit of the outgoing buffer ?
  enum FlowControlPolicy {
    POLICY_CLOSE = 1,          // close the connection
    POLICY_DROP_OLD_DATA = 2,  // drop the old data, keep the new one
    POLICY_DROP_NEW_DATA = 3,  // keep the old data, drop the new one
  };

  ServerParams();
  ServerParams(const string& root_url,
               bool dlog_level_,
               bool strict_request_headers,
               bool allow_all_methods,
               int32 max_header_size,
               int32 max_body_size,
               int32 max_chunk_size,
               int64 max_num_chunks,
               http::Header::ParseError worst_accepted_header_error,
               int32 max_concurrent_connections,
               int32 max_concurrent_requests,
               int32 max_concurrent_requests_per_connection,
               int32 request_read_timeout_ms,
               int32 keep_alive_timeout_sec,
               int32 reply_write_timeout_ms,
               int32 max_reply_buffer_size,
               FlowControlPolicy reply_full_buffer_policy,
               const string& default_content_type,
               bool ignore_different_http_hosts);

  // Root for our serving path
  URL root_url_;

  // Log in detail ?
  bool dlog_level_;

  // Parse strictly all HTTP headers
  bool strict_request_headers_;

  // Allowed methods - by default we allow non destructive methods only:
  // CONNECT, HEAD, GET and POST. If this is turned on, it is the
  // responsability of the processors to decide.
  bool allow_all_methods_;

  // How long the acceptable HTTP header can be ?
  int32 max_header_size_;
  // For non chunked body, how long this can be ?
  int32 max_body_size_;
  // For chunked body, how big one chunk can be ?
  int32 max_chunk_size_;
  // For chunked body, how many chuncks can we accept in a request / reply ?
  // (-1 => no limit)
  int64 max_num_chunks_;

  // When parsing headers, what is the worst acceptable error ?
  // (Recommended: Header::READ_NO_STATUS_REASON)
  http::Header::ParseError worst_accepted_header_error_;

  // How many concurrent connections can we have ?
  int32 max_concurrent_connections_;
  // How many concurrent requests can we accept ?
  int32 max_concurrent_requests_;
  // How many concurrent requests per each connection do we accept ?
  int32 max_concurrent_requests_per_connection_;

  // How long to wait to receive a request ?
  int32 request_read_timeout_ms_;
  // Timeout on keep-alive connections ? (On 0 - we close the connection)
  int32 keep_alive_timeout_sec_;
  // How long to wait for reads from the outbud to complete ?
  int32 reply_write_timeout_ms_;
  // How big can grow the reply buffer ?
  int32 max_reply_buffer_size_;
  FlowControlPolicy reply_full_buffer_policy_;

  // Default content type for our answers
  string default_content_type_;
  // If true we ignore different http hosts received in the sourece urls..
  bool ignore_different_http_hosts_;
};

class ServerRequest;
class ServerConnection;
class ServerProtocol;
class Server;

//////////////////////////////////////////////////////////////////////
//
// Server accepting connection - we need to know a factory method
//   for the serving connection.
//
class ServerAcceptor {
 public:
  ServerAcceptor(net::Selector* selector,
                 const net::NetFactory& net_factory,
                 net::PROTOCOL net_protocol,
                 const net::HostPort& local_addr,
                 http::Server* server);
  virtual ~ServerAcceptor();

  const net::HostPort& local_addr() const {
    return local_addr_;
  }

  bool Listen();
  void Close();

 private:
  // Called every time a client wants to connect.
  // We return false to shutdown this guy,
  // or true to accept it.
  bool AcceptorFilterHandler(const net::HostPort& peer_address);
  // After AcceptorFilterHandler returned true,
  // the client connection is delivered here.
  void AcceptorAcceptHandler(net::NetConnection* peer_connection);

 private:
  net::Selector* selector_;
  net::NetAcceptor* acceptor_;

  const net::HostPort local_addr_;

  // This guy does the work (registering the serving connections,
  // processing requests etc.)
  http::Server* const server_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ServerAcceptor);
};

//////////////////////////////////////////////////////////////////////

class Server {
 public:
  // Creates a server with a given name, main network thread and
  // protocol behavior.
  // The factory callback is used to create the serving connection
  // of a desired type, and requests may be processed by threads in a
  // thread pool *iff* threadpool_size > 0
  Server(const char* name,
         net::Selector* selector,
         const net::NetFactory& net_factory,
         const ServerParams& protocol_params);
  virtual ~Server();

  // Add an Acceptor for listening on a local port.
  // The local_address should contain at least the port.
  void AddAcceptor(net::PROTOCOL net_protocol,
                   const net::HostPort& local_address);

  // Binds every Acceptor to it's port and start serving requests.
  // If a bind fails, we abort (crash by SIGABRT).
  void StartServing();

  // Stop every Acceptor
  void StopServing();

  typedef Callback1<ServerRequest*> ServerCallback;

  // Registers processing functions for a given path. The new callback
  // will replace the old one (returns true).
  // If the processor is delete or replaced, the corresponding allowed
  // ips are deleted !! (so watch out).
  // The most specific path is chosen:
  //  E.g. if you register for "/test1" -> processor1 and
  //          "/test1/test2" -> processor2, for a path like:
  //  "/test1/test2/test3" -> we invoke processor2
  //  "/test1" -> we invoke processor1
  //  "/test1/test3" -> we invoke processor1
  //
  // is_public: If true, all incoming clients are served.
  //            If false, only certain allowed_ips_ can access this path.
  // auto_del_callback: Should we take ownership of the 'callback'?
  void RegisterProcessor(const string& path, ServerCallback* callback,
      bool is_public, bool auto_del_callback);

  // The reverse of RegisterProcessor.
  void UnregisterProcessor(const string& path);

  // Sets IP sets for given given path (if not public). We do not take
  // control of the pointer (which should be around for the whole
  // duration of Server's life.
  // If the given filter is NULL -> means all allowed :)
  // The path resolving is same as for processors
  void RegisterAllowedIpAddresses(const string& path,
                                  const net::IpV4Filter* ips);

  // Registers that on the given path the server receives streams
  // of data from the client and we need to do multiple processing calls.
  void RegisterClientStreaming(const string& path,
                               bool is_client_streaming);

  // Processes a given request - fully read from the client.
  // In this function we call the right handler, and in
  // case of errors (like handlers called from bad ips, 404s or requests
  // that are rejected from other reasons).
  // Used internally.
  void ProcessRequest(http::ServerRequest* req);

  // Determines if specific protocol params must be followed by the
  // given request. (e.g. is a client streaming - etc)
  // Used internally.
  void GetSpecificProtocolParams(http::ServerRequest* req);

  // Set your own default request processor
  void set_default_processor(ServerCallback* default_processor) {
    delete default_processor_;
    default_processor_ = default_processor;
  }

  // This is called in case an error occurred on the request
  void set_error_processor(ServerCallback* error_processor) {
    delete error_processor_;
    error_processor_ = error_processor;
  }

  const ServerParams& protocol_params() const {
    return protocol_params_;
  }
  int num_connections() const {
    return protocols_.size();
  }

  // Notification Received from the server acceptor when a new protocol
  // (w/ connection is created).
  void AddClient(ServerProtocol* proto);
  void DeleteClient(ServerProtocol* proto);

  net::Selector* selector() { return selector_; }
  const string& name() const { return name_; }
 private:
  void DefaultRequestProcessor(ServerRequest* req);
  void ErrorRequestProcessor(ServerRequest* req);

  // Name of the server - we return this in the "Server" field
  const string name_;
  // Main network thread selector
  net::Selector* const selector_;
  // Main network factory (for creating tcp/ssl acceptors)
  const net::NetFactory& net_factory_;

  // Parameters regarding the behavior of the protocol
  // (timeouts, buffer sizes etc)
  const ServerParams protocol_params_;

  synch::Mutex mutex_;

  // The guys who accept connections for us.
  // We use multiple acceptors, to listen on multiple ports.
  vector<ServerAcceptor*> acceptors_;

  // Callbacks for processing regular
  struct Processor {
    ServerCallback* callback_;
    bool auto_del_callback_;
    Processor(ServerCallback* callback, bool auto_del_callback)
      : callback_(callback), auto_del_callback_(auto_del_callback) {}
    ~Processor() { if ( auto_del_callback_ ) { delete callback_; } }
  };
  typedef map<string, Processor*> ProcessorMap;
  ProcessorMap processors_;

  // Signals streaming clients
  typedef map<string, bool> IsStreamingClientMap;
  IsStreamingClientMap is_streaming_client_map_;

  // Allowed ips / per path - NULL -> All allowed
  typedef map<string, const net::IpV4Filter*> AllowedIpsMap;
  AllowedIpsMap allowed_ips_;

  // This is called when we cannot find a processor for a given path
  // (by default we return a 404)
  ServerCallback* default_processor_;

  // This is called in case an error occured on the request
  // (by default we return a 500);
  ServerCallback* error_processor_;

  // All the current opened communications
  typedef hash_set<http::ServerProtocol*> ProtocolSet;
  ProtocolSet protocols_;

  // Number of equests that are in processing state - we used to track
  // w/ these the over to top clients and orphaned requests..
  typedef hash_map<http::ServerProtocol*, int> ProtoReqMap;
  ProtoReqMap protocol_outstanding_requests_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Server);
};

//////////////////////////////////////////////////////////////////////
//
// Derive your server connection from here. You need to override the
// virtual functions declared in here.. Make sure that your protocol
// sees the payload data you receive !! (called on ProcessMoreData());
// The ServerProtocol must exist for all the duration of a connection.
// The connection must inform (on deletion) that is going away..
//
class ServerConnection {
 public:
  // we take ownership of "net_connection"
  ServerConnection(net::Selector* selector,
                       net::NetConnection* net_connection,
                       http::ServerProtocol* protocol);
  virtual ~ServerConnection();

  // Called when protocol puts some data on the wire
  void NotifyWrite();

  // TODO(cosmin): remove useless function.
  //               You never separate fd from connection.
  // Detaches the connection from the fd - you an use it at that point
  // as you wish
  net::NetConnection* DetachFromFd() {
    net::NetConnection* conn = net_connection_;
    net_connection_ = NULL;
    conn->DetachAllHandlers();
    return conn;
  }
  void RequestReadEvents(bool enable) {
    if ( net_connection_ ) {
      net_connection_->RequestReadEvents(enable);
    }
  }
  void RequestWriteEvents(bool enable) {
    if ( net_connection_ ) {
      net_connection_->RequestWriteEvents(enable);
    }
  }

  const http::ServerProtocol* protocol() const;

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

 private:
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);

 private:
  net::Selector* selector_;
  net::NetConnection* net_connection_;

  // This guy needs to be informed about all data received and contains
  // the communication state (when needed)
  http::ServerProtocol* protocol_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ServerConnection);
};

//////////////////////////////////////////////////////////////////////
//
// ServerProtocol - manages a client conversation - parses the requests
//        and effectively writes the response to the client. Does some
//        request error checking (but all responses, even the error
//        ones go through the server first, even if in the end come to us).
//        We have this to separate the protocol decisions themselves
//        from the implementation details of the communication channel.
//
class ServerProtocol {
 public:
  ServerProtocol(net::Selector* server_selector,
                 net::Selector* net_selector,
                 Server* server);
  ~ServerProtocol();

  // Parameters for how we speak the protocol
  const ServerParams& protocol_params() const {
    return server_->protocol_params();
  }
  const net::HostPort& remote_address() const {
    return remote_address_;
  }
  const net::HostPort& local_address() const {
    return local_address_;
  }
  http::Server* server() const {
    return server_;
  }
  net::Selector* server_selector() const {
    return server_selector_;
  }
  net::Selector* net_selector() const {
    return net_selector_;
  }
  // Can be used to do flow control (beside the default parametes from
  // protocol_params_)
  int32 outbuf_size() const {
    return connection_ == NULL ? 0 : connection_->outbuf()->Size();
  }
  int32 free_outbuf_size() const {
    return connection_ == NULL ? 0 :
             max(0, protocol_params().max_reply_buffer_size_ -
                    protocol_params().max_header_size_ -
                    outbuf_size());
  }

  // Sets the underground TCP connection - call it once
  // (We also set some parameters)
  void set_connection(ServerConnection* conn);

  // Detaches the given request from the associated fd. On return the
  // req. will have the protocol NULL-ified, and you should not use
  // that member again.
  // It returns the fd of the serving connection - is your duty to read/write
  // from it and to close it.
  // If this returns INVALID_FD_VALUE means a orphaned connection (closed
  // connection by client in the meantime) and you should disregard any
  // continuation or use of req.
  net::NetConnection* DetachFromFd(http::ServerRequest* req);

  // called when the connection is deleted
  void NotifyConnectionDeletion();
  // called when the connection handled a write
  void NotifyConnectionWrite();

  // Flow control functions:
  void PauseReading() {
    if ( connection_ != NULL ) {
      connection_->RequestReadEvents(false);
    }
  }
  void ResumeReading() {
    if ( connection_ != NULL ) {
      connection_->RequestReadEvents(true);
    }
  }
  void ResumeWriting() {
    if ( connection_ != NULL ) {
      connection_->RequestWriteEvents(true);
    }
  }

  // This closes all active requests and clears all stuff
  // PRECONDITION: condition_ == NULL
  void CloseAllActiveRequests();

  // This callback processes more data from connection_->inbuf()
  bool ProcessMoreData();

  // Response finalization - replies to the given request. If
  // is_server_streaming is on we expect a chuncked multi chunk
  // response (you need to continue calling StreamData until you
  // call w/ a io_eos on true.
  // Is your duty not to leek requests or to double-call this function
  // w. the same req.
  //
  // PRECONDITION : a lock is held on req->request_lock() for multithreading
  void ReplyForRequest(ServerRequest* req, HttpReturnCode status);
  // Streams a new chunk of data for given request. If is_eos is true,
  // it means that it was the last chunk..
  //
  // PRECONDITION : a lock is held on req->request_lock() for multithreading
  void StreamData(ServerRequest* req, bool is_eos);

  // Does the actual header setup and puts data out (if not orphaned).
  // Returns true if we should close the conn.
  bool PrepareResponse(ServerRequest* req, HttpReturnCode status);

  const string& name() const { return name_; }
  const bool dlog_level() const { return dlog_level_; }

 private:
  // Prepares what status to return on a parsing error
  void PrepareErrorRequest(ServerRequest* server_request);
  // This disposes the request and maybe closes the connection..
  void EndRequestProcessing(ServerRequest* req, bool is_eos);
  // Timeout handling - basically we close the connection
  void HandleTimeout(int64 timeout_id);

  // Timeout ids:
  static const int32 kWriteTimeout = 1;
  static const int32 kRequestTimeout = 2;

 private:
  // not really sure what is this name
  string name_;

  const bool dlog_level_;

  // the selector of the server (probably not needed)
  net::Selector* const server_selector_;

  // our networking selector (we run exclusively on this selector)
  net::Selector* const net_selector_;

  // The HTTP server (our parent). Never goes away.
  http::Server* const server_;

  // We set timeouts using this guy
  net::Timeouter timeouter_;

  // Parses requests for us
  http::RequestParser parser_;

  // The connection that we process. THIS may GO AWAY
  ServerConnection* connection_;

  // The connection that we process
  http::ServerRequest* crt_recv_;
  // The request in process of sending

  http::ServerRequest* crt_send_;

  typedef set<http::ServerRequest*> RequestSet;
  RequestSet active_requests_;

  // The address & port where we received the request. Copied from connection_
  net::HostPort local_address_;

  // Who is (was) on the other side. Copied from connection_
  net::HostPort remote_address_;

  bool closed_;

  friend class Server;

  DISALLOW_EVIL_CONSTRUCTORS(ServerProtocol);
};

//////////////////////////////////////////////////////////////////////
//
// ServerRequest just holds data necessary for processing a request
// in the http::Server framework.
// It contains the main data structures:
//   server_ -> the processing http::Server
//   protocol_ -> manages the client connection
//   request_ -> the actual request
//
// Synchronization members:
//  ready_callback_ -> after streaming some data you can provide an callback
//         that will be called when some space is available in the
//         output buffer for client
//
// State variables:
//  is_server_streaming_ - this means that the server is sending
//        the answer in multiple data chunks.
//  is_server_streaming_chunks_ - this means that the server is
//        actually encoding the reply in chunks, otherwise the reply
//        is just a regular reply with no content-length,
//        terminated by the server closing the connection
//        is streaming but not using chunks - used only when
//        is_server_streaming_ is true
//  is_client_streaming_ - the client sends data in multiple chunks.
//        when this is on, expect to have your handler called
//        multiple times with the same ServerRequest
//  is_keep_alive_ - used internally, this signals a "Keep-Alive"
//        underline http connection
//  is_orphaned_ - is turned on by the server for connections that
//        got closed by the client / timed out or other errors.
//        When this is on you should stop processing that request and
//        discard it ..
//  free_output_bytes() - you should not output more then these many
//        bytes in the request_->server_data() as this will be more
//        then the connection output buffer and discarded / determine
//        connection closing etc
//
// The response for a request can come in two modes. The common part is
// that the relevant data must be set in request()->server_header_ (control
// data, like content type) and request()->server_data_(the actual reply).
//
class ServerRequest {
 public:
  // We are just a placeholder for these things - we do not own them, but
  // they should be around for the whole life of the request.
  // There is one exception of this rule:
  //   -- if the connection closes underneath us, we become
  explicit ServerRequest(http::ServerProtocol* protocol)
      : protocol_(protocol),
        request_(protocol->protocol_params().strict_request_headers_),
        is_server_streaming_(false),
        is_server_streaming_chunks_(true),
        is_client_streaming_(false),
        is_keep_alive_(false),
        is_orphaned_(false),
        is_parsing_finished_(false),
        ready_pending_limit_(0),
        ready_callback_(NULL),
        closed_callback_(NULL),
        server_callback_(NULL),
        is_initialized_(false) {
    UpdateOutputBytes();
  }
  ~ServerRequest() {
    clear_closed_callback();
    delete ready_callback_;
    ready_callback_ = NULL;
  }
  //////////////////////////////////////////////////////////////////////
  //
  // Accessors
  //

  const ServerParams& protocol_params() const {
    return protocol_->protocol_params();
  }

  // Use this to access the interesting data in this request (what client
  // sent) and to write data for the client.
  http::Request* request() { return &request_; }

  //  is_server_streaming_ - this means that the server is sending
  //        the answer in multiple data chunks.
  bool is_server_streaming() const { return is_server_streaming_; }

  //  is_server_streaming_chunks_ - this means that the server is
  //        actually encoding the reply in chunks.
  bool is_server_streaming_chunks() const { return is_server_streaming_chunks_;}

  //  is_client_streaming_ - the client sends data in multiple chunks.
  //        when this is on, expect to have your handler called
  //        multiple times with the same ServerRequest
  bool is_client_streaming() const { return is_client_streaming_; }

  //  is_orphaned_ - is turned on by the server for connections that
  //        got closed by the client / timed out or other errors.
  //        When this is on you should stop processing that request and
  //        delete it (for a server streaming request)
  bool is_orphaned() const { return is_orphaned_; }

  //  pending_output_bytes_ - the number of bytes still buffered by the
  //                          underlying connection
  // WARNING !!! this function is called on both: media & net threads
  int32 pending_output_bytes() const { return outbuf_size_; }
  // Don't output more then these many bytes in the request_->server_data()
  // as this will be more then the connection output buffer and
  // discarded / determine connection closing etc
  // WARNING !!! this function is called on both: media & net threads
  int32 free_output_bytes() const { return free_outbuf_size_; }
  // update the local copy of outbuf_size
  void UpdateOutputBytes() {
    CHECK(protocol_->net_selector()->IsInSelectThread());

    outbuf_size_ = protocol_->outbuf_size();
    free_outbuf_size_ = protocol_->free_outbuf_size();
  }

  // Who is on the other side of the wire ?
  const net::HostPort& remote_address() const {
    return protocol_->remote_address();
  }
  // Local address where we received the request.
  const net::HostPort& local_address() const {
    return protocol_->local_address();
  }

  //  The closed_callback is called when the peer closes connection
  //  while uploading. Useful in client streaming.
  // NOTE: If the client request completes, and the server starts sending
  //       back the response, this callback is reset to a server function.
  void set_closed_callback(Closure* closed_callback) {
    CHECK(!closed_callback->is_permanent());
    clear_closed_callback();
    closed_callback_ = closed_callback;
  }
  // Helper - clears the close callback (if we desire to close from
  // another thread
  void clear_closed_callback() {
    delete closed_callback_;
    closed_callback_ = NULL;
  }

  // 'ready_callback' will be run (Just once!! == Edge triggered)
  // in media selector, when outbuf available space is greater than
  // 'ready_pending_limit'.
  // NOTE: we take ownership of 'ready_callback', it must not be permanent.
  void set_ready_callback(Closure* ready_callback, int32 ready_pending_limit) {
    CHECK_GT(ready_pending_limit, 0);
    CHECK(!ready_callback->is_permanent());
    delete ready_callback_;
    ready_callback_ = ready_callback;
    ready_pending_limit_ = ready_pending_limit;
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Resolving 'non-streaming' (i.e. one answer shot) requests
  //

  // Request resolving methods that involve one answer per request
  // (i.e. no streaming).
  // Once you call this you should not touch the ServerRequest again
  // -> is gone
  void ReplyWithStatus(HttpReturnCode status);
  void Reply() {
    ReplyWithStatus(http::OK);
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Resolving 'steaming' (i.e. data comes to client in multiple shots)
  // requests
  //

  // Request resolving that imply streaming of data on multiple chunks
  // status should be a 1xx or 2xx code !
  // On return you should check the orphaned status of the request
  // (if is true you better not continue w/ this one).
  // Anyway, after this call, the request is still in your yard and
  // you should take care of it.
  //
  // [MIHAI] We also allow the code using the server protocol
  // to stream without encoding the reply in chunks, as this
  // is valid per the HTTP RFC and there are situations when
  // this is highly desirable (low latency, small chunks).
  //
  // See http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.4
  void BeginStreamingData(HttpReturnCode status,
                          Closure* closed_callback,
                          bool chunked);

  // Continues outputting data.
  // On return you should check the orphaned status of the request
  // (if is true you better not continue w/ this one).
  // Anyway, after this call, the request is still in your yard and
  // you should take care of it.
  void ContinueStreamingData();

  // Finishes a streaming request (outputs the last chunk of data).
  // Upon return the request is gone. You should not touch that again..
  void EndStreamingData();

  // Detaches this request from fd (check Server to see)
  net::NetConnection* DetachFromFd() {
    return protocol_->DetachFromFd(this);
  }

  // Flow control functions:
  void PauseReading() {
    protocol_->PauseReading();
  }
  void ResumeReading() {
    protocol_->ResumeReading();
  }
  void ResumeWriting() {
    protocol_->ResumeWriting();
  }

  // Authenticates this request against a given authenticator (looking at
  // the authorization fields / http Basic Authorization fields..
  // Returns the authentication status:
  //  -- true - the user is authenticated and is OK.
  //  -- false - the user is noth authenticated - and the proper reply was sent
  //   (401 reply) - you just discard it..
  bool AuthenticateRequest(net::UserAuthenticator* authenticator);
  void AuthenticateRequest(
      net::UserAuthenticator* authenticator,
      net::UserAuthenticator::AnswerCallback* answer_callback);
  void AnswerUnauthorizedRequest(net::UserAuthenticator* authenticator);


  // Returns a string representation for this request:
  string ToString() const {
    return strutil::StringPrintf("HTTP[%s -> %s]",
                                 remote_address().ToString().c_str(),
                                 request_.url() != NULL
                                 ? request_.url()->spec().c_str()
                                 : "??");
  }

  // If this is on, you will not see any more data on this guy, the parser
  // is in final state ..
  bool is_parsing_finished() const {
    return is_parsing_finished_;
  }
  net::Selector* net_selector() { return protocol_->net_selector(); }

 private:
  //////////////////////////////////////////////////////////////////////
  //
  // Internal stuff..
  //

  http::ServerProtocol* protocol() { return protocol_; }
  bool is_keep_alive() const { return is_keep_alive_; }

  // Signals output buffer space available.
  void SignalReady() {
    CHECK(net_selector()->IsInSelectThread());
    if ( ready_callback_ != NULL ) {
      CHECK(!ready_callback_->is_permanent());
      Closure* c = ready_callback_;
      ready_callback_ = NULL;
      c->Run();
    }
  }
  // Signals that request is done
  void SignalClosed() {
    if ( closed_callback_ != NULL ) {
      Closure* c = closed_callback_;
      closed_callback_ = NULL;
      c->Run();
    }
  }
  // Used *only* for detaching operations !
  void ResetProtocol() {
    protocol_ = NULL;
  }
  // Helper used to give ourselves to a processor
  void ProcessRequest() {
    server_callback_->Run(this);
  }

 private:
  // Processors:
  http::ServerProtocol* protocol_;

  // Request data
  http::Request request_;

  // Request state
  bool is_server_streaming_;
  bool is_server_streaming_chunks_;
  bool is_client_streaming_;
  bool is_keep_alive_;
  bool is_orphaned_;
  bool is_parsing_finished_;

  // When output pending bytes drops below this threshold, call read_callback_
  int32 ready_pending_limit_;
  // Notification of outbuf space available. Must run on media selector.
  // Must be: non-permanent. Called just once! (Edge Triggered)
  Closure* ready_callback_;

  // Notification of an orhphaned / closed request
  Closure* closed_callback_;

  // The callback that processes this event - set and used internally by
  // the server
  Server::ServerCallback* server_callback_;
  // Used internally to signal the header is passed and request has some
  // specific members set.
  bool is_initialized_;

  // a proxy duplicate of the protocol_->connection_->net_connection->outbuf()
  //   ->Size(). Because this value is needed in media_thread for flow control,
  // while the whole protocol+connection runs on net_thread. Poor design.
  int32 outbuf_size_;
  int32 free_outbuf_size_;

  friend class Server;
  friend class ServerProtocol;

  DISALLOW_EVIL_CONSTRUCTORS(ServerRequest);
};
}

#endif  // __NET_HTTP_HTTP_SERVER_PROTOCOL_H__
