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

#include "whisperlib/http/http_client_protocol.h"
#include "whisperlib/base/core_errno.h"

#define LOG_HTTP \
    LOG_INFO_IF(params_->dlog_level_) << " - HTTP[" << server_ << "]: "

namespace http {

//////////////////////////////////////////////////////////////////////
//
// ClientParams
//

ClientParams::ClientParams()
    : version_(VERSION_1_1),
      user_agent_("Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.6) "
                  "Gecko/20060601 Firefox/2.0.0.6 (Ubuntu-edgy)"),
      dlog_level_(false),
      max_header_size_(4096),
      max_body_size_(-1),
      max_chunk_size_(1 << 18),
      max_num_chunks_(-1),
      accept_no_content_length_(true),
      max_concurrent_requests_(1),
      max_waiting_requests_(100),
      default_request_timeout_ms_(120000),
      connect_timeout_ms_(20000),
      write_timeout_ms_(20000),
      read_timeout_ms_(20000),
      max_output_buffer_size_(1<<17),
      keep_alive_sec_(300000) {
}

ClientParams::ClientParams(const string& user_agent,
                           bool dlog_level,
                           int32 max_header_size,
                           int64 max_body_size,
                           int64 max_chunk_size,
                           int64 max_num_chunks,
                           bool accept_no_content_length,
                           int32 max_concurrent_requests,
                           int32 max_waiting_requests,
                           int32 default_request_timeout_ms,
                           int32 connect_timeout_ms,
                           int32 write_timeout_ms,
                           int32 read_timeout_ms,
                           int32 max_output_buffer_size,
                           int32 keep_alive_sec)
    : version_(VERSION_1_1),
      user_agent_(user_agent),
      dlog_level_(dlog_level),
      max_header_size_(max_header_size),
      max_body_size_(max_body_size),
      max_chunk_size_(max_chunk_size),
      max_num_chunks_(max_num_chunks),
      accept_no_content_length_(accept_no_content_length),
      max_concurrent_requests_(max_concurrent_requests),
      max_waiting_requests_(max_waiting_requests),
      default_request_timeout_ms_(default_request_timeout_ms),
      connect_timeout_ms_(connect_timeout_ms),
      write_timeout_ms_(write_timeout_ms),
      read_timeout_ms_(read_timeout_ms),
      max_output_buffer_size_(max_output_buffer_size),
      keep_alive_sec_(keep_alive_sec) {
}

//////////////////////////////////////////////////////////////////////
//
// ClientRequest
//

ClientRequest::ClientRequest()
    : error_(http::CONN_INCOMPLETE),
      request_timeout_ms_(0),
      request_id_(0),
      is_pure_dumping_(false) {
}

ClientRequest::ClientRequest(HttpMethod http_method,
                             const URL* url)
    : error_(http::CONN_INCOMPLETE),
      request_timeout_ms_(0),
      request_id_(0),
      is_pure_dumping_(false) {
  request_.client_header()->PrepareRequestLine(
      url->PathForRequest().c_str(), http_method);
}

ClientRequest::ClientRequest(HttpMethod http_method,
                             const string& escaped_query_path)
    : error_(http::CONN_INCOMPLETE),
      request_timeout_ms_(0),
      request_id_(0),
      is_pure_dumping_(false) {
  request_.client_header()->PrepareRequestLine(escaped_query_path.c_str(),
                                               http_method);
}
ClientRequest::ClientRequest(
    HttpMethod http_method,
    const string& unescaped_path,
    const vector< pair<string, string> >* unescaped_query_comp)
    : error_(http::CONN_INCOMPLETE),
      request_timeout_ms_(0),
      is_pure_dumping_(false) {
  string uri(URL::UrlEscape(unescaped_path));
  if ( unescaped_query_comp != NULL ) {
    uri += "?";
    uri += EscapeQueryParameters(*unescaped_query_comp);
  }
  request_.client_header()->PrepareRequestLine(uri.c_str(),
                                               http_method);
}

ClientRequest::ClientRequest(
    HttpMethod http_method,
    const string& unescaped_path,
    const vector< pair<string, string> >* unescaped_query_comp,
    const string& fragment)
    : error_(http::CONN_INCOMPLETE),
      request_timeout_ms_(0),
      is_pure_dumping_(false) {
  string uri(URL::UrlEscape(unescaped_path));
  if ( unescaped_query_comp != NULL ) {
    uri += "?";
    uri += EscapeQueryParameters(*unescaped_query_comp);
  }
  uri += "#";
  uri += fragment;
  request_.client_header()->PrepareRequestLine(uri.c_str(),
                                               http_method);
}

string ClientRequest::EscapeQueryParameters(
    const vector< pair<string, string> >& unescaped_query_comp) {
  string s;
  for ( int i = 0; i < unescaped_query_comp.size(); ++i ) {
    if ( i > 0 ) s += "&";
    s += (URL::UrlEscape(unescaped_query_comp[i].first) + "=" +
          URL::UrlEscape(unescaped_query_comp[i].second));
  }
  return s;
}

//////////////////////////////////////////////////////////////////////
//
// BaseClientConnection
//
BaseClientConnection::BaseClientConnection(net::Selector* selector,
                                           const net::NetFactory& net_factory,
                                           net::PROTOCOL net_protocol)
    : selector_(selector),
      net_connection_(NULL),
      protocol_(NULL) {
  Initialize(net_factory, net_protocol);
}

BaseClientConnection::BaseClientConnection(net::Selector* selector)
    : selector_(selector),
      net_connection_(NULL),
      protocol_(NULL) {
  Initialize(net::NetFactory(selector), net::PROTOCOL_TCP);
}

void BaseClientConnection::Initialize(const net::NetFactory& net_factory,
                                      net::PROTOCOL net_protocol) {
  net_connection_ = net_factory.CreateConnection(net_protocol);
  net_connection_->SetReadHandler(NewPermanentCallback(
      this, &BaseClientConnection::ConnectionReadHandler), true);
  net_connection_->SetWriteHandler(NewPermanentCallback(
      this, &BaseClientConnection::ConnectionWriteHandler), true);
  net_connection_->SetConnectHandler(NewPermanentCallback(
      this, &BaseClientConnection::ConnectionConnectHandler), true);
  net_connection_->SetCloseHandler(NewPermanentCallback(
      this, &BaseClientConnection::ConnectionCloseHandler), true);
}
BaseClientConnection::~BaseClientConnection() {
  delete net_connection_;
  net_connection_ = NULL;
}

void BaseClientConnection::NotifyWrite() {
  net_connection_->RequestWriteEvents(true);
}

void BaseClientConnection::ConnectionConnectHandler() {
  if ( !protocol_->NotifyConnected() ) {
    ForceClose();
  }
}

bool BaseClientConnection::ConnectionReadHandler() {
  if ( !protocol_->NotifyConnectionRead() ) {
    ForceClose();
  }
  return true;
}

bool BaseClientConnection::ConnectionWriteHandler() {
  protocol_->NotifyConnectionWrite();
  return true;
}
void BaseClientConnection::ConnectionCloseHandler(
    int err, net::NetConnection::CloseWhat what) {
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    net_connection_->FlushAndClose();
    return;
  }
  if ( protocol_ ) {
    protocol_->NotifyConnectionDeletion();
    protocol_ = NULL;
  }
  VLOG(5) << "HTTP BaseClientConnection completely closed,"
      " deleting in select loop..";
  selector_->DeleteInSelectLoop(this);
}

