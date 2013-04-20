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

#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/io/ioutil.h"

#define LOG_HTTP LOG_INFO_IF(dlog_level_) << name() << ": "

namespace http {

ServerParams::ServerParams()
    : root_url_("http://localhost/"),
      dlog_level_(false),
      strict_request_headers_(true),
      allow_all_methods_(false),
      max_header_size_(4096),
      max_body_size_(1 << 20),
      max_chunk_size_(1 << 18),
      max_num_chunks_(20),
      worst_accepted_header_error_(Header::READ_NO_STATUS_REASON),
      max_concurrent_connections_(800),
      max_concurrent_requests_(10000),
      max_concurrent_requests_per_connection_(10),
      request_read_timeout_ms_(10000),
      keep_alive_timeout_sec_(15),
      reply_write_timeout_ms_(20000),
      max_reply_buffer_size_(1 << 20),
      reply_full_buffer_policy_(ServerParams::POLICY_CLOSE),
      default_content_type_("text/html; charset=UTF-8"),
      ignore_different_http_hosts_(true) {
}

ServerParams::ServerParams(
    const string& root_url,
    bool dlog_level,
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
    bool ignore_different_http_hosts)
    : root_url_(root_url),
      dlog_level_(dlog_level),
      strict_request_headers_(strict_request_headers),
      allow_all_methods_(allow_all_methods),
      max_header_size_(max_header_size),
      max_body_size_(max_body_size),
      max_chunk_size_(max_chunk_size),
      max_num_chunks_(max_num_chunks),
      worst_accepted_header_error_(worst_accepted_header_error),
      max_concurrent_connections_(max_concurrent_connections),
      max_concurrent_requests_(max_concurrent_requests),
      max_concurrent_requests_per_connection_(
          max_concurrent_requests_per_connection),
      request_read_timeout_ms_(request_read_timeout_ms),
      keep_alive_timeout_sec_(keep_alive_timeout_sec),
      reply_write_timeout_ms_(reply_write_timeout_ms),
      max_reply_buffer_size_(max_reply_buffer_size),
      reply_full_buffer_policy_(reply_full_buffer_policy),
      default_content_type_(default_content_type),
      ignore_different_http_hosts_(ignore_different_http_hosts) {
}


//////////////////////////////////////////////////////////////////////
//
// http::ServerAcceptor
//
ServerAcceptor::ServerAcceptor(net::Selector* selector,
                               const net::NetFactory& net_factory,
                               net::PROTOCOL net_protocol,
                               const net::HostPort& local_addr,
                               http::Server* server)
    : selector_(selector),
      acceptor_(net_factory.CreateAcceptor(net_protocol)),
      local_addr_(local_addr),
      server_(server) {
  acceptor_->SetFilterHandler(NewPermanentCallback(
      this, &ServerAcceptor::AcceptorFilterHandler), true);
  acceptor_->SetAcceptHandler(NewPermanentCallback(
      this, &ServerAcceptor::AcceptorAcceptHandler), true);
}

ServerAcceptor::~ServerAcceptor() {
  delete acceptor_;
  acceptor_ = NULL;
}

bool ServerAcceptor::Listen() {
  return acceptor_->Listen(local_addr_);
}
void ServerAcceptor::Close() {
  acceptor_->Close();
}

bool ServerAcceptor::AcceptorFilterHandler(const net::HostPort& peer_address) {
  VLOG(1) << acceptor_->PrefixInfo()
          << "Filtering an accept from: " << peer_address
          << " with: " << server_->num_connections() << " active connections"
          << " and a max of: "
          << server_->protocol_params().max_concurrent_connections_;
  if ( server_->num_connections() >=
       server_->protocol_params().max_concurrent_connections_ ) {
    LOG_ERROR << acceptor_->PrefixInfo() << "Too many connections ! - refusing";
    return false;
  }
  return true;
}
void ServerAcceptor::AcceptorAcceptHandler(net::NetConnection* net_connection) {
  http::ServerProtocol* const protocol =
      new ServerProtocol(selector_,
                         net_connection->net_selector(),
                         server_);
  ServerConnection* const http_connection = new ServerConnection(
      net_connection->net_selector(), net_connection, protocol);
  protocol->set_connection(http_connection);
  server_->AddClient(protocol);
}

ServerConnection::ServerConnection(net::Selector* selector,
                                           net::NetConnection* net_connection,
                                           http::ServerProtocol* protocol)
    : selector_(selector),
      net_connection_(net_connection),
      protocol_(protocol) {
  net_connection_->SetReadHandler(NewPermanentCallback(
      this, &ServerConnection::ConnectionReadHandler), true);
  net_connection_->SetWriteHandler(NewPermanentCallback(
      this, &ServerConnection::ConnectionWriteHandler), true);
  net_connection_->SetCloseHandler(NewPermanentCallback(
      this, &ServerConnection::ConnectionCloseHandler), true);
}
ServerConnection::~ServerConnection() {
  delete net_connection_;
  net_connection_ = NULL;
}
void ServerConnection::NotifyWrite() {
  CHECK(selector_->IsInSelectThread());
  net_connection_->RequestWriteEvents(true);
}
bool ServerConnection::ConnectionReadHandler() {
  CHECK(selector_->IsInSelectThread());
  if ( !protocol_->ProcessMoreData() ) {
    ForceClose();
    return false;
  }
  return true;
}
bool ServerConnection::ConnectionWriteHandler() {
  CHECK(selector_->IsInSelectThread());
  protocol_->NotifyConnectionWrite();
  return true;
}
void ServerConnection::ConnectionCloseHandler(
    int err, net::NetConnection::CloseWhat what) {
  CHECK(selector_->IsInSelectThread());
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    net_connection_->FlushAndClose();
    return;
  }
  // We have to call NotifyConnectionDeletion here because we need to stick to
  // this original pattern:
  //   [protocol] calls
  //     -> ServerConnection::ForceClose
  //        ->ServerConnection::ConnectionCloseHandler
  //          ->[procotol]::NotifyConnectionDeletion
  // When ForceClose completes the NotifyConnectionDeletion must be already run.
  if ( protocol_ != NULL ) {
    protocol_->NotifyConnectionDeletion();
    protocol_ = NULL;
  }
  VLOG(1) << "HTTP ServerConnection completely closed,"
      " deleting in select loop..";
  selector_->DeleteInSelectLoop(this);
}