//////////////////////////////////////////////////////////////////////
//
//  BaseClientProtocol
//
BaseClientProtocol::BaseClientProtocol(
    const ClientParams* params,
    http::BaseClientConnection* connection,
    net::HostPort server)
    : name_(string("HTTP CLI[") + server.ToString() + "]"),
      params_(params),
      server_(server),
      available_output_size_(params->max_output_buffer_size_),
      conn_error_(http::CONN_INCOMPLETE),
      connection_(connection),
      current_request_(NULL),
      timeouter_(connection->selector(),
                 NewPermanentCallback(
                     this, &BaseClientProtocol::HandleTimeoutEvent)),
      parser_read_state_(0),
      parser_(name_.c_str(),
              params->max_header_size_,
              params->max_body_size_,
              params->max_chunk_size_,
              params->max_num_chunks_,
              false,   // accept_wrong_method
              false,   // accept_wrong_version
              params->accept_no_content_length_) {
  connection_->set_protocol(this);
  parser_.set_dlog_level(params->dlog_level_);
}

BaseClientProtocol::~BaseClientProtocol() {
  if ( connection_ != NULL ) {
    connection_->set_protocol(NULL);
    connection_->ForceClose();
    connection_ = NULL;
  }
  CHECK(current_request_ == NULL);
}

void BaseClientProtocol::Clear() {
  if ( connection_ != NULL ) {
    connection_->ForceClose();
    connection_ = NULL;
  }
}


bool BaseClientProtocol::NotifyConnected() {
  conn_error_ = CONN_OK;
  timeouter_.UnsetTimeout(kConnectTimeout);
  SendRequestToServer(current_request_);
  return true;
}

void BaseClientProtocol::SendRequestToServer(ClientRequest* req) {
  // Write the first stuff..
  if ( params_->keep_alive_sec_ > 0 ) {
    req->request()->client_header()->AddField(
        kHeaderKeepAlive,
        strutil::StringPrintf("%d", static_cast<int>(params_->keep_alive_sec_)),
        true);
    req->request()->client_header()->AddField(
        kHeaderConnection, "Keep-Alive", true);
  }
  if (!req->request()->client_header()->HasField(http::kHeaderHost)) {
    req->request()->client_header()->AddField(http::kHeaderHost,
      connection_->remote_address().ip_object().ToString(), true);
  }

  if ( req->is_pure_dumping() ) {
    req->request()->client_header()->AppendToStream(connection_->outbuf());
    connection_->outbuf()->AppendStream(req->request()->client_data());
  } else {
    req->request()->AppendClientRequest(connection_->outbuf());
  }
  if ( params_->write_timeout_ms_ > 0 ) {
    timeouter_.SetTimeout(kWriteTimeout, params_->write_timeout_ms_);
  }
  if ( params_->default_request_timeout_ms_ > 0 ||
       req->request_timeout_ms() > 0 ) {
    timeouter_.SetTimeout(kRequestTimeout + req->request_id(),
                          max(params_->default_request_timeout_ms_,
                              req->request_timeout_ms()));
  } else if ( params_->read_timeout_ms_ > 0 ) {
    timeouter_.SetTimeout(kWriteTimeout, params_->read_timeout_ms_);
  }

  connection_->NotifyWrite();
  // TODO(cosmin): We should NEVER disable read events.
  //connection_->RequestReadEvents(true);
}

void BaseClientProtocol::NotifyConnectionDeletion() {
  LOG_HTTP << "Server closed on us - closing all stuff.. ";
  connection_ = NULL;
  timeouter_.UnsetAllTimeouts();
  if ( conn_error_ == http::CONN_INCOMPLETE ||
       conn_error_ == http::CONN_OK ) {
    conn_error_ = http::CONN_CONNECTION_CLOSED;
  }
}

bool BaseClientProtocol::NotifyConnectionRead() {
  CHECK(current_request_ != NULL);
  if ( parser_.InFinalState() ) {
    LOG_HTTP << "Illegal data after complete response. Closing connection...";
    connection_->ForceClose();
    return true;
  }

  if ( params_->read_timeout_ms_ > 0 ) {
    timeouter_.SetTimeout(kWriteTimeout, params_->read_timeout_ms_);
  }
  do {
    parser_read_state_ = parser_.ParseServerReply(
        connection_->inbuf(), current_request_->request());
  } while ( parser_read_state_ & http::RequestParser::CONTINUE );
  if ( !parser_.InFinalState() ) {
    return true;
  }
  current_request_->set_error(CONN_OK);
  timeouter_.UnsetAllTimeouts();

  // We keep connection open anyway ..
  return true;    // we keep it open anyway..
}