//////////////////////////////////////////////////////////////////////
//
// http::Server
//

Server::Server(const char* name,
               net::Selector* selector,
               const net::NetFactory& net_factory,
               const ServerParams& protocol_params)
    : name_(name),
      selector_(selector),
      net_factory_(net_factory),
      protocol_params_(protocol_params),
      acceptors_(),
      default_processor_(NewPermanentCallback(
                             this, &Server::DefaultRequestProcessor)),
      error_processor_(NewPermanentCallback(
                           this, &Server::ErrorRequestProcessor)) {
}

Server::~Server() {
  for ( uint32 i = 0; i < acceptors_.size(); i++ ) {
    delete acceptors_[i];
  }
  acceptors_.clear();

  // TODO(cpopescu): force close all -
  //                 see how this interacts w/ processor stuff..
  // CHECK(!protocols_.empty());
  //
  synch::MutexLocker l(&mutex_);
  while ( !processors_.empty() ) {
    delete processors_.begin()->second;
    processors_.erase(processors_.begin());
  }
}

void Server::AddAcceptor(net::PROTOCOL net_protocol,
                         const net::HostPort& local_address) {
  acceptors_.push_back(new ServerAcceptor(selector_,
                                          net_factory_,
                                          net_protocol,
                                          local_address,
                                          this));
}
void Server::StartServing() {
  for ( uint32 i = 0; i < acceptors_.size(); i++ ) {
    ServerAcceptor& acceptor = *acceptors_[i];
    CHECK(acceptor.Listen());
    LOG_INFO << "HTTP Turned on listening on: " << acceptor.local_addr();
  }
}
void Server::StopServing() {
  for ( uint32 i = 0; i < acceptors_.size(); i++ ) {
    ServerAcceptor& acceptor = *acceptors_[i];
    acceptor.Close();
  }
}

void Server::RegisterProcessor(const string& path,
                               ServerCallback* callback,
                               bool is_public,
                               bool auto_del_callback) {
  CHECK_NOT_NULL(callback);
  CHECK(callback->is_permanent());
  const string reg_path(strutil::NormalizeUrlPath(path));
  synch::MutexLocker l(&mutex_);
  const ProcessorMap::iterator it = processors_.find(reg_path);
  if ( it != processors_.end() ) {
    delete it->second;
    LOG_INFO << "HTTP processor replaced for path: " << reg_path;
    it->second = new Processor(callback, auto_del_callback);
  } else {
    LOG_INFO << "HTTP listening on path: " << reg_path;
    processors_[reg_path] = new Processor(callback, auto_del_callback);
  }
  if ( is_public ) {
    allowed_ips_[reg_path] = NULL;  // all alowed
  }
}