void BaseClientProtocol::NotifyConnectionWrite() {
  available_output_size_ =
      (params_->max_output_buffer_size_ - connection_->outbuf()->Size());
  if ( available_output_size_ < 0 )
    available_output_size_ = 0;
  if ( params_->write_timeout_ms_ > 0 ) {
    if ( connection_->outbuf()->IsEmpty() ) {
      timeouter_.UnsetTimeout(kWriteTimeout);
    } else {
      timeouter_.SetTimeout(kWriteTimeout, params_->write_timeout_ms_);
    }
  }
}

void BaseClientProtocol::HandleTimeoutEvent(int64 timeout_id) {
  if ( HandleTimeout(timeout_id) ) {
    return;
  }
  LOG_HTTP << "Timeout encountered, id: " << timeout_id
           << " closing connection";
  connection_->ForceClose();
}

void BaseClientProtocol::PauseReading() {
  if ( connection_ != NULL ) {
    connection_->RequestReadEvents(false);
  }
}
void BaseClientProtocol::ResumeReading() {
  if ( connection_ != NULL && current_request_ != NULL ) {
    connection_->RequestReadEvents(true);
  }
}

void BaseClientProtocol::PauseWriting() {
  if ( connection_ != NULL ) {
    connection_->RequestWriteEvents(false);
  }
}
void BaseClientProtocol::ResumeWriting() {
  if ( connection_ != NULL && current_request_ != NULL ) {
    connection_->RequestWriteEvents(true);
  }
}


bool BaseClientProtocol::HandleTimeout(int64 timeout_id) {
  if ( timeout_id >= kRequestTimeout ) {
    conn_error_ = CONN_REQUEST_TIMEOUT;
  } else {
    switch ( timeout_id ) {
      case kConnectTimeout: conn_error_ = CONN_CONNECT_TIMEOUT; break;
      case kWriteTimeout:   conn_error_ = CONN_WRITE_TIMEOUT; break;
      case kReadTimeout:    conn_error_ = CONN_READ_TIMEOUT; break;
      default:              return true;
    }
  }
  LOG_HTTP << "HandleTimeout, timeout_id: " << timeout_id
           << ", conn_error_: " << conn_error_name();
  return false;
}

#ifndef WHISPER_DISABLE_STREAMING_HTTP_CLIENTS

//////////////////////////////////////////////////////////////////////
//
//  ClientStreamingProtocol
//
ClientStreamingProtocol::ClientStreamingProtocol(
    const ClientParams* params,
    http::BaseClientConnection* connection,
    net::HostPort server)
    : BaseClientProtocol(params, connection, server),
      source_stopped_(true),
      streaming_callback_(NULL) {
}

ClientStreamingProtocol::~ClientStreamingProtocol() {
  if ( current_request_ != NULL ) {
    if ( conn_error_ != CONN_INCOMPLETE ) {
      current_request_->set_error(conn_error_);
    } else {
      current_request_->set_error(CONN_CLIENT_CLOSE);
    }
    current_request_ = NULL;
  }
}

void ClientStreamingProtocol::BeginStreaming(
    ClientRequest* request,
    StreamingCallback* streaming_callback) {
  CHECK(current_request_ == NULL);
  parser_read_state_ = 0;
  parser_.Clear();
  source_stopped_ = false;
  current_request_ = request;
  streaming_callback_ = streaming_callback;
  CHECK(streaming_callback->is_permanent());
  if ( params_->connect_timeout_ms_ > 0 ) {
    timeouter_.SetTimeout(kConnectTimeout, params_->connect_timeout_ms_);
  }
  LOG_HTTP << " Connecting to " << server_;
  connection_->Connect(server_);
}