void Server::UnregisterProcessor(const string& path) {
  const string reg_path(strutil::NormalizeUrlPath(path));
  const ProcessorMap::iterator it = processors_.find(reg_path);
  if ( it == processors_.end() ) {
    LOG_INFO << "No HTTP processor found to be deleted for path: " << reg_path;
    return;
  }
  delete it->second;
  processors_.erase(it);
  allowed_ips_.erase(reg_path);
}

void Server::RegisterAllowedIpAddresses(const string& path,
                                        const net::IpV4Filter* ips) {
  const string reg_path(strutil::NormalizeUrlPath(path));
  synch::MutexLocker l(&mutex_);
  const AllowedIpsMap::iterator it_ips = allowed_ips_.find(reg_path);
  if ( it_ips != allowed_ips_.end() ) {
    it_ips->second = ips;
  } else {
    allowed_ips_[reg_path] = ips;
  }
}

void Server::RegisterClientStreaming(const string& path,
                                     bool is_client_streaming) {
  const string reg_path(strutil::NormalizeUrlPath(path));
  synch::MutexLocker l(&mutex_);
  is_streaming_client_map_[reg_path] = is_client_streaming;
}

void Server::AddClient(ServerProtocol* proto) {
  LOG_INFO << name() << " HTTP Adding client: " << proto->name()
           << " to: " << protocols_.size() << " clients already serving";
  synch::MutexLocker l(&mutex_);
  const ProtocolSet::const_iterator it = protocols_.find(proto);
  CHECK(it == protocols_.end());
  protocols_.insert(proto);
}

void Server::DeleteClient(ServerProtocol* proto) {
  synch::MutexLocker l(&mutex_);
  VLOG(1) << name() << " HTTP Deleting client: " << proto->name()
          << " from: " << protocols_.size() << " clients already serving";
  CHECK(protocols_.erase(proto));
}

void Server::DefaultRequestProcessor(ServerRequest* req) {
  req->request()->server_data()->Write(
      "<h1>404 Not Found</h1>"
      "The server has not found anything matching the Request-URI.");
  req->ReplyWithStatus(http::NOT_FOUND);
}

void Server::ErrorRequestProcessor(ServerRequest* req) {
  // See if we already got a status - else set a 500
  http::HttpReturnCode code = req->request()->server_header()->status_code();
  if ( code == http::UNKNOWN ) {
    code = http::INTERNAL_SERVER_ERROR;
  }
  req->request()->server_data()->Write(
      strutil::StringPrintf("<h1>%d %s</h1>",
                            static_cast<int>(code),
                            GetHttpReturnCodeDescription(code)));
  req->ReplyWithStatus(code);
}


//////////////////////////////////////////////////////////////////////
//
// some utilitities:
namespace {
static bool VerifyHost(http::Header* client_header, URL* url) {
  string host;
  if ( client_header->FindField(kHeaderHost, &host) ) {
    if ( url->has_host() &&
         !strutil::StrCaseEqual(host, url->host()) ) {
      return false;
    }
  }
  return true;
}
}

void Server::GetSpecificProtocolParams(http::ServerRequest* req) {
  synch::MutexLocker l(&mutex_);
  req->request()->InitializeUrlFromClientRequest(protocol_params_.root_url_);
  URL* const url = req->request()->url();
  if ( url != NULL ) {
    string url_path(url->UrlUnescape(url->path().c_str(),
                                     url->path().size()));
    req->is_client_streaming_ = io::FindPathBased(&is_streaming_client_map_,
                                                  url_path);
  }
  req->is_initialized_ = true;
}

void Server::ProcessRequest(http::ServerRequest* req) {
  if ( req->server_callback_ != NULL ) {
    req->server_callback_->Run(req);
    return;
  }

  Header* const hc = req->request()->client_header();
  Header* const hs = req->request()->server_header();
  URL* const url = req->request()->url();

  // Should come from a registered protocol
  // DCHECK(protocols_.find(req->protocol()) != protocols_.end());

  // See if we already got a status set - and reply
  if ( hs->status_code() == http::UNKNOWN ) {
    if ( hc->http_version() != VERSION_1_1 &&
         hc->http_version() != VERSION_1_0 ) {
      // Bad version
      hs->set_status_code(HTTP_VERSION_NOT_SUPPORTED);
    } else if ( hc->method() == METHOD_UNKNOWN ||
                url == NULL || !url->is_valid() ) {
      // Bad request
      hs->set_status_code(BAD_REQUEST);
    } else if ( !protocol_params_.allow_all_methods_ &&
                hc->method() != METHOD_GET &&
                hc->method() != METHOD_HEAD &&
                hc->method() != METHOD_POST ) {
      // Bad method.
      hs->set_status_code(METHOD_NOT_ALLOWED);
    } else if ( !protocol_params_.ignore_different_http_hosts_ &&
                !VerifyHost(hc, url) ) {
      hs->set_status_code(BAD_REQUEST);
    }
  }
  if ( hs->status_code() != http::UNKNOWN ) {
    error_processor_->Run(req);
    return;
  }

  // Accepted request - looks OK !
  string url_path(url->UrlUnescape(url->path().c_str(),
                                   url->path().size()));
  string url_path2(url_path);

  synch::MutexLocker l(&mutex_);
  Processor* const proc = io::FindPathBased(&processors_, url_path);
  if ( proc == NULL ) {
    LOG_ERROR << "Cannot find a processor for path: [" << url_path << "]"
                 ", looking through: " << strutil::ToStringKeys(processors_);
    req->server_callback_ = default_processor_;
  } else {
    // Check if the ip is authorized
    const net::IpV4Filter* const
        ipfilter = io::FindPathBased(&allowed_ips_, url_path2);
    if ( ipfilter != NULL &&
         !ipfilter->Matches(
             net::IpAddress(
                 req->protocol()->remote_address().ip_object())) ) {
      hs->set_status_code(FORBIDDEN);
      req->server_callback_ = error_processor_;
    } else {
      req->server_callback_ = proc->callback_;
    }
  }
  req->server_callback_->Run(req);
}


//////////////////////////////////////////////////////////////////////
//
// http::ServerProtocol
//

ServerProtocol::ServerProtocol(net::Selector* server_selector,
                               net::Selector* net_selector,
                               Server* server)
    : dlog_level_(server->protocol_params().dlog_level_),
      server_selector_(server_selector),
      net_selector_(net_selector),
      server_(server),
      timeouter_(net_selector, NewPermanentCallback(
                     this, &ServerProtocol::HandleTimeout)),
      parser_("",
              server->protocol_params().max_header_size_,
              server->protocol_params().max_body_size_,
              server->protocol_params().max_chunk_size_,
              server->protocol_params().max_num_chunks_,
              false, false,
              server->protocol_params().worst_accepted_header_error_),
      connection_(NULL),
      crt_recv_(NULL),
      crt_send_(NULL),
      closed_(false) {
}

ServerProtocol::~ServerProtocol() {
  CHECK(net_selector()->IsInSelectThread());
  CHECK(active_requests_.empty());
  CHECK(connection_ == NULL);
  timeouter_.UnsetAllTimeouts();
  delete crt_recv_;
  CHECK(crt_send_ == NULL);
}

void ServerProtocol::set_connection(ServerConnection* conn) {
  CHECK(net_selector()->IsInSelectThread());
  CHECK(connection_ == NULL);
  connection_ = conn;
  local_address_ = conn->local_address();
  remote_address_ = conn->remote_address();
  name_ = strutil::StringPrintf(
      "HTTP[%d](%s:%d)",
      static_cast<int>(conn->local_address().port()),
      conn->remote_address().ip_object().ToString().c_str(),
      static_cast<int>(conn->remote_address().port()));
  parser_.set_name(name_);
  timeouter_.SetTimeout(kRequestTimeout,
                        protocol_params().request_read_timeout_ms_);
}

net::NetConnection* ServerProtocol::DetachFromFd(ServerRequest* req) {
  CHECK(net_selector()->IsInSelectThread());
  LOG_INFO << name() << " - Detaching FD.";
  CHECK(req->protocol() == this);
  active_requests_.erase(req);
  CHECK(active_requests_.empty())
      << "Cannot detach if you accept multiple concurrent requests!";
  net::NetConnection * net_connection = connection_->DetachFromFd();
  req->ResetProtocol();
  NotifyConnectionDeletion();
  return net_connection;
}

void ServerProtocol::NotifyConnectionDeletion() {
  CHECK(net_selector()->IsInSelectThread());
  VLOG(1) << "Client closed on us - closing all stuff.. ";
  connection_ = NULL;
  closed_ = true;
  timeouter_.UnsetAllTimeouts();
  server_->DeleteClient(this);
  if ( active_requests_.empty() ) {
    // LOG_HTTP << "HTTP :" << name()
    //         << " No active requests - deleting client";
    LOG_HTTP << " No active requests - deleting client";
    net_selector_->DeleteInSelectLoop(this);
  } else {
    LOG_INFO << "HTTP :" << name()
             << " Waking all " << active_requests_.size()
             << " pending requests.";
    net_selector_->RunInSelectLoop(
        NewCallback(this, &ServerProtocol::CloseAllActiveRequests));
  }
}