void ClientStreamingProtocol::NotifyConnectionWrite() {
  if ( source_stopped_ ) {
    return;   // nothing to do ..
  }
  DCHECK(current_request_ != NULL);
  BaseClientProtocol::NotifyConnectionWrite();
  if ( available_output_size_ > params_->max_output_buffer_size_ / 2 &&
       streaming_callback_ != NULL ) {
    source_stopped_ = !streaming_callback_->Run(available_output_size_);
  }
  if ( available_output_size_ > 0 &&
       !current_request_->request()->client_data()->IsEmpty() ) {
    if ( current_request_->is_pure_dumping() ) {
      if ( available_output_size_ <=
           current_request_->request()->client_data()->Size() ) {
        connection_->outbuf()->AppendStream(
            current_request_->request()->client_data());
      } else {
        connection_->outbuf()->AppendStreamNonDestructive(
            current_request_->request()->client_data(),
            available_output_size_);
        current_request_->request()->client_data()->Skip(
            available_output_size_);
      }
    } else {
      current_request_->request()->AppendClientChunk(
          connection_->outbuf(), params_->max_chunk_size_);
    }
    if ( params_->write_timeout_ms_ > 0 ) {
      timeouter_.SetTimeout(kWriteTimeout, params_->write_timeout_ms_);
    }
    // TODO(cpopescu): figure this out...
    // If chunked (or even unchunked) we need to disable the request timeout
    // as we expect to receive a stream
    // if (current_request_->request()->server_header()->IsChunkedTransfer()) {
    timeouter_.UnsetTimeout(kRequestTimeout + current_request_->request_id());
    // }

    connection_->NotifyWrite();
  }
}

bool ClientStreamingProtocol::NotifyConnectionRead() {
  bool ret = BaseClientProtocol::NotifyConnectionRead();
  if ( current_request_->is_finalized() ) {
    current_request_ = NULL;
    LOG_HTTP << " Request is finalized - informing the streamer.";
    if ( !source_stopped_ ) {
      source_stopped_ = !streaming_callback_->Run(-1);
    }
  }
  return ret && !source_stopped_;
}

void ClientStreamingProtocol::NotifyConnectionDeletion() {
  BaseClientProtocol::NotifyConnectionDeletion();
  if ( current_request_ ) {
    current_request_->set_error(conn_error_);
    if ( !source_stopped_ ) {
      LOG_HTTP << " Connection is finalized - informing the streamer.";
      source_stopped_ = !streaming_callback_->Run(-1);
    }
  }
}

//////////////////////////////////////////////////////////////////////
//
// ClientStreamReceiverProtocol
//
ClientStreamReceiverProtocol::ClientStreamReceiverProtocol(
    const ClientParams* params,
    http::BaseClientConnection* connection,
    net::HostPort server)
    : BaseClientProtocol(params, connection, server),
      streaming_callback_(NULL) {
}

ClientStreamReceiverProtocol::~ClientStreamReceiverProtocol() {
  if ( current_request_ != NULL ) {
    if ( conn_error_ != CONN_INCOMPLETE ) {
      current_request_->set_error(conn_error_);
    } else {
      current_request_->set_error(CONN_CLIENT_CLOSE);
    }
    current_request_->set_error(conn_error_);
    current_request_ = NULL;
  }
}

void ClientStreamReceiverProtocol::BeginStreamReceiving(
    ClientRequest* request,
    Closure* streaming_callback) {
  CHECK(streaming_callback->is_permanent());
  CHECK(current_request_ == NULL);
  current_request_ = request;
  streaming_callback_ = streaming_callback;
  CHECK(streaming_callback_->is_permanent());
  parser_read_state_ = 0;
  parser_.Clear();
  if ( params_->connect_timeout_ms_ > 0 ) {
    timeouter_.SetTimeout(kConnectTimeout, params_->connect_timeout_ms_);
  }
  LOG_HTTP << " Connecting to " << server_;
  connection_->Connect(server_);
}


bool ClientStreamReceiverProtocol::NotifyConnectionRead() {
  const bool ret = BaseClientProtocol::NotifyConnectionRead();
  if ( current_request_->is_finalized() ) {
    current_request_ = NULL;
    streaming_callback_->Run();
  } else if ( ((parser_read_state_ & http::RequestParser::HEADER_READ) ==
               http::RequestParser::HEADER_READ) ) {
    // If chunked (or even unchunked) we need to disable the request timeout
    // as we expect to receive a stream
    // if (current_request_->request()->server_header()->IsChunkedTransfer()) {
    timeouter_.UnsetTimeout(kRequestTimeout + current_request_->request_id());
    // }

    // Call for a new chunk of data - insure that the first happens
    // after the header is completed.
    streaming_callback_->Run();
  }
  return ret;
}