void ServerProtocol::CloseAllActiveRequests() {
  CHECK(net_selector()->IsInSelectThread());
  CHECK(connection_ == NULL);
  vector<http::ServerRequest*> reqs;
  for ( RequestSet::const_iterator it = active_requests_.begin();
        it != active_requests_.end(); ++it ) {
    reqs.push_back(*it);
  }
  for ( int i = 0; i < reqs.size(); ++i ) {
    reqs[i]->SignalClosed();
  }
  if ( !active_requests_.empty() ) {
    LOG_ERROR << " WARNING : " << active_requests_.size()
              << " requests did not close correctly"
              << " so far -- keep an eye if they save it somewhere";
  }
}

void ServerProtocol::HandleTimeout(int64 timeout_id) {
  LOG_HTTP << "Timeout encountered - closing: " << timeout_id;
  if ( connection_ ) {
    connection_->ForceClose();
    connection_ = NULL;
  }
}

bool ServerProtocol::ProcessMoreData() {
  CHECK(net_selector()->IsInSelectThread());
  if ( closed_ ) {
    connection_->inbuf()->Clear();
    return true;   // 'soft' closed ..
  }
  if ( crt_recv_ == NULL ) {
    parser_.Clear();
    parser_.set_max_num_chunks(protocol_params().max_num_chunks_);
    crt_recv_ = new ServerRequest(this);
  } else {
    CHECK(!parser_.InFinalState());
  }
  int read_state = 0;
  do {
    read_state = parser_.ParseClientRequest(connection_->inbuf(),
                                            crt_recv_->request());
    LOG_HTTP << "In state: " << read_state << " - "
             << parser_.ParseStateName();
    crt_recv_->is_parsing_finished_ = parser_.InFinalState();

    if ( read_state & http::RequestParser::HEADER_READ ) {
      if ( !crt_recv_->is_initialized_ ) {
        server_->GetSpecificProtocolParams(crt_recv_);
      }

      if ( !parser_.InFinalState() && crt_recv_->is_client_streaming() ) {
        parser_.set_max_num_chunks(-1);
        parser_.set_max_body_size(-1);
        active_requests_.insert(crt_recv_);
        server_->ProcessRequest(crt_recv_);
        if ( crt_recv_ == NULL || crt_recv_->server_callback_ == NULL ) {
          closed_ = true;
          return false;
        }
        timeouter_.UnsetTimeout(kRequestTimeout);
      }
    }
    if ( parser_.InFinalState() ) {
      break;
    }
  } while ( read_state & http::RequestParser::CONTINUE );

  // We need more data (and no error happened)
  if ( !parser_.InFinalState() ) {
    return true;
  }
  timeouter_.UnsetTimeout(kRequestTimeout);
  if ( parser_.InErrorState() ) {
    LOG_HTTP << net_selector()
             << "Fully parsed an error request " << parser_.ParseStateName();
    PrepareErrorRequest(crt_recv_);
    closed_ = true;
    return true;
  } else {
    LOG_HTTP << "Fully parsed an OK request ["
             << strutil::StrTrim(
                 crt_recv_->request()->client_header()->ComposeFirstLine())
             << "]";
  }
  active_requests_.insert(crt_recv_);
  if ( active_requests_.size() >
       protocol_params().max_concurrent_requests_per_connection_ ) {
      crt_recv_->request()->server_header()->PrepareStatusLine(
          SERVICE_UNAVAILABLE);
      crt_recv_->request()->server_data()->Write(
          "Too many concurrent requests.");
  }
  server_->ProcessRequest(crt_recv_);
  crt_recv_ = NULL;
  return true;
}

void ServerProtocol::PrepareErrorRequest(ServerRequest* req) {
  CHECK(parser_.InErrorState());
  HttpReturnCode code = UNKNOWN;
  switch ( parser_.parse_state() ) {
    case RequestParser::ERROR_HEADER_TOO_LONG:
    case RequestParser::ERROR_CHUNK_HEADER_TOO_LONG:
      code = http::REQUEST_URI_TOO_LARGE;
      break;
    case RequestParser::ERROR_CONTENT_TOO_LONG:
    case RequestParser::ERROR_CONTENT_GZIP_TOO_LONG:
    case RequestParser::ERROR_CHUNK_TOO_LONG:
    case RequestParser::ERROR_CHUNK_TOO_MANY:
    case RequestParser::ERROR_CHUNK_CONTENT_GZIP_TOO_LONG:
    case RequestParser::ERROR_CHUNK_TRAILER_TOO_LONG:
      code = http::REQUEST_ENTITY_TOO_LARGE;
      break;
    case RequestParser::ERROR_HEADER_BAD:
    case RequestParser::ERROR_HEADER_LINE:
    case RequestParser::ERROR_CONTENT_GZIP_ERROR:
    case RequestParser::ERROR_CONTENT_GZIP_UNFINISHED:
    case RequestParser::ERROR_CHUNK_TRAIL_HEADER:
    case RequestParser::ERROR_CHUNK_BAD_CHUNK_TERMINATION:
    case RequestParser::ERROR_CHUNK_BIGGER_THEN_DECLARED:
    case RequestParser::ERROR_CHUNK_UNFINISHED_GZIP_CONTENT:
    case RequestParser::ERROR_CHUNK_CONTINUED_FINISHED_GZIP_CONTENT:
    case RequestParser::ERROR_CHUNK_CONTENT_GZIP_ERROR:
      code = http::BAD_REQUEST;
      break;
    case RequestParser::ERROR_TRANSFER_ENCODING_UNKNOWN:
    case RequestParser::ERROR_CONTENT_ENCODING_UNKNOWN:
      code = http::INTERNAL_SERVER_ERROR;
      break;
    case RequestParser::ERROR_CHUNK_BAD_CHUNK_LENGTH:
    case RequestParser::ERROR_HEADER_BAD_CONTENT_LEN:
      code = http::LENGTH_REQUIRED;
      break;
    default:
      code = http::INTERNAL_SERVER_ERROR;
  }
  req->request()->server_header()->PrepareStatusLine(code);
  req->request()->server_data()->Write(
      strutil::StringPrintf("<p>Request Error: %d [detail: %s].</p>",
                            static_cast<int>(code),
                            parser_.ParseStateName()));
  req->ReplyWithStatus(code);
}


bool ServerProtocol::PrepareResponse(ServerRequest* req,
                                     HttpReturnCode status) {
  CHECK(net_selector()->IsInSelectThread());
  if ( crt_send_ == NULL ) {
    crt_send_ = req;
  }
  bool should_close = false;
  if ( !parser_.InFinalState() ) {
    should_close = true;
  }
  // We never orhpan a connection here - causes loads of trouble ..
  if ( connection_ == NULL )
    return true;
  // Prepare the reply parameters
  http::Header* const hs = req->request()->server_header();  // shortcut
  hs->PrepareStatusLine(
      status, req->request()->client_header()->http_version());
  hs->SetChunkedTransfer(
      req->request()->client_header()->http_version() >= VERSION_1_1 &&
      req->is_server_streaming() &&
      req->is_server_streaming_chunks());

  // Set some necessary headers *if not set*
  if ( !hs->HasField(http::kHeaderDate) ) {
    time_t crt_time;
    hs->SetDateField(http::kHeaderDate, time(&crt_time));
  }
  if ( !hs->HasField(http::kHeaderServer) ) {
    hs->AddField(http::kHeaderServer, server_->name(), true);
  }
  if ( !hs->HasField(http::kHeaderContentType) ) {
    hs->AddField(http::kHeaderContentType,
                 protocol_params().default_content_type_,
                 true);
  }
  // Determine keep-alive stuff
  if ( protocol_params().keep_alive_timeout_sec_ > 0 &&
       crt_send_->request()->client_header()->HasField(
           http::kHeaderKeepAlive) ) {
    hs->AddField(http::kHeaderConnection, "Keep-Alive", true);
    hs->AddField(
        http::kHeaderKeepAlive,
        strutil::IntToString(protocol_params().keep_alive_timeout_sec_),
        true);
    crt_send_->is_keep_alive_ = true;
  } else {
    hs->AddField(http::kHeaderConnection, "Close", true);
    crt_send_->is_keep_alive_ = false;
  }
  // Write the data out
  if ( connection_->outbuf()->Size() >
       protocol_params().max_reply_buffer_size_ ) {
    switch ( protocol_params().reply_full_buffer_policy_ ) {
      case ServerParams::POLICY_CLOSE:
        LOG_INFO << name() << ": connection outbuf full -> Closing";
        should_close = true;
        crt_send_->is_keep_alive_ = false;
        break;
      case ServerParams::POLICY_DROP_OLD_DATA:
        LOG_INFO << name() << ": connection outbuf full -> Dropping old data";
        connection_->outbuf()->Clear();
        crt_send_->request()->AppendServerReply(
            connection_->outbuf(),
            req->is_server_streaming(),
            req->is_server_streaming_chunks());
        break;
      case ServerParams::POLICY_DROP_NEW_DATA:
        LOG_INFO << name() << ": connection outbuf full -> Dropping new data";
        break;
    }
  } else {
    crt_send_->request()->AppendServerReply(
        connection_->outbuf(),
        req->is_server_streaming(),
        req->is_server_streaming_chunks());
  }
  return should_close;
}