void ClientStreamReceiverProtocol::NotifyConnectionDeletion() {
  BaseClientProtocol::NotifyConnectionDeletion();
  if ( current_request_ ) {
    current_request_->set_error(conn_error_);
    current_request_ = NULL;
    streaming_callback_->Run();
  }
}

#endif   // WHISPER_DISABLE_STREAMING_HTTP_CLIENTS

//////////////////////////////////////////////////////////////////////
//
// ClientProtocol
//

ClientProtocol::ClientProtocol(
    const ClientParams* params,
    http::BaseClientConnection* connection,
    net::HostPort server)
    : BaseClientProtocol(params, connection, server),
      crt_id_(1LL),
      active_requests_(params->max_concurrent_requests_),
      callback_map_(params->max_concurrent_requests_ +
                    params->max_waiting_requests_),
      reading_request_(NULL) {
}

ClientProtocol::~ClientProtocol() {
  // Things get cleared via:
  // BaseClientProtocol::~BaseClientProtocol -->
  //     --> close connection -->
  //     --> NotifyConnectionDeletion
}

void ClientProtocol::SendRequest(
    ClientRequest* request, Closure* done_callback) {
  if ( connection_ == NULL ) {
    request->set_error(CONN_CONNECTION_CLOSED);
    done_callback->Run();
    return;
  }
  request->set_request_id(crt_id_++);
  if ( waiting_requests_.size() >= params_->max_waiting_requests_ ) {
    request->set_error(CONN_TOO_MANY_REQUESTS);
    connection_->selector()->RunInSelectLoop(done_callback);
    return;
  } else {
    callback_map_[request] = done_callback;
    waiting_requests_.push_back(request);
  }
  if ( connection_->state() == net::TcpConnection::DISCONNECTED ) {
    if ( params_->connect_timeout_ms_ > 0 ) {
      timeouter_.SetTimeout(kConnectTimeout,
                            params_->connect_timeout_ms_);
    }
    LOG_HTTP << " Connecting to " << server_;
    connection_->Connect(server_);
  } else {
    WriteRequestsToServer();
  }
}

//////////////////////////////////////////////////////////////////////

bool ClientProtocol::NotifyConnected() {
  // [COSMIN] Wouldn't it be nicer to call
  //          BaseClientProtocol::NotifyConnected ??
  conn_error_ = CONN_OK;
  timeouter_.UnsetTimeout(kConnectTimeout);
  WriteRequestsToServer();
  return true;
}