void ServerProtocol::ReplyForRequest(ServerRequest* req,
                                     HttpReturnCode status) {
  CHECK(net_selector()->IsInSelectThread());
  bool should_close = false;
  if ( crt_send_ != NULL ) {
    LOG_INFO << "Dumping invalid request received : " << req;
    // TODO(miancule): This seems to fix the persistent HTTP bugs,
    // but I'm pretty sure that this is somehow incorrect...
    if ( req == crt_send_ ) {
      crt_send_ = NULL;
    }
    net_selector_->DeleteInSelectLoop(req);
    return;
  }
  crt_send_ = req;
  if ( connection_ == NULL ) {
    // Orphaned request:
    crt_send_->is_orphaned_ = true;
    should_close = true;
  } else {
    should_close = PrepareResponse(req, status);
  }
  EndRequestProcessing(crt_send_,
                       !req->is_server_streaming() || should_close);
}

void ServerProtocol::StreamData(ServerRequest* req, bool is_eos) {
  CHECK(net_selector()->IsInSelectThread());
  CHECK_EQ(crt_send_, req);
  if ( connection_ == NULL ) {
    // Orphaned request:
    req->is_orphaned_ = true;
  } else {
    // We need this protection to insure that we don't output empty chunks
    // (which signal EOS) when we don't want. Because of multithreading,
    // two calls to ContinueStreamingData from a worker thread
    // can determine two calls to this function. In the first we output all,
    // while in the second we will have nothing to output..
    if ( !req->request()->server_data()->IsEmpty() ) {
      // Write the data out
      if ( connection_->outbuf()->Size() >
           protocol_params().max_reply_buffer_size_ ) {
        LOG_WARNING << "HTTP outbuf size exceeded: "
                    << connection_->outbuf()->Size()
                    << " > max_reply_buffer_size: "
                    << protocol_params().max_reply_buffer_size_;
        switch ( protocol_params().reply_full_buffer_policy_ ) {
          case ServerParams::POLICY_CLOSE:
            LOG_HTTP << "Buffer full - closing connection.";
            is_eos = true;
            req->is_keep_alive_ = false;
            break;
          case ServerParams::POLICY_DROP_OLD_DATA:
            LOG_HTTP << "Buffer full - dropping "
                     << connection_->outbuf()->Size()
                     << " bytes from connection.";
            connection_->outbuf()->Clear();
            req->request()->AppendServerChunk(
                connection_->outbuf(), req->is_server_streaming_chunks());
            break;
          case ServerParams::POLICY_DROP_NEW_DATA:
            break;
        }
      } else {
        req->request()->AppendServerChunk(
            connection_->outbuf(), req->is_server_streaming_chunks());
      }
      connection_->NotifyWrite();
    }
    if ( is_eos ) {
      // The finishing touch
      req->request()->AppendServerChunk(
          connection_->outbuf(), req->is_server_streaming_chunks());
      connection_->NotifyWrite();
    }
  }
  EndRequestProcessing(req, is_eos);
}

void ServerProtocol::EndRequestProcessing(ServerRequest* req, bool is_eos) {
  CHECK(net_selector()->IsInSelectThread());
  CHECK(crt_send_ == req);
  if ( is_eos ) {
    crt_send_ = NULL;
  }
  const bool should_close = is_eos && (!req->is_keep_alive() ||
                                       req->is_orphaned());
  if ( connection_ != NULL ) {
    if ( !connection_->outbuf()->IsEmpty() ) {
      timeouter_.SetTimeout(kWriteTimeout,
                            protocol_params().reply_write_timeout_ms_);
      connection_->NotifyWrite();
    }
  }
  bool signal_ready = false;
  if ( is_eos || req->is_orphaned() ) {
    req->SignalClosed();
  } else if ( connection_->outbuf()->IsEmpty() ) {
    signal_ready = true;
  }

  if ( is_eos ) {
    LOG_HTTP
        << "Request completed: ["
        << strutil::StrTrim(
            req->request()->client_header()->ComposeFirstLine()) << "] => ["
        << strutil::StrTrim(
            req->request()->server_header()->ComposeFirstLine()) << "]"
        << " conn sent bytes so far: "
        << (connection_ != NULL ?
            connection_->count_bytes_written() : -1)
        << " conn received bytes so far: "
        << (connection_ != NULL ?
            connection_->count_bytes_read() : -1)
        << " should_close: " << should_close;
    active_requests_.erase(req);
    if ( crt_recv_ == req ) {
      crt_recv_ = NULL;
    }
    net_selector_->DeleteInSelectLoop(req);
    if ( active_requests_.empty() && connection_ == NULL ) {
      net_selector_->DeleteInSelectLoop(this);
    }
  }
  if ( should_close ) {
    closed_ = true;
    if ( connection_ != NULL ) {
      LOG_HTTP << "Closing connection.";
      connection_->FlushAndClose();
    }
  } else if ( is_eos && connection_ != NULL ) {
    timeouter_.SetTimeout(kRequestTimeout,
                          protocol_params().keep_alive_timeout_sec_ * 1000);
  }
  if ( signal_ready ) {
    req->SignalReady();
  }
}

void ServerProtocol::NotifyConnectionWrite() {
  DCHECK(net_selector_->IsInSelectThread());

  if ( crt_send_ != NULL ) {
    // update outbuf_size proxies (outbuf is depleting)
    crt_send_->UpdateOutputBytes();
    if ( crt_send_->pending_output_bytes() < crt_send_->ready_pending_limit_ ) {
      crt_send_->SignalReady();
    }
  }
  if ( !connection_->outbuf()->IsEmpty() ) {
    timeouter_.SetTimeout(kWriteTimeout,
                          protocol_params().reply_write_timeout_ms_);
  } else {
    timeouter_.UnsetTimeout(kWriteTimeout);
  }
}

//////////////////////////////////////////////////////////////////////

void ServerRequest::ReplyWithStatus(HttpReturnCode status) {
  CHECK(protocol_ != NULL);
  CHECK(protocol_->net_selector()->IsInSelectThread());
  // We do not lock on this one - as the reply happens once
  CHECK(!is_server_streaming_);
  protocol_->ReplyForRequest(this, status);
}

void ServerRequest::BeginStreamingData(HttpReturnCode status,
                                       Closure* closed_callback,
                                       bool chunked) {
  CHECK(protocol_->net_selector()->IsInSelectThread());
  // As this can come from another thread, we need to move it all
  // to the main thread.
  CHECK(!is_server_streaming_)
      << "Bug - Multiple calls to BeginStreamingData";
  is_server_streaming_ = true;
  is_server_streaming_chunks_ = chunked;
  if ( closed_callback != NULL ) {
    set_closed_callback(closed_callback);
  }
  // We never orphan a connection here - causes loads of trouble ..
  protocol_->PrepareResponse(this, status);
}

void ServerRequest::ContinueStreamingData() {
  CHECK(protocol_->net_selector()->IsInSelectThread());
  CHECK(is_server_streaming_)
      << "Bug - verify that your request is not orphaned.";
  protocol_->StreamData(this, false);
  // update outbuf_size proxies (outbuf is growing)
  UpdateOutputBytes();
}

void ServerRequest::EndStreamingData() {
  // As this can come from another thread, we need to move it all
  // to the main thread.
  CHECK(protocol_->net_selector()->IsInSelectThread());
  CHECK(is_server_streaming_)
      << "Bug - verify that your request is not orphaned.";

  protocol_->StreamData(this, true);
  // update outbuf_size proxies (outbuf is growing)
  UpdateOutputBytes();
}

void ServerRequest::AnswerUnauthorizedRequest(
    net::UserAuthenticator* authenticator) {
  request()->server_header()->AddField(
      http::kHeaderWWWAuthenticate,
      strutil::StringPrintf("Basic realm=\"%s\"",
                            authenticator->realm().c_str()),
      true);
  ReplyWithStatus(http::UNAUTHORIZED);
}

bool ServerRequest::AuthenticateRequest(net::UserAuthenticator* authenticator) {
  if ( authenticator == NULL ) {
    return true;
  }
  string user, passwd;
  if ( !request()->client_header()->GetAuthorizationField(
           &user, &passwd) ||
       (authenticator->Authenticate(user, passwd) !=
        net::UserAuthenticator::Authenticated) ) {
    return false;
  }
  return true;
}

void ServerRequest::AuthenticateRequest(
    net::UserAuthenticator* authenticator,
    net::UserAuthenticator::AnswerCallback* answer_callback) {
  if ( authenticator == NULL ) {
    answer_callback->Run(net::UserAuthenticator::Authenticated);
    return;
  }
  string user, passwd;
  if ( !request()->client_header()->GetAuthorizationField(&user, &passwd) ) {
    answer_callback->Run(net::UserAuthenticator::MissingCredentials);
    return;
  }
  authenticator->Authenticate(user, passwd, answer_callback);
}
}