bool ClientProtocol::NotifyConnectionRead() {
  if ( reading_request_  == NULL ) {
    parser_read_state_ = 0;
    parser_.Clear();
    parser_.set_max_num_chunks(params_->max_num_chunks_);
    if ( params_->max_concurrent_requests_ == 1 &&
         !active_requests_.empty() ) {
      CHECK_EQ(active_requests_.size(), 1);
      reading_request_ = active_requests_.begin()->second;
    } else {
      reading_request_ = new ClientRequest();
    }
  } else {
    CHECK(!parser_.InFinalState());
  }

  do {
    parser_read_state_ = parser_.ParseServerReply(
        connection_->inbuf(), reading_request_->request());
    if ( ((parser_read_state_ & http::RequestParser::HEADER_READ) ==
          http::RequestParser::HEADER_READ) &&
         reading_request_->request_id() == 0 ) {
      IdentifyReadingRequest();
    }
  } while ( parser_read_state_ & http::RequestParser::CONTINUE );
  // We need more data (and no error happened)
  if ( !parser_.InFinalState() ) {
    return true;
  }
  LOG_HTTP << "Request finished in state: "
           << parser_.ParseStateName()
           << " req[ " << reading_request_->name() << " ]";
  reading_request_->set_error(
      parser_.InErrorState()
      ? http::CONN_HTTP_PARSING_ERROR
      : http::CONN_OK);
  http::Header::ParseError err =
      reading_request_->request()->server_header()->parse_error();
  if ( reading_request_->request_id() > 0 ) {
    timeouter_.UnsetTimeout(kRequestTimeout +
                            reading_request_->request_id());
    active_requests_.erase(reading_request_->request_id());
    const CallbackMap::iterator
        it_cb = callback_map_.find(reading_request_);
    if (it_cb != callback_map_.end()) {
        Closure* const closure = it_cb->second;
        callback_map_.erase(it_cb);
        closure->Run();
    }
  } else {
    LOG_HTTP << "Deleting an orphaned request: "
             << reading_request_->name();
    delete reading_request_;
    // We cannot trust this connection any more
    conn_error_ = CONN_DEPENDENCY_FAILURE;
    return false;
  }
  reading_request_ = NULL;
  if ( parser_.InErrorState() ) {
    LOG_WARNING << "HTTP[" << server_ << "]: "
                << "Exiting the connection due to broken parser state: "
                << parser_.ParseStateName()
                << " [ header parsing error: "
                << http::Header::ParseErrorName(err) << " ]";
    conn_error_ = CONN_DEPENDENCY_FAILURE;
    return false;
  }
  // Putting more requests on line
  WriteRequestsToServer();
  return true;
}

void ClientProtocol::NotifyConnectionDeletion() {
  BaseClientProtocol::NotifyConnectionDeletion();
  ResolveAllRequestsWithError();
}

bool ClientProtocol::HandleTimeout(int64 timeout_id) {
  if ( timeout_id >= kRequestTimeout ) {
    const int64 req_id = timeout_id - kRequestTimeout;
    const RequestMap::iterator it = active_requests_.find(req_id);
    if ( it != active_requests_.end() ) {
      const CallbackMap::iterator it_cb = callback_map_.find(it->second);
      CHECK(it_cb != callback_map_.end());
      Closure* const closure = it_cb->second;
      it->second->set_error(CONN_REQUEST_TIMEOUT);
      active_requests_.erase(it);
      callback_map_.erase(it_cb);
      closure->Run();
    }
    return true;
  }
  return BaseClientProtocol::HandleTimeout(timeout_id);
}

/////////////////////////////////////////////////////////////////////

bool ClientProtocol::IdentifyReadingRequest() {
  CHECK(reading_request_ != NULL);
  CHECK_EQ(reading_request_->request_id(), 0);
  Header* const hs = reading_request_->request()->server_header();
  const string req_id_str = hs->FindField(kHeaderXRequestId);

  int64 req_id = -1;
  if ( !req_id_str.empty() ) {
    char* endptr;
    errno = 0;  // essential
    req_id = strtoll(req_id_str.c_str(), &endptr, 10);
    if ( errno || endptr == NULL || *endptr != '\0' ) {
      req_id = -1;
    }
  } else if ( params_->max_concurrent_requests_ == 1 && current_request_ != NULL ) {
    req_id = current_request_->request_id();
  }

  if (req_id != -1) {
      const RequestMap::const_iterator it = active_requests_.find(req_id);
      if ( it == active_requests_.end() ) {
        LOG_WARNING  << "HTTP[" << server_ << "]: "
                     << " Orphaned response received from the server:\n"
                     << hs->ToString();
        reading_request_->set_request_id(-1);
      } else {
        Header* const hs2 = it->second->request()->server_header();
        hs2->CopyHeaders(*hs, true);
        hs2->set_http_version(hs->http_version());
        hs2->set_status_code(hs->status_code());
        hs2->set_reason(hs->reason());
        hs2->set_first_line_type(hs->first_line_type());
        it->second->request()->server_data()->AppendStream(
            reading_request_->request()->server_data());
        delete reading_request_;
        reading_request_ = it->second;
      }
  } else {
    LOG_WARNING << "HTTP[" << server_ << "]: "
                << " - Expecting a request-id header from "
                << " the server and got noting (header:\n"
                << hs->ToString();
    reading_request_->set_request_id(-1);
  }
  CHECK_NE(reading_request_->request_id(), 0);
  return reading_request_->request_id() > 0;
}

void ClientProtocol::ResolveAllRequestsWithError() {
  timeouter_.UnsetAllTimeouts();
  vector<Closure*> to_resolve;
  LOG_HTTP << "Resolving all pending requests [active:"
           << active_requests_.size()
           << " waiting:" << waiting_requests_.size()
           << " with error: "
           << http::ClientErrorName(conn_error_);
  if ( !active_requests_.empty() ) {
    // CHECK_NE(conn_error_, http::CONN_INCOMPLETE);
    //   Can actually happen when requested
    for ( RequestMap::const_iterator it = active_requests_.begin();
          it != active_requests_.end(); ++it ) {
      it->second->set_error(conn_error_);
      to_resolve.push_back(callback_map_[it->second]);
    }
  }
  for ( RequestsQueue::const_iterator it = waiting_requests_.begin();
        it != waiting_requests_.end(); ++it ) {
    (*it)->set_error(http::CONN_DEPENDENCY_FAILURE);
    to_resolve.push_back(callback_map_[*it]);
  }
  waiting_requests_.clear();
  active_requests_.clear();
  callback_map_.clear();
  for ( int i = 0; i < to_resolve.size(); ++i ) {
    to_resolve[i]->Run();
  }
}

void ClientProtocol::WriteRequestsToServer() {
  while ( !waiting_requests_.empty() &&
          active_requests_.size() < params_->max_concurrent_requests_ ) {
    current_request_ = waiting_requests_.front();
    waiting_requests_.pop_front();

    Header* const hc = current_request_->request()->client_header();
    CHECK(current_request_ != NULL);
    hc->AddField(
        kHeaderXRequestId,
        strutil::StringPrintf("%" PRId64 "", current_request_->request_id()),
        true);
    // TODO(cpopescu): figure out timeouts !!
    CHECK(!hc->IsChunkedTransfer())
        << " No chuncked transer encoding for client requests "
        << " w/ ClientProtocol please.";
    LOG_HTTP << " Sending request to server: "
             << current_request_->name();
    CHECK(active_requests_.insert(
              make_pair(current_request_->request_id(),
                        current_request_)).second);
    SendRequestToServer(current_request_);
    current_request_ = NULL;
  }
  LOG_HTTP << " WriteRequestsToServer ended with: " << active_requests_.size()
           << " waiting and " << waiting_requests_.size() << " active. ";
}

string ClientProtocol::StatusString() const {
    string s;
    s += "\n\n ========>  Waiting requests: ";
    for (int i = 0; i < waiting_requests_.size(); ++i) {
        s += "\n";
        s += waiting_requests_[i]->name();
    }
    s += "\n ========> Active requests requests: ";
    for (RequestMap::const_iterator it = active_requests_.begin();
         it != active_requests_.end(); ++it) {
        s += "\n";
        s += it->second->name();
    }
    s += "\n";
    return s;
}

//////////////////////////////////////////////////////////////////////

const char* ClientErrorName(ClientError err) {
  switch ( err ) {
    CONSIDER(CONN_INCOMPLETE);
    CONSIDER(CONN_OK);
    CONSIDER(CONN_CONNECT_ERROR);
    CONSIDER(CONN_CONNECT_TIMEOUT);
    CONSIDER(CONN_WRITE_TIMEOUT);
    CONSIDER(CONN_READ_TIMEOUT);
    CONSIDER(CONN_CONNECTION_CLOSED);
    CONSIDER(CONN_REQUEST_TIMEOUT);
    CONSIDER(CONN_DEPENDENCY_FAILURE);
    CONSIDER(CONN_TOO_MANY_REQUESTS);
    CONSIDER(CONN_HTTP_PARSING_ERROR);
    CONSIDER(CONN_CLIENT_CLOSE);
    CONSIDER(CONN_TOO_MANY_RETRIES);
  }
  return "UNKNOWN";
}
}
