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
// Authors: Cosmin Tudorache & Catalin Popescu

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef LINUX_ERQUEUE_H
#include <linux/errqueue.h>
#endif

#include "whisperlib/base/errno.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/base/gflags.h"

#include "whisperlib/net/connection.h"
#include "whisperlib/net/dns_resolver.h"

DEFINE_bool(net_connection_debug,
            false,
            "Enable debug messages in NetConnection");

namespace net {

#define IF_NET_DEBUG if ( !FLAGS_net_connection_debug ); else

#define ICONNLOG  IF_NET_DEBUG LOG_INFO << this->PrefixInfo()
#define WCONNLOG  IF_NET_DEBUG LOG_WARNING << this->PrefixInfo()
#define ECONNLOG              LOG_ERROR << this->PrefixInfo()
#define FCONNLOG              LOG_FATAL << this->PrefixInfo()

// log always
#define AICONNLOG             LOG_INFO << this->PrefixInfo()

#  define DCONNLOG IF_NET_DEBUG DLOG_INFO << this->PrefixInfo()
#  define D10CONNLOG IF_NET_DEBUG VLOG(10) << this->PrefixInfo()


void NetAcceptor::SetFilterHandler(FilterHandler* filter_handler, bool own) {
  CHECK(filter_handler->is_permanent());
  DetachFilterHandler();
  filter_handler_ = filter_handler;
  own_filter_handler_ = own;
}
void NetAcceptor::DetachFilterHandler() {
  if ( own_filter_handler_ ) {
    delete filter_handler_;
  }
  filter_handler_ = NULL;
  own_filter_handler_ = false;
}
void NetAcceptor::SetAcceptHandler(AcceptHandler* accept_handler, bool own) {
  CHECK(accept_handler->is_permanent());
  DetachAcceptHandler();
  accept_handler_ = accept_handler;
  own_accept_handler_ = own;
}
void NetAcceptor::DetachAcceptHandler() {
  if ( own_accept_handler_ ) {
    delete accept_handler_;
  }
  accept_handler_ = NULL;
  own_accept_handler_ = false;
}
void NetAcceptor::DetachAllHandlers() {
  DetachFilterHandler();
  DetachAcceptHandler();
}

bool NetAcceptor::InvokeFilterHandler(const net::HostPort& peer_address) {
  return !filter_handler_ ||
          filter_handler_->Run(peer_address);
}
void NetAcceptor::InvokeAcceptHandler(NetConnection* new_connection) {
  CHECK_NOT_NULL(accept_handler_) << "missing accept_handler_ !";
  accept_handler_->Run(new_connection);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void NetConnection::SetConnectHandler(ConnectHandler* connect_handler,
                                      bool own) {
  CHECK(connect_handler->is_permanent());
  DetachConnectHandler();
  connect_handler_ = connect_handler;
  own_connect_handler_ = own;
}
void NetConnection::DetachConnectHandler() {
  if ( own_connect_handler_ ) {
    delete connect_handler_;
  }
  connect_handler_ = NULL;
  own_connect_handler_ = false;
}
void NetConnection::SetReadHandler(ReadHandler* read_handler, bool own) {
  CHECK(read_handler->is_permanent());
  DetachReadHandler();
  read_handler_ = read_handler;
  own_read_handler_ = own;
}
void NetConnection::DetachReadHandler() {
  if ( own_read_handler_ ) {
    delete read_handler_;
  }
  read_handler_ = NULL;
  own_read_handler_ = false;
}
void NetConnection::SetWriteHandler(WriteHandler* write_handler, bool own) {
  CHECK(write_handler->is_permanent());
  DetachWriteHandler();
  write_handler_ = write_handler;
  own_write_handler_ = own;
}
void NetConnection::DetachWriteHandler() {
  if ( own_write_handler_ ) {
    delete write_handler_;
  }
  write_handler_ = NULL;
  own_write_handler_ = false;
}
void NetConnection::SetCloseHandler(CloseHandler* close_handler, bool own) {
  CHECK(close_handler->is_permanent());
  DetachCloseHandler();
  close_handler_ = close_handler;
  own_close_handler_ = own;
}
void NetConnection::DetachCloseHandler() {
  if ( own_close_handler_ ) {
    delete close_handler_;
  }
  close_handler_ = NULL;
  own_close_handler_ = false;
}
void NetConnection::DetachAllHandlers() {
  DetachConnectHandler();
  DetachReadHandler();
  DetachWriteHandler();
  DetachCloseHandler();
}

void NetConnection::InvokeConnectHandler() {
  ICONNLOG << "Connected! invoking application connect handler.. ";
  CHECK_NOT_NULL(connect_handler_) << "no connect_handler found";
  connect_handler_->Run();
}
bool NetConnection::InvokeReadHandler() {
  CHECK_NOT_NULL(read_handler_) << "no read_handler found";
  return read_handler_->Run();
  // TODO(cosmin): check return value on read_handler_ or make it return void
}
bool NetConnection::InvokeWriteHandler() {
  CHECK_NOT_NULL(write_handler_) << "no write_handler found";
  return write_handler_->Run();
  // TODO(cosmin): check return value on write_handler_ or make it return void
}
void NetConnection::InvokeCloseHandler(int err, CloseWhat what) {
  if ( !close_handler_ ) {
    // TODO(cosmin): remove fatal/warning.
    //               Default behavior should FlushAndClose.
    FCONNLOG << "No close_handler_ found";
    FlushAndClose();
    return;
  }
  close_handler_->Run(err, what);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

TcpAcceptor::TcpAcceptor(Selector* selector,
                         const TcpAcceptorParams& tcp_params)
  : NetAcceptor(tcp_params),
    Selectable(selector),
    tcp_params_(tcp_params),
    fd_(INVALID_FD_VALUE) {
}
TcpAcceptor::~TcpAcceptor() {
  InternalClose(0);
  CHECK_EQ(fd_, INVALID_FD_VALUE);
  CHECK_EQ(state(), DISCONNECTED);
}

bool TcpAcceptor::Listen(const HostPort& local_addr) {
  struct sockaddr_storage addr;
  const bool is_ipv6 = local_addr.SockAddr(&addr);

  CHECK(fd_ == INVALID_FD_VALUE) << "Attempting Listen on valid socket";
  CHECK(state() == DISCONNECTED) << "Attempting Listen on listening socket";

  // create socket
  fd_ = ::socket(addr.ss_family, SOCK_STREAM, 0);
  if ( fd_ < 0 ) {
    ECONNLOG << "::socket failed, err: " << GetLastSystemErrorDescription();
    return false;
  }

  // set socket options
  if ( !SetSocketOptions() ) {
    ECONNLOG << "SetSocketOptions failed, closing socket fd: " << fd_;
    int result = ::close(fd_);
    if ( result != 0 ) {
      ECONNLOG << "::close failed for fd=" << fd_
               << " with error: " << GetLastSystemErrorDescription();
    }
    fd_ = INVALID_FD_VALUE;
    return false;
  }

  // bind socket
  if ( ::bind(fd_, reinterpret_cast<const sockaddr*>(&addr),
              is_ipv6 ? sizeof(addr) : sizeof(sockaddr)) ) {
    ECONNLOG << "Error binding fd: " << fd_ << " to "
             << local_addr << " : " << GetLastSystemErrorDescription();
    int result = ::close(fd_);
    if ( result != 0 ) {
      ECONNLOG << "::close failed for fd=" << fd_
               << " with error: " << GetLastSystemErrorDescription();
    }
    fd_ = INVALID_FD_VALUE;
    return false;
  }

  // listen on socket
  if ( ::listen(fd_, tcp_params_.backlog_) ) {
    ECONNLOG << "::listen failed for fd: " << fd_
             << " , backlog: " << tcp_params_.backlog_
             << " , local_address: " << local_addr
             << " , err: " << GetLastSystemErrorDescription();
    int result = ::close(fd_);
    if ( result != 0 ) {
      ECONNLOG << "::close failed for fd=" << fd_
               << " with error: " << GetLastSystemErrorDescription();
    }
    fd_ = INVALID_FD_VALUE;
    return false;
  }

  // register to selector
  if ( !selector_->Register(this) ) {
    ECONNLOG << "selector_->Register failed, closing socket fd: " << fd_;
    int result = ::close(fd_);
    if ( result != 0 ) {
      ECONNLOG << "::close failed for fd=" << fd_
               << " with error: " << GetLastSystemErrorDescription();
    }
    fd_ = INVALID_FD_VALUE;
    return false;
  }

  // Initialize local address from socket. In case the user supplied
  // port 0 we learn the system chosen port now.
  InitializeLocalAddress();
  set_state(LISTENING);
  AICONNLOG << "Bound and listening on " << local_address();

  // Read Events are enabled by default

  return true;
}
void TcpAcceptor::Close() {
  InternalClose(0);
}
string TcpAcceptor::PrefixInfo() const {
  ostringstream oss;
  oss << StateName() << " : [" << local_address() << " (fd: " << fd_ << ")] ";
  return oss.str();
}

bool TcpAcceptor::SetSocketOptions() {
  CHECK_NE(fd_, INVALID_FD_VALUE);
  // Enable non blocking (critical for using selector)
  const int flags = fcntl(fd_, F_GETFL, 0);
  if ( flags < 0 ) {
    ECONNLOG << "::fcntl failed for fd=" << fd_
             << " err: " << GetLastSystemErrorDescription();
    return false;
  }
  const int new_flags = flags | O_NONBLOCK;
  int result = fcntl(fd_, F_SETFL, new_flags);
  if ( result < 0 ) {
    ECONNLOG << "::fcntl failed for fd=" << fd_
             << " new_flags=" << new_flags
             << " err: " << GetLastSystemErrorDescription();
    return false;
  }
  // Enable fast bind reusing (without this option, closing the socket
  //  will switch OS port to CLOSE_WAIT state for ~1 minute, during which
  //  bind fails with EADDRINUSE)
  const int true_flag = 1;
  if ( setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                  reinterpret_cast<const char *>(&true_flag),
                  sizeof(true_flag)) < 0 ) {
    ECONNLOG << "::setsockopt failed for fd_=" << fd_
             << " err: " << GetLastSystemErrorDescription();
    return false;
  }
  // Also disable the SIGPIPE - for IOS / APPLE (on linux we deal w/ it
  // via the signal handler)
#if defined(IOS) || defined(MAXOSX)
  const int set = 1;
  if ( setsockopt(fd_, SOL_SOCKET, SO_NOSIGPIPE,
                  reinterpret_cast<const char *>(&set),
                  sizeof(set)) ) {
    ECONNLOG << "::setsockopt SO_NOSIGPIPE failed for fd_=" << fd_
             << " err: " << GetLastSystemErrorDescription();
    return false;
  }
#endif


  return true;
}

int TcpAcceptor::ExtractSocketErrno() {
  return TcpConnection::ExtractSocketErrno(fd_);
}

void TcpAcceptor::InternalClose(int err) {
  if ( fd_ == INVALID_FD_VALUE ) {
    CHECK_EQ(state(), DISCONNECTED);
    return;
  }
  D10CONNLOG << "Unregistering acceptor (fd: " << fd_ << ")...";
  selector_->Unregister(this);
  D10CONNLOG << "Performing ::close... ";
  int result = ::close(fd_);
  if ( result != 0 ) {
    ECONNLOG << "::close failed for fd=" << fd_
             << " with error: " << GetLastSystemErrorDescription();
  }
  fd_ = INVALID_FD_VALUE;
  set_state(DISCONNECTED);
  set_last_error_code(err);
}

bool TcpAcceptor::HandleReadEvent(const SelectorEventData& event) {
  // new client connection

  //perform ::accept
  struct sockaddr_storage address;
  socklen_t addrlen = sizeof(address);
  const int client_fd = ::accept(fd_,
                                 reinterpret_cast<struct sockaddr*>(&address),
                                 &addrlen);
  if ( client_fd < 0 ) {
    if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
      // This could happen if the connecting client goes away just before
      // we execute "accept".
      ECONNLOG << "HandleReadEvent with no pending connection request!: "
               << GetLastSystemErrorDescription();
      return true;
    }
    ECONNLOG << "::accept failed: " << GetLastSystemErrorDescription();
    return false;
  }

  HostPort hp(&address);
  ICONNLOG << "TcpConnection accepted from " << hp;

  // filter client
  if ( !InvokeFilterHandler(hp) ) {
    ECONNLOG << "Dumping connection from " << hp
             << " because filter_handler_ refused it";
    if ( ::close(client_fd) ) {
      ECONNLOG << "::close fd: " << fd_ << " failed: "
               << GetLastSystemErrorDescription();
    }
    return true;
  }
  Selector* const selector_to_use = tcp_params_.GetNextSelector();
  if ( selector_to_use != NULL ) {
    selector_to_use->RunInSelectLoop(
        NewCallback(this, &TcpAcceptor::InitializeAcceptedConnection,
                    selector_to_use, client_fd));
  } else {
    InitializeAcceptedConnection(selector_, client_fd);
  }
  return true;
}

void TcpAcceptor::InitializeAcceptedConnection(Selector* selector,
                                               int client_fd) {
  // WARNING: selector = is a secondary net selector
  //          selector_ = is the main media selector
  //          InvokeAcceptHandler() is running on the secondary net selector.
  DCHECK(selector->IsInSelectThread());

  // create a TcpConnection object for this client
  TcpConnection* client = new TcpConnection(selector,
                                            tcp_params_.tcp_connection_params_);
  if ( !client->Wrap(client_fd) ) {
    ECONNLOG << "Failed to Wrap incoming client fd: " << client_fd
             << " dumping connection..";
    if ( ::close(client_fd) ) {
      ECONNLOG << "::close fd: " << fd_ << " failed: "
               << GetLastSystemErrorDescription();
    }
  } else {
    // for TCP an accepted fd is already fully connected
    CHECK_EQ(client->state(), TcpConnection::CONNECTED);

    // deliver this new client to application
    InvokeAcceptHandler(client);
  }
}

void TcpAcceptor::InitializeLocalAddress() {
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  if ( !::getsockname(fd_,
                      reinterpret_cast<sockaddr*>(&addr), &len) ) {
    set_local_address(HostPort(&addr));
  } else {
    ECONNLOG << "::getsockname failed: " << GetLastSystemErrorDescription();
  }
}

bool TcpAcceptor::HandleWriteEvent(const SelectorEventData& event) {
  FCONNLOG << "Erroneous call to HandleWriteEvent on server socket";
  return false;
}


bool TcpAcceptor::HandleErrorEvent(const SelectorEventData& event) {
#ifndef __USE_LEAN_SELECTOR__
  //////////////////////////////////////////////////////////////////////
  //
  // EPOLL & EPOLL version
  //
  const int events = event.internal_event_;
  ECONNLOG << "HandleErrorEvent: 0x" << std::hex << events;
#if defined(HAVE_SYS_EPOLL_H)
  if ( events & EPOLLHUP )
#else
  if ( events & POLLHUP )
#endif
  {
    ECONNLOG << "HUP on server socket";
    return true;
  }
#if defined(HAVE_SYS_EPOLL_H)
  if ( events & EPOLLRDHUP )
#else
  if ( events & POLLRDHUP )
#endif
  {
    ECONNLOG << "RDHUP on server socket";
    return true;
  }
#if defined(HAVE_SYS_EPOLL_H)
  if ( events & EPOLLERR )
#else
  if ( events & POLLERR )
#endif
  {
    const int err = ExtractSocketErrno();
    ECONNLOG << local_address().ToString()  << " HandleErrorEvent err=" << err
             << " - " << GetSystemErrorDescription(err);
    InternalClose(err);
    return false;
  }
  ECONNLOG << "HandleErrorEvent: unknown event: 0x" << std::hex << events;
#endif  // __USE_LEAN_SELECTOR__
  return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

TcpConnection::TcpConnection(Selector* selector,
                             const TcpConnectionParams& tcp_params)
    : NetConnection(selector, tcp_params),
      Selectable(selector),
      tcp_params_(tcp_params),
      fd_(INVALID_FD_VALUE),
      local_address_(),
      remote_address_(),
      write_closed_(true),
      read_closed_(true),
      timeouter_(selector,
          NewPermanentCallback(this, &TcpConnection::HandleTimeoutEvent)),
      last_read_ts_(0),
      last_write_ts_(0),
      handle_dns_result_(NewPermanentCallback(this,
          &TcpConnection::HandleDnsResult)) {
}

TcpConnection::~TcpConnection() {
  InternalClose(0, true);
  CHECK_EQ(state(), DISCONNECTED);
  DetachAllHandlers();
  delete handle_dns_result_;
  handle_dns_result_ = NULL;
}

void TcpConnection::Close(CloseWhat what) {
  if ( fd_ == INVALID_FD_VALUE ) {
    CHECK_EQ(state(), DISCONNECTED);
    return;
  }

  ///////////////////////////////////////////
  // Ignore CLOSE_READ, we should never need it.

  ////////////////////////////////////////////
  // If CLOSE_WRITE requested , go to FLUSHING state
  if ( what == CLOSE_WRITE ||
       what == CLOSE_READ_WRITE ) {
    if ( !write_closed() ) {
      set_state(FLUSHING);
      RequestWriteEvents(true);
      // NOTE: when outbuf_ gets empty we execute ::shutdown(write)
      //       and set write_closed_ = true
    }
  }
}

//////////////////////////////////////////////////////////////////////

bool TcpConnection::Wrap(int fd) {
  fd_ = fd;
  if ( !SetSocketOptions() ) {
    return false;
  }
  if ( !selector_->Register(this) ) {
    fd_ = INVALID_FD_VALUE;
    return false;
  }
  set_state(TcpConnection::CONNECTED);
  set_read_closed(false);
  set_write_closed(false);
  InitializeLocalAddress();
  InitializeRemoteAddress();
  RequestReadEvents(true);
  return true;
}

bool TcpConnection::Connect(const HostPort& remote_addr) {
  CHECK(state() == DISCONNECTED ||
        state() == RESOLVING) << "Illegal state: " << StateName();
  CHECK(fd_ == INVALID_FD_VALUE) << "FD already created?!";

  // maybe start DNS resolve
  if ( state() == DISCONNECTED && remote_addr.ip_object().IsInvalid() ) {
    remote_address_ = remote_addr;
    LOG_INFO << " Resolving first: " << remote_addr.host() << " to connect";
    DnsResolve(net_selector(), remote_addr.host(), handle_dns_result_);
    set_state(RESOLVING);
    // NEXT: HandleDnsResult will be called when DNS query completes
    return true;
  }

  struct sockaddr_storage addr;
  const bool is_ipv6 = remote_addr.SockAddr(&addr);
  // create socket
  fd_ = ::socket(addr.ss_family, SOCK_STREAM, 0);
  if ( fd_ < 0 ) {
    ECONNLOG << "::socket failed: " << GetLastSystemErrorDescription();
    fd_ = INVALID_FD_VALUE;
    return false;
  }
  // set socket options: non-blocking, ...
  if ( !SetSocketOptions() ) {
    ECONNLOG << "SetSocketOptions failed, aborting Connect.. ";
    ::close(fd_);
    fd_ = INVALID_FD_VALUE;
    return false;
  }
  // register with selector
  if ( !selector_->Register(this) ) {
    ECONNLOG << "Failed to register with selector, aborting Connect.. ";
    ::close(fd_);
    fd_ = INVALID_FD_VALUE;
    return false;
  }

  // begin connect
  set_state(CONNECTING);
  set_read_closed(false);
  set_write_closed(false);
  remote_address_ = remote_addr;

  if ( ::connect(fd_,
                 reinterpret_cast<const struct sockaddr*>(&addr),
                 is_ipv6 ? sizeof(addr) : sizeof(struct sockaddr)) ) {
    CHECK(errno != EALREADY)
        << "a previous connection attempt has not yet  been completed";
    if ( errno == EINPROGRESS ) {
      ICONNLOG << "Connecting to " << remote_address();
      RequestWriteEvents(true);
      RequestReadEvents(true);
      return true;
    } else {
        ECONNLOG << "Error connecting fd: " << fd_
               << " to addr: " << remote_addr
               << ", family: " << addr.ss_family
               << ", error: " << GetLastSystemErrorDescription();
      InternalClose(GetLastSystemError(), false);
      return false;
    }
  }

  // TODO(cosmin): connect already completed.
  //               But to simplify logic, we wait for the first HandleWrite
  //               and call InvokeConnectHandler there.
  RequestWriteEvents(true);
  RequestReadEvents(true);
  return true;
  /*
  // connect completed
  set_state(CONNECTED);
  InitializeLocalAddress();

  // Even in this unlikely event, we do not run HandleConnect from inside
  // Connect - it may deadlock: the application usually synchronizes
  // Connect and HandleConnect.
  selector_->RunInSelectLoop(
      NewCallback(this, &TcpConnection::InvokeConnectHandler));
  return true;
  */
}

void TcpConnection::FlushAndClose() {
  Close(CLOSE_WRITE);
}
void TcpConnection::ForceClose() {
  InternalClose(0, true);
}

bool TcpConnection::SetSendBufferSize(int size) {
  if ( ::setsockopt(fd_, SOL_SOCKET, SO_SNDBUF,
                    reinterpret_cast<const char *>(&size), sizeof(size)) ) {
    ECONNLOG << "::setsockopt failed: " << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}
bool TcpConnection::SetRecvBufferSize(int size) {
  if ( ::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF,
                    reinterpret_cast<const char *>(&size), sizeof(size)) ) {
    ECONNLOG << "::setsockopt failed: " << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}

void TcpConnection::RequestReadEvents(bool enable) {
  CHECK_NOT_NULL(selector_);
  D10CONNLOG << "RequestReadEvents => " << std::boolalpha << enable;
  selector_->EnableReadCallback(this, enable);
}
void TcpConnection::RequestWriteEvents(bool enable) {
  CHECK_NOT_NULL(selector_);
  D10CONNLOG << "RequestWriteEvents => " << std::boolalpha << enable;
  selector_->EnableWriteCallback(this, enable);
}

const HostPort& TcpConnection::local_address() const {
  return local_address_;
}
const HostPort& TcpConnection::remote_address() const {
  return remote_address_;
}

string TcpConnection::PrefixInfo() const {
  ostringstream oss;
  int64 now_ts = selector_ != NULL ? selector_->now() : timer::TicksMsec();
  oss << StateName() << " : ["
      << local_address() << " => " << remote_address()
      << " (fd: " << fd_
      << ", R ms: ";
  if ( last_read_ts_ == 0 ) {
    oss << "never";
  } else {
    oss << (now_ts - last_read_ts_);
  }
  oss << ", W ms: ";
  if ( last_write_ts_ == 0 ) {
    oss << "never";
  } else {
    oss << (now_ts - last_write_ts_);
  }
  oss << ")] ";
  return oss.str();
}


//////////////////////////////////////////////////////////////////////

bool TcpConnection::HandleReadEvent(const SelectorEventData& event) {
  CHECK(state() != DISCONNECTED) << "Invalid state: " << StateName();
  D10CONNLOG << "HandleReadEvent: " << std::hex << event.internal_event_;

  if ( state() == CONNECTING ) {
    set_state(CONNECTED);
    // read and write events should be enabled
    InitializeLocalAddress();
    InvokeConnectHandler();
    CHECK(state() == CONNECTED ||
          state() == DISCONNECTED ||
          state() == FLUSHING);
    // either the application closed the connection in "ConnectHandler"
    // or the connection goes on in CONNECTED state
    return state() == CONNECTED;
  }

  CHECK(state() == CONNECTED ||
        state() == FLUSHING) << "Illegal state: " << StateName();

  // Read from network into inbuf_
  int32 cb = Selectable::Read(inbuf(), tcp_params_.read_limit_);
  if ( cb < 0 ) {
    const int err = ExtractSocketErrno();
    ECONNLOG << "Closing connection because Read failed: "
             << " system: " << GetLastSystemErrorDescription()
             << " - internal err no: " << err;
    InternalClose(err, true);
    return false;
  }

  D10CONNLOG << "HandleReadEvent: #" << cb << " bytes read,"
             << " #" << inbuf()->Size() << " total bytes in inbuf_";
  inc_bytes_read(cb);
  last_read_ts_ = selector_->now();

  if ( cb > 0 ) {
    // call application read_handler_
    if ( !InvokeReadHandler() ) {
      WCONNLOG << "Closing TcpConnection because read_handler_ said so";
      InternalClose(0, true);
      return false;
    }
    D10CONNLOG << "HandleReadEvent: after read_handler_"
               << " #" << inbuf()->Size() << " bytes still remaining in inbuf_";
  }

  if ( cb == 0 ) {
    if ( write_closed() ||
         state() == FLUSHING ||
         (errno && (errno != EAGAIN && errno != EWOULDBLOCK)) ) {
      WCONNLOG << "Previous read returned 0 bytes, READ half closed";
      set_read_closed(true);
    }
  }

  if ( read_closed() ) {
    InvokeCloseHandler(0, CLOSE_READ);
    if ( fd_ != INVALID_FD_VALUE ) {
      // we need this because (E)POLLIN continuously fires
      RequestReadEvents(false);
      // TODO(cosmin): remove, application should close WRITE
      Close(CLOSE_WRITE);
    }
    return true;
  }
  return true;
}

bool TcpConnection::HandleWriteEvent(const SelectorEventData& event) {
  CHECK(state() != DISCONNECTED) << "Invalid state: " << StateName();
  D10CONNLOG << "HandleWriteEvent: " << std::hex << event.internal_event_;

  if ( state() == CONNECTING ) {
    set_state(CONNECTED);
    // read and write events should be enabled
    InitializeLocalAddress();
    InvokeConnectHandler();
    CHECK(state() == CONNECTED ||
          state() == DISCONNECTED ||
          state() == FLUSHING);
    // either the application closed the connection in "ConnectHandler"
    // or the connection goes on in CONNECTED state
    return state() == CONNECTED;
  }

  CHECK(state() == CONNECTED ||
        state() == FLUSHING) << "Illegal state: " << StateName();

  // write data from outbuf_ to network
  const int32 cb = Selectable::Write(outbuf(), tcp_params_.write_limit_);
  if ( cb < 0 ) {
    const int err = ExtractSocketErrno();
    ECONNLOG << "Closing connection because Write failed: "
             << "  system: " << GetLastSystemErrorDescription()
             << " - internal err: " << err;
    InternalClose(err, true);
    return false;
  }
  D10CONNLOG << "HandleWriteEvent: #" << cb << " bytes written"
             << " to: " << remote_address() << ", left: " << outbuf()->Size();
  inc_bytes_written(cb);
  last_write_ts_ = selector_->now();

  if ( state() != FLUSHING ) {
    // call application write_handler_
    if ( !InvokeWriteHandler() ) {
      WCONNLOG << "Closing connection because write_handler_ said so";
      InternalClose(0, true);
      return false;
    }
  }

  if ( outbuf()->IsEmpty() ) {
    RequestWriteEvents(false);   // stop write events.

    if ( state() == FLUSHING ) {
      // FLUSHING finished sending all buffered data.
      // Execute ::shutdown write half.
      WCONNLOG << "Flushing finished, executing shutdown WRITE half.";
      int result = ::shutdown(fd_, SHUT_WR);
      if ( result != 0 ) {
        // ENOTCONN happens when we want to shutdown before the initial
        // connect gets completed. So it's not an error actually.
        if ( GetLastSystemError() != ENOTCONN ) {
          ECONNLOG << "::shutdown failed with fd=" << fd_ << " how=SHUT_WR"
                   << " err: " << GetLastSystemErrorDescription();
        }
        InternalClose(0, true);
        return false;
      }
      set_write_closed(true);
      // We closed the write half, the peer is notified by RDHUP.
      // Now we wait him to close the connection too, and when it does
      // we get a HUP.
      // In case of linger_timeout we force close the connection.
      timeouter_.SetTimeout(kShutdownTimeoutId,
                            tcp_params_.shutdown_linger_timeout_ms_);
      return true;
    }
  }

  return true;
}

bool TcpConnection::HandleErrorEvent(const SelectorEventData& event) {
#ifndef __USE_LEAN_SELECTOR__
  CHECK_NE(state(), DISCONNECTED);

  const int events = event.internal_event_;

  // Possible error events, according to epoll_ctl(2) manual page:
  // ("events" is a combination of one or more of these)
  //
  // EPOLLRDHUP  Stream socket peer closed connection, or shut down
  //             writing half of connection. (This flag is especially useful
  //             for writing simple code to detect peer shutdown when using
  //             Edge Triggered monitoring.)
  //
  // EPOLLERR    Error condition happened on the associated file descriptor.
  //
  // EPOLLHUP    Hang up happened on the associated file descriptor.
  //
  // (Similar for poll)
#if defined(HAVE_SYS_EPOLL_H)
  if ( (events & EPOLLERR) == EPOLLERR )
#else
  if ( (events & POLLERR) == POLLERR )
#endif
  {
    const int err = ExtractSocketErrno();
    ECONNLOG << " HandleErrorEvent err=" << err
             << " - " << GetSystemErrorDescription(err);
    InternalClose(err, true);
    return false;
  }

  // IMPORTANT:
  // The chain of events on a connected socket:
  //        A                            B
  //    -----------                 -----------
  // executes:
  // a) close fd, or
  // b) shutdown write
  //     ========================>
  //                               receives RDHUP, and executes
  //                               c) close fd, or
  //                               d) shutdown write
  //     <========================
  // a) nothing happens
  // b) receives HUP,
  //    and executes close fd
  //     ========================>
  //                               c) nothing happens
  //                               d) receives HUP,
  //                                  and executes close fd

#if defined(HAVE_SYS_EPOLL_H)
  if ( (events & EPOLLHUP) == EPOLLHUP )
#else
  if ( (events & POLLHUP) == POLLHUP )
#endif
  {
    // peer completely closed the connection
    WCONNLOG << "HandleErrorEvent: EPOLLHUP, both READ and WRITE halves closed";
    set_write_closed(true);
#if defined(HAVE_SYS_EPOLL_H)
    if ( (events & EPOLLIN) && state() != CONNECTING )
#else
    if ( (events & POLLIN) && state() != CONNECTING )
#endif
    {
      // don't close here, let the next HandleReadEvent read pending data.
      // EPOLLHUP is continuously generated.
      return true;
    }
    WCONNLOG << "Closing connection because both"
                " READ and WRITE halves are closed.";
    InternalClose(0, true);
    return false;
  }
#if defined(HAVE_SYS_EPOLL_H)
  if ( (events & EPOLLRDHUP) == EPOLLRDHUP )
#else
  if ( (events & POLLRDHUP) == POLLRDHUP )
#endif
  {
    WCONNLOG << "HandleErrorEvent: EPOLLRDHUP, READ half closed";
    set_state(FLUSHING);
#if defined(HAVE_SYS_EPOLL_H)
    if ( (events & EPOLLIN) && state() != CONNECTING )
#else
    if ( (events & POLLIN) && state() != CONNECTING )
#endif
    {
      // peer closed write half of the connection
      // there may be pending data on read. So wait until recv() returns 0,
      // then set read_closed_ = true;
      return true;
    }
    // no (E)POLLIN means READ disabled ..
    ECONNLOG << "Peer closed on us - treat as error !";
    InternalClose(0, true);
    return false;
  }
  ECONNLOG << "HandleErrorEvent: unknown event: 0x" << std::hex << events;
#endif  // __USE_LEAN_SELECTOR__
  return true;
}

void TcpConnection::Close() {
  // this call comes from Selectable interface
  InternalClose(0, true);
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

bool TcpConnection::SetSocketOptions() {
  // enable non-blocking
  CHECK_GE(fd_, 0);
  const int flags = fcntl(fd_, F_GETFL, 0);
  if ( flags < 0 ) {
    ECONNLOG << "::fcntl failed for fd=" << fd_
             << " err: " << GetLastSystemErrorDescription();
    return false;
  }
  const int new_flags = flags | O_NONBLOCK;
  int result = ::fcntl(fd_, F_SETFL, new_flags);
  if ( result < 0 ) {
    ECONNLOG << "::fcntl failed for fd=" << fd_
             << " new_flags=" << new_flags
             << " err: " << GetLastSystemErrorDescription();
    return false;
  }
  // disable Nagel buffering algorithm
  const int true_flag = 1;
  if ( ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY,
                    reinterpret_cast<const char *>(&true_flag),
                    sizeof(true_flag)) < 0 ) {
    ECONNLOG << "::setsockopt failed for fd=" << fd_
             << " err: " << GetLastSystemErrorDescription();
    return false;
  }
  // set tcp parameters
  if ( tcp_params_.send_buffer_size_ != -1 ) {
    if ( !SetSendBufferSize(tcp_params_.send_buffer_size_) ) {
      return false;
    }
  }
  if ( tcp_params_.recv_buffer_size_ != -1 ) {
    if ( !SetRecvBufferSize(tcp_params_.recv_buffer_size_) ) {
      return false;
    }
  }
  // Also disable the SIGPIPE - for IOS / APPLE (on linux we deal w/ it
  // via the signal handler)
#if defined(IOS) || defined(MAXOSX)
  const int set = 1;
  if ( setsockopt(fd_, SOL_SOCKET, SO_NOSIGPIPE,
                  reinterpret_cast<const char *>(&set),
                  sizeof(set)) ) {
    ECONNLOG << "::setsockopt SO_NOSIGPIPE failed for fd_=" << fd_
             << " err: " << GetLastSystemErrorDescription();
    return false;
  }
#endif

  return true;
}

// static
int TcpConnection::ExtractSocketErrno(int fd) {
  int error;
  socklen_t len = sizeof(error);
  if ( getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) ) {
    return errno;
  }
  return error;
}

void TcpConnection::InitializeLocalAddress() {
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  if ( !::getsockname(fd_,
                      reinterpret_cast<struct sockaddr*>(&addr), &len) ) {
    local_address_ = HostPort(&addr);
  } else {
    ECONNLOG << "::getsockname failed: " << GetLastSystemErrorDescription();
  }
}
void TcpConnection::InitializeRemoteAddress() {
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  if ( !::getpeername(fd_,
                      reinterpret_cast<struct sockaddr*>(&addr), &len) ) {
    remote_address_ = HostPort(&addr);
  } else {
    ECONNLOG << "::getpeername failed: " << GetLastSystemErrorDescription();
  }
}

void TcpConnection::InternalClose(int err, bool call_close_handler) {
  if ( state() == DISCONNECTED ) {
    CHECK_EQ(fd_, INVALID_FD_VALUE);
    return;
  }
  if ( fd_ != INVALID_FD_VALUE ) {
    D10CONNLOG << "Unregistering connection.. ";
    selector_->Unregister(this);
    ::shutdown(fd_, SHUT_RDWR);
    DCONNLOG << "Performing the ::close... ";
    if ( ::close(fd_) < 0 ) {
      ECONNLOG << "Error closing fd: " << fd_ << " err: "
               << GetLastSystemErrorDescription();
    }
    fd_ = INVALID_FD_VALUE;
  }
  DnsCancel(handle_dns_result_);
  set_state(DISCONNECTED);
  set_read_closed(true);
  set_write_closed(true);
  set_last_error_code(err);
  timeouter_.UnsetAllTimeouts();
  if (inbuf()->Size() > 0) {
      ECONNLOG << "Unread bytes: #" << inbuf()->Size();
  }
  inbuf()->Clear();
  if (outbuf()->Size() > 0) {
      ECONNLOG << "Unwritten bytes: #" << outbuf()->Size();
  }
  outbuf()->Clear();
  if ( call_close_handler ) {
    InvokeCloseHandler(err, CLOSE_READ_WRITE);
  }
}

void TcpConnection::HandleTimeoutEvent(int64 timeout_id) {
  if ( timeout_id == kShutdownTimeoutId ) {
    ECONNLOG << "Shutdown linger timeout. Forcing close...";
    InternalClose(0, true);
    return;
  }
  FCONNLOG << "Unknown timeout_id: " << timeout_id;
  InternalClose(0, true);
}

void TcpConnection::HandleDnsResult(scoped_ref<DnsHostInfo> info) {
  if (state() != RESOLVING) {
    return;   // probably in course of closing
  }
  if ( info.get() == NULL || info->ipv4_.empty() ) {
    LOG_ERROR << "Cannot resolve: " << remote_address_.host()
              << ", info: " << info.ToString();
    set_state(DISCONNECTED);
    InvokeCloseHandler(0, CLOSE_READ_WRITE);
    return;
  }
  const IpAddress& ip = info->ipv4_[0];
  if ( !Connect(HostPort(ip, remote_address_.port())) ) {
    InvokeCloseHandler(0, CLOSE_READ_WRITE);
    return;
  }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#if defined(HAVE_OPENSSL_SSL_H) && defined(USE_OPENSSL)

SslAcceptor::SslAcceptor(Selector* selector,
                         const SslAcceptorParams& ssl_params)
  : NetAcceptor(ssl_params),
    selector_(selector),
    params_(ssl_params),
    tcp_acceptor_(selector) {
  tcp_acceptor_.SetFilterHandler(NewPermanentCallback(
      this, &SslAcceptor::TcpAcceptorFilterHandler), true);
  tcp_acceptor_.SetAcceptHandler(NewPermanentCallback(
      this, &SslAcceptor::TcpAcceptorAcceptHandler), true);
}
SslAcceptor::~SslAcceptor() {
  SslClear();
}

bool SslAcceptor::TcpAcceptorFilterHandler(const net::HostPort& peer_addr) {
  return InvokeFilterHandler(peer_addr);
}
void SslAcceptor::TcpAcceptorAcceptHandler(NetConnection* net_connection) {
  TcpConnection* tcp_connection = static_cast<TcpConnection*>(net_connection);
  SslConnection* ssl_connection =
      new SslConnection(tcp_connection->net_selector(),
                        params_.ssl_connection_params_);
  ICONNLOG << "SslConnection allocated: " << ssl_connection
            << " starting SSL setup...";
  // Set temporary handlers in the new ssl_connection. We'll be notified
  // when the ssl connect is completed. Only after the ssl connection
  // is fully established we pass it to application.
  ssl_connection->SetConnectHandler(NewPermanentCallback(
      this, &SslAcceptor::SslConnectionConnectHandler, ssl_connection), true);
  ssl_connection->SetCloseHandler(NewPermanentCallback(
      this, &SslAcceptor::SslConnectionCloseHandler, ssl_connection), true);
  ssl_connection->Wrap(tcp_connection);
}

void SslAcceptor::SslConnectionConnectHandler(SslConnection* ssl_connection) {
  ICONNLOG << "SslConnection setup done: " << ssl_connection
           << " forwarding to application";
  // Ssl connection ready, we detach our temporary handlers
  // to let the application attach and use it.
  ssl_connection->DetachAllHandlers();
  // pass ssl connection to application
  InvokeAcceptHandler(ssl_connection);
}
void SslAcceptor::SslConnectionCloseHandler(SslConnection* ssl_connection,
    int err, NetConnection::CloseWhat what) {
  if ( what != NetConnection::CLOSE_READ_WRITE ) {
    // ignore partial close
    return;
  }
  // ssl connection broken, we have to delete it
  // NOTE: we are called from SslConnection !! don't use "delete" here
  ECONNLOG << "SslConnection setup failed: " << ssl_connection
           << " deleting..";
  ssl_connection->net_selector()->DeleteInSelectLoop(ssl_connection);
}

bool SslAcceptor::Listen(const net::HostPort& local_addr) {
  return SslInitialize() && tcp_acceptor_.Listen(local_addr);
}
void SslAcceptor::Close() {
  tcp_acceptor_.Close();
}
string SslAcceptor::PrefixInfo() const {
  return tcp_acceptor_.PrefixInfo() + " [SSL]: ";
}

// TODO(cosmin): test, remove
bool SslAcceptor::SslInitialize() {
  if ( params_.ssl_connection_params_.ssl_context_ == NULL ) {
    ECONNLOG << "Missing SSL context";
    return false;
  }
  if ( SSL_CTX_check_private_key(
         params_.ssl_connection_params_.ssl_context_) != 1) {
    ECONNLOG << "SslAcceptor needs an SSL certificate & key";
    //return false;
  }

  return true;
}
void SslAcceptor::SslClear() {
}

//////////////////////////////////////////////////////////////////////

SslConnection::SslConnection(Selector* selector,
                             const SslConnectionParams& ssl_params)
    : NetConnection(selector, ssl_params),
      selector_(selector),
      ssl_params_(ssl_params),
      tcp_connection_(NULL),
      is_server_side_(false),
      p_ctx_(NULL),
      p_bio_read_(NULL),
      p_bio_write_(NULL),
      p_ssl_(NULL),
      handshake_finished_(false),
      read_blocked_(false),
      read_blocked_on_write_(false),
      write_blocked_on_read_(false),
      ssl_out_count_(0),
      ssl_in_count_(0),
      timeouter_(selector,
                 NewPermanentCallback(this, &SslConnection::HandleTimeoutEvent)) {
}
SslConnection::~SslConnection() {
  ForceClose();
  delete tcp_connection_;
  tcp_connection_ = NULL;
}

void SslConnection::Wrap(TcpConnection* tcp_connection) {
  CHECK_NULL(tcp_connection_);
  tcp_connection_ = tcp_connection;
  tcp_connection_->SetConnectHandler(NewPermanentCallback(
      this, &SslConnection::TcpConnectionConnectHandler), true);
  tcp_connection_->SetCloseHandler(NewPermanentCallback(
      this, &SslConnection::TcpConnectionCloseHandler), true);
  tcp_connection_->SetReadHandler(NewPermanentCallback(
      this, &SslConnection::TcpConnectionReadHandler), true);
  tcp_connection_->SetWriteHandler(NewPermanentCallback(
      this, &SslConnection::TcpConnectionWriteHandler), true);
  set_state(CONNECTING);
  is_server_side_ = true;
  // resume from the point where the TCP is connected and SSL handshake should start
  TcpConnectionConnectHandler();
}
bool SslConnection::Connect(const HostPort& remote_addr) {
  CHECK_NULL(tcp_connection_);
  tcp_connection_ = new TcpConnection(selector_);
  tcp_connection_->SetConnectHandler(NewPermanentCallback(
      this, &SslConnection::TcpConnectionConnectHandler), true);
  tcp_connection_->SetCloseHandler(NewPermanentCallback(
      this, &SslConnection::TcpConnectionCloseHandler), true);
  tcp_connection_->SetReadHandler(NewPermanentCallback(
      this, &SslConnection::TcpConnectionReadHandler), true);
  tcp_connection_->SetWriteHandler(NewPermanentCallback(
      this, &SslConnection::TcpConnectionWriteHandler), true);
  set_state(CONNECTING);
  is_server_side_ = false;
  ICONNLOG << "Connecting to " << remote_addr;
  if ( !tcp_connection_->Connect(remote_addr) ) {
    ECONNLOG << "Connect failed for remote address: " << remote_addr;
    delete tcp_connection_;
    tcp_connection_ = NULL;
    set_state(DISCONNECTED);
    return false;
  }
  // The next thing will be:
  //  - TcpConnectionConnectHandler: TCP is connected and SSL handshake should start
  //  - TcpConnectionCloseHandler: TCP broken, clear everything.
  return true;
}
void SslConnection::FlushAndClose() {
  set_state(FLUSHING);
  RequestWriteEvents(true);
}
void SslConnection::ForceClose() {
  SslClear();
  tcp_connection_->ForceClose();
}
bool SslConnection::SetSendBufferSize(int size) {
  CHECK_NOT_NULL(tcp_connection_);
  return tcp_connection_->SetSendBufferSize(size);
}
bool SslConnection::SetRecvBufferSize(int size) {
  CHECK_NOT_NULL(tcp_connection_);
  return tcp_connection_->SetRecvBufferSize(size);
}
void SslConnection::RequestReadEvents(bool enable) {
  CHECK_NOT_NULL(tcp_connection_);
  return tcp_connection_->RequestReadEvents(enable);
}
void SslConnection::RequestWriteEvents(bool enable) {
  CHECK_NOT_NULL(tcp_connection_);
  return tcp_connection_->RequestWriteEvents(enable);
}
const HostPort& SslConnection::local_address() const {
  static const HostPort empty_address;
  return tcp_connection_ == NULL ? empty_address :
                                   tcp_connection_->local_address();
}
const HostPort& SslConnection::remote_address() const {
  static const HostPort empty_address;
  return tcp_connection_ == NULL ? empty_address :
                                   tcp_connection_->remote_address();
}
string SslConnection::PrefixInfo() const {
  CHECK_NOT_NULL(tcp_connection_);
  return tcp_connection_->PrefixInfo() +
         "[SSL: " + StateName() + "]: ";
}


bool SslConnection::HandleReadEvent(const SelectorEventData& event) {
  FCONNLOG << "TODO(cosmin): this function should never be called";
  return true;
}
bool SslConnection::HandleWriteEvent(const SelectorEventData& event) {
  FCONNLOG << "TODO(cosmin): this function should never be called";
  return true;
}
bool SslConnection::HandleErrorEvent(const SelectorEventData& event) {
  FCONNLOG << "TODO(cosmin): this function should never be called";
  return true;
}
void SslConnection::Close() {
  CHECK_NOT_NULL(tcp_connection_);
  tcp_connection_->ForceClose();
}
int SslConnection::GetFd() const {
  return INVALID_FD_VALUE;
}


void SslConnection::TcpConnectionConnectHandler() {
  ICONNLOG << "TcpConnection established, initializing SSL layer..";
  // initialize SSL structures here
  if ( !SslInitialize(is_server_side_) ) {
    ECONNLOG << "SslInitialize failed, closing underlying TCP connection..";
    ForceClose();
    return;
  }
  // Our state is still CONNECTING, the next thing is:
  //  - TCP invokes TcpConnectionWriteHandler, and the SSL handshake will begin
}
bool SslConnection::TcpConnectionReadHandler() {
  DCONNLOG << "SslConnection::TcpConnectionReadHandler";
  // Read from TCP --> write to SSL
  while ( !tcp_connection_->inbuf()->IsEmpty() ) {
    char buf[1024];
    int32 read = tcp_connection_->inbuf()->Read(buf, sizeof(buf));
    int32 write = BIO_write(p_bio_read_, buf, read);
    if ( write < read ) {
      // we use memory BIO, no reason for BIO_write to fail
      ECONNLOG << "BIO_write failed, closing connection";
      ForceClose();
      return false;
    }

    ssl_in_count_ += write;
    WCONNLOG << "BIO write: " << write << " bytes"
                ", BIO total: in " << ssl_in_count_
             << " / out " << ssl_out_count_
             << " TCP buffers: in " << tcp_connection_->inbuf()->Size()
             << " / out " << tcp_connection_->outbuf()->Size();
    DCONNLOG << "TCP >>>> " << write << " bytes >>>> SSL";
  }
  if ( state() == CONNECTING ) {
    // still in handshake
    SslHandshake();
    return true;
  }

  if ( write_blocked_on_read_ ) {
    DCONNLOG << "SSL >>>> write in progress.";
    // an SSL_write is in progress, we cannot SSL_read.
    RequestWriteEvents(true);
    return true;
  }

  //NOTE: SSL_pending looks only inside SSL layer, and not into BIO buffer.
  //      So even if you have tons of data in BIO, SSL_pending still returns 0.

  // Read from SSL --> write to inbuf()
  while ( BIO_pending(p_bio_read_) ) {
    // If there is no data in p_bio_read_ then avoid calling SSL_read because
    // it would return WANT_READ and we'll get read_blocked.
    // WCONNLOG << "Going to SSL_read from bio_data: " << SslPrintableBio(p_bio_read_);
    char* buffer;
    int32 scratch;
    inbuf()->GetScratchSpace(&buffer, &scratch);
    const int32 cb = SSL_read(p_ssl_, buffer, scratch);

    //WCONNLOG << "SSL read: " << read << " bytes"
    //            " => " << SslErrorName(SSL_get_error(p_ssl_, read));
    //WCONNLOG << "After SSL_read remaining bio_data: " << SslPrintableBio(p_bio_read_);
    read_blocked_ = false;
    read_blocked_on_write_ = false;
    if ( cb < 0 ) {
      int error = SSL_get_error(p_ssl_, cb);
      switch(error) {
        case SSL_ERROR_NONE:
          break;
        case SSL_ERROR_WANT_READ:
          read_blocked_ = true;
          break;
        case SSL_ERROR_WANT_WRITE:
          read_blocked_on_write_ = true;
          RequestWriteEvents(true);
          break;
        case SSL_ERROR_ZERO_RETURN:
          // End of data. We need to SSL_shutdown.
          FlushAndClose();
          return true;
        default:
          // TODO(cosmin): make error, non fatal
          ECONNLOG << "SSL_read fatal, SSL_get_error => "
                   << error << " " << SslErrorName(error)
                   << " , " << SslLastError()
                   << " , closing connection";
          ForceClose();
          return false;
      };
      inbuf()->ConfirmScratch(0);
      break;
    }
    // SSL_read was successful
    inbuf()->ConfirmScratch(cb);
    DCONNLOG << "SSL >>>> " << cb << " bytes >>>> APP";
  }

  if ( !read_blocked_ && !outbuf()->IsEmpty() ) {
    // the write has been stopped due to read_blocked_
    RequestWriteEvents(true);
  }

  // in FLUSHING state discard all input. We're waiting for the outbuf() to
  // become empty so we can gracefully shutdown SSL.
  if ( state() == FLUSHING ) {
    inbuf()->Clear();
  }

  // skip InvokeReadHandler if inbuf() is empty
  if ( inbuf()->IsEmpty() ) {
    return true;
  }

  // ask application to read data from our inbuf()
  return InvokeReadHandler();
}
bool SslConnection::TcpConnectionWriteHandler() {
  DCONNLOG << "SslConnection::TcpConnectionWriteHandler";
  bool success = true;

  if ( state() == CONNECTING ) {
    SslHandshake();
  } else if (read_blocked_ || read_blocked_on_write_) {
    // A partial SSL_read is in progress. DON'T use SSL_write! or it will
    //  corrupt internal ssl structures.
    // If we don't write anything to TCP, the write event will be stopped.
    // The ReadHandler will test outbuf non empty and re-enable write.
  } else {
    // ask application to write something in our outbuf()
    // [don't ask if we're FLUSHING]
    if ( state() == CONNECTED ) {
      success = InvokeWriteHandler();
    }

    // Read from outbuf() --> write to SSL
    while ( !outbuf()->IsEmpty() ) {
      outbuf()->MarkerSet();
      char buf[1024];
      int32 read = outbuf()->Read(buf, sizeof(buf));
      WCONNLOG << "APP read: " << read << " bytes";
      CHECK_GT(read, 0); // SSL_write() behavior is undefined on 0 bytes
                         // besides, we've already checked !outbuf()->IsEmpty()
      int write = SSL_write(p_ssl_, buf, read);
      // write = the number of encrypted bytes written in BIO, always > read
      //WCONNLOG << "SSL write: " << write << " bytes"
      //            " => " << SslErrorName(SSL_get_error(p_ssl_, write));
      //WCONNLOG << "After SSL_write BIO contains data: " << SslPrintableBio(p_bio_write_);
      DCONNLOG << "SSL <<<< " << write << " bytes <<<< APP";
      write_blocked_on_read_ = false;
      if ( write <= 0 ) {
        outbuf()->MarkerRestore();
        int error = SSL_get_error(p_ssl_, write);
        ECONNLOG << "SSL_write failed, SSL_get_error => "
                 << error << " " << SslErrorName(error)
                 << " , " << SslLastError();
        switch(error) {
          case SSL_ERROR_WANT_READ:
            write_blocked_on_read_ = true;
            // we need more data in p_bio_read_ so we're just gonna wait for
            // ReadHandler to happen
            return true;
          case SSL_ERROR_WANT_WRITE:
            // p_bio_write_ is probably full.
            // But we use memory BIO, so it should never happen.
            break;
          default:
            ECONNLOG << "SSL_write fatal, closing connection";
            ForceClose();
            return false;
        };
        break;
      }
      // SSL_write was successful
      outbuf()->MarkerClear();
    }
  }

  // WCONNLOG << "After all SSL_write BIO contains data: "
  //  << SslPrintableBio(p_bio_write_);

  // Read from SSL --> write to TCP
  while ( BIO_pending(p_bio_write_) > 0 ) {
    char buf[1024];
    int32 read = BIO_read(p_bio_write_, buf, sizeof(buf));
    ssl_out_count_ += (read < 0 ? 0 : read);
    //WCONNLOG << "BIO read: " << read << " bytes, BIO total:"
    //            " in " << ssl_in_count_ << " / out " << ssl_out_count_;
    if ( read <= 0 ) {
      // should never happen, we use plain memory BIO
      ECONNLOG << "BIO_read failed, closing connection";
      ForceClose();
      return false;
    }
    int32 write = tcp_connection_->outbuf()->Write(buf, read);
    CHECK(write == read) << "Memory stream should be unlimited";
    DCONNLOG << "TCP <<<< " << read << " bytes <<<< SSL";
  }

  // if we sent every piece of data, shutdown SSL
  if ( state() == FLUSHING && outbuf()->IsEmpty() ) {
    SslShutdown();
    tcp_connection_->FlushAndClose();
    // Now it's just TCP's job to flush and close
  }

  return success;
}
void SslConnection::TcpConnectionCloseHandler(int err, CloseWhat what) {
  if ( what != CLOSE_READ_WRITE ) {
    // ignore partial close of TCP layer
    SslShutdown();
    return;
  }
  // TCP completely closed
  WCONNLOG << "Underlying TcpConnection closed."
              " err: " << err << " , what: " << CloseWhatName(what);
  set_state(DISCONNECTED);
  InvokeCloseHandler(err, what);
}


void SslConnection::HandleTimeoutEvent(int64 timeout_id) {
}
//static
const char* SslConnection::SslErrorName(int err) {
  switch ( err ) {
    CONSIDER(SSL_ERROR_NONE);
    CONSIDER(SSL_ERROR_SSL);
    CONSIDER(SSL_ERROR_WANT_READ);
    CONSIDER(SSL_ERROR_WANT_WRITE);
    CONSIDER(SSL_ERROR_WANT_X509_LOOKUP);
    CONSIDER(SSL_ERROR_SYSCALL);
    CONSIDER(SSL_ERROR_ZERO_RETURN);
    CONSIDER(SSL_ERROR_WANT_CONNECT);
    CONSIDER(SSL_ERROR_WANT_ACCEPT);
    default:
      return "UNKNOWN";
  }
}
//static
const char* SslConnection::SslWantName(int want) {
  switch ( want ) {
    CONSIDER(SSL_NOTHING);
    CONSIDER(SSL_WRITING);
    CONSIDER(SSL_READING);
    CONSIDER(SSL_X509_LOOKUP);
    default:
      return "UNKNWOWN";
  }
}
//static
std::string SslConnection::SslLastError() {
  vector<string> errors;
  errors.push_back("Error stack:");
  while ( true ) {
    int line;
    const char* file;
    const int e = ERR_get_error_line(&file, &line);
    if ( e == 0 ) {
      break;
    }
    char text[512] = {0,};
    ERR_error_string_n(e, text, sizeof(text));
    errors.push_back(strutil::StringPrintf(" %s:%s:%d", text, file, line));
  }
  errors.push_back(strutil::StringPrintf("General error: %s",
                                         GetLastSystemErrorDescription()));
  return strutil::JoinStrings(errors, "\n");
}

//static
const void SslConnection::SslLibraryInit() {
  SSL_library_init();                      /* initialize library */
  SSL_load_error_strings();                /* readable error messages */
  ERR_load_SSL_strings();
  ERR_load_CRYPTO_strings();
  ERR_load_crypto_strings();
  // actions_to_seed_PRNG();
}
//static
X509* SslConnection::SslLoadCertificateFile(const string& filename) {
  // Load certificate file.
  FILE * f = ::fopen(filename.c_str(), "r");
  if ( f == NULL ) {
    LOG_ERROR << "Cannot find certificate file: [" << filename << "]";
    return NULL;
  }
  X509* certificate = NULL;
  if ( NULL == PEM_read_X509(f, &certificate, NULL, NULL) ) {
    LOG_ERROR << "PEM_read_X509 failed to load certificate from file: ["
              << filename << "]";
    fclose(f);
    return NULL;
  }
  fclose(f);
  CHECK_NOT_NULL(certificate);
  LOG_INFO << "SSL Loaded certificate file: [" << filename << "]";
  return certificate;
}
//static
EVP_PKEY* SslConnection::SslLoadPrivateKeyFile(const string& filename) {
  // Load private key file.
  FILE * f = ::fopen(filename.c_str(),"r");
  if ( f == NULL ) {
    LOG_ERROR << "Cannot find key file: [" << filename << "]";
    return NULL;
  }
  EVP_PKEY* key = NULL;
  if ( NULL == PEM_read_PrivateKey(f, &key, NULL, NULL) ) {
    LOG_ERROR << "PEM_read_PrivateKey failed to load key from file: ["
             << filename << "]";
    fclose(f);
    return NULL;
  }
  fclose(f);
  CHECK_NOT_NULL(key);
  LOG_INFO << "SSL Loaded private key file: [" << filename << "]";
  return key;
}

//static
X509* SslConnection::SslDuplicateX509(const X509& src) {
  X509* s = const_cast<X509*>(&src);
  X509* d = X509_dup(s);
  CHECK_NOT_NULL(d);
  return d;
}
//static
EVP_PKEY* SslConnection::SslDuplicateEVP_PKEY(const EVP_PKEY& src) {
  // TODO(cosmin): code copied from:
  //  http://www.mail-archive.com/openssl-users@openssl.org/msg17614.html
  EVP_PKEY* k = const_cast<EVP_PKEY*>(&src);
  k->references++;
  return k;

  // TODO(cosmin): code copied from:
  //  http://www.mail-archive.com/openssl-users@openssl.org/msg17680.html
  /*
  EVP_PKEY* pKey = const_cast<EVP_PKEY*>(&src);
  EVP_PKEY* pDupKey = EVP_PKEY_new();
  RSA* pRSA = EVP_PKEY_get1_RSA(pKey);
  RSA* pRSADupKey = NULL;
  if( eKeyType == eKEY_PUBLIC ) // Determine the type of the "source" EVP_PKEY
    pRSADupKey = RSAPublicKey_dup(pRSA);
  else
    pRSADupKey = RSAPrivateKey_dup(pRSA);
  RSA_free(pRSA);
  EVP_PKEY_set1_RSA(pDupKey, pRSADupKey);
  RSA_free(pRSADupKey);
  return pDupKey;
  */
}
//static
string SslConnection::SslPrintableBio(BIO* bio) {
  char * bio_data = NULL;
  long bio_data_size = BIO_get_mem_data(bio, &bio_data);
  return strutil::PrintableDataBufferHexa(bio_data, bio_data_size);
}
//static
SSL_CTX* SslConnection::SslCreateContext(const string& certificate_filename,
                                         const string& key_filename) {
  SslLibraryInit();
  X509* ssl_certificate = NULL;
  if ( certificate_filename != "" ) {
    ssl_certificate = SslLoadCertificateFile(certificate_filename);
    if ( ssl_certificate == NULL ) {
      LOG_ERROR << "SslLoadCertificateFile failed for file: ["
                << certificate_filename << "]";
      return NULL;
    }
  }
  EVP_PKEY* ssl_key = NULL;
  if ( key_filename != "" ) {
    ssl_key = SslLoadPrivateKeyFile(key_filename);
    if ( ssl_key == NULL ) {
      LOG_ERROR << "SslLoadPrivateKeyFile failed for file: ["
                << key_filename << "]";
      X509_free(ssl_certificate);
      return NULL;
    }
  }
  SSL_CTX* ssl_ctx = SSL_CTX_new(SSLv23_method());
  if ( ssl_ctx == NULL ) {
    LOG_ERROR << "SSL_CTX_new failed: " << SslLastError();
    X509_free(ssl_certificate);
    EVP_PKEY_free(ssl_key);
    return NULL;
  }
  const long ssl_ctx_mode = SSL_CTX_get_mode(ssl_ctx);
  const long ssl_new_ctx_mode = ssl_ctx_mode |
                                SSL_MODE_ENABLE_PARTIAL_WRITE |
                                SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER;
  const long result = SSL_CTX_set_mode(ssl_ctx, ssl_new_ctx_mode);
  CHECK_EQ(result, ssl_new_ctx_mode);

  // The server needs certificate and key.
  // The client may optionally use certificate and key.
  if ( ssl_certificate != NULL ) {
    if ( SSL_CTX_use_certificate(ssl_ctx, ssl_certificate) <= 0 ) {
      LOG_ERROR << "SSL_CTX_use_certificate failed: " << SslLastError();
      X509_free(ssl_certificate);
      EVP_PKEY_free(ssl_key);
      SSL_CTX_free(ssl_ctx);
      return NULL;
    }
    // Now the 'ssl_certificate' is part of 'context'.
    // It will get freed when the 'context' is freed.
    ssl_certificate = NULL;
  }
  if ( ssl_key != NULL ) {
    if ( SSL_CTX_use_PrivateKey(ssl_ctx, ssl_key) <= 0 ) {
      LOG_ERROR << "SSL_CTX_use_PrivateKey failed: " << SslLastError();
      X509_free(ssl_certificate);
      EVP_PKEY_free(ssl_key);
      SSL_CTX_free(ssl_ctx);
      return NULL;
    }
    // Now the 'ssl_key' is part of 'context'.
    // It will get freed when the 'context' is freed.
    ssl_key = NULL;
  }

  return ssl_ctx;
}
//static
void SslConnection::SslDeleteContext(SSL_CTX* ssl_ctx) {
  if ( ssl_ctx == NULL ) {
    return;
  }
  SSL_CTX_free(ssl_ctx);
}



bool SslConnection::SslInitialize(bool is_server) {
  DCONNLOG << "Initializing SSL ...";

  CHECK_NULL(p_ctx_);
  p_ctx_ = ssl_params_.ssl_context_;
  if ( p_ctx_ == NULL ) {
    ECONNLOG << "no SSL_CTX provided";
    return false;
  }

  CHECK_NULL(p_ssl_);
  p_ssl_ = SSL_new(p_ctx_);
  CHECK_NOT_NULL(p_ssl_);

  p_bio_read_ = BIO_new(BIO_s_mem());
  CHECK_NOT_NULL(p_bio_read_);
  p_bio_write_ = BIO_new(BIO_s_mem());
  CHECK_NOT_NULL(p_bio_write_);

  SSL_set_bio(p_ssl_, p_bio_read_, p_bio_write_);
  if ( is_server ) {
    SSL_set_accept_state(p_ssl_);
  } else {
    SSL_set_connect_state(p_ssl_);
  }
  return true;
}
void SslConnection::SslClear() {
  if ( p_ssl_ ) {
    // SSL_free also deletes the associated BIOs
    SSL_free(p_ssl_);
    p_ssl_ = NULL;
    //BIO_free_all(p_bio_read_);
    p_bio_read_ = NULL;
    //BIO_free_all(p_bio_write_);
    p_bio_write_ = NULL;
  }
  if ( p_bio_read_ ) {
    BIO_free_all(p_bio_read_);
    p_bio_read_ = NULL;
  }
  if ( p_bio_write_ ) {
    BIO_free_all(p_bio_write_);
    p_bio_write_ = NULL;
  }
  // We do not own the SSL_CTX.
  // We only received this pointer by SslConnectionParams.
  p_ctx_ = NULL;

  CHECK_NULL(p_ssl_);
  CHECK_NULL(p_bio_read_);
  CHECK_NULL(p_bio_write_);
  CHECK_NULL(p_ctx_);
  //TODO(cosmin): the SslConnection is not reusable, for the time being.
  //handshake_finished_ = false;
}
void SslConnection::SslHandshake() {
  if ( handshake_finished_ ) {
    return;
  }
  DCONNLOG << "SslHandshake...";
  if ( SSL_is_init_finished(p_ssl_) ) {
    // SSL completed the handshake but did we empty the ssl buffers?
    if ( BIO_pending(p_bio_write_) > 0 ) {
      DCONNLOG << "SslHandshake finished. Delaying connect handler... because"
                  " BIO_pending(p_bio_write_): " << BIO_pending(p_bio_write_);
      RequestWriteEvents(true);
      return;
    }
    DCONNLOG << "SslHandshake finished. Invoking connect handler.";
    handshake_finished_ = true;
    set_state(CONNECTED);
    selector_->RunInSelectLoop(NewCallback(this, &SslConnection::InvokeConnectHandler));
    return;
  }
  int result = SSL_do_handshake(p_ssl_);
  DCONNLOG << "SslHandshake SSL_do_handshake => " << result;
  if ( result < 1 ) {
    int error = SSL_get_error(p_ssl_, result);
    DCONNLOG << "SslHandshake SSL_get_error => "
             << error << " " << SslErrorName(error)
             << " , BIO_pending(p_bio_write_): " << BIO_pending(p_bio_write_)
             << " , BIO_pending(p_bio_read_): " << BIO_pending(p_bio_read_)
             << " , error: " << SslLastError();
    if ( error != SSL_ERROR_WANT_READ &&
         error != SSL_ERROR_WANT_WRITE ) {
        ECONNLOG << "SSL_do_handshake failed: " << SslErrorName(error)
                 << " , error: " << SslLastError();
        ForceClose();
        return;
    }
    // Handshake still in progress..
    RequestWriteEvents(true);
    // Next thing:
    //  - TcpConnectionWriteHandler will read from SSl --> write to TCP and
    //                              will call SslHandshake again.
    return;
  }
  // The handshake is completed for this endpoint(SSL_do_handshake returned 1).
  // But maybe we need to send some data to the other endpoint.
  int ssl_want = SSL_want(p_ssl_);
  DCONNLOG << "ssl_want: " << ssl_want << " " << SslWantName(ssl_want)
           << " , BIO_pending(p_bio_write_): " << BIO_pending(p_bio_write_)
           << " , BIO_pending(p_bio_read_): " << BIO_pending(p_bio_read_);
  RequestWriteEvents(true);
  // Next thing:
  //  - TcpConnectionWriteHandler will read from SSl --> write to TCP and
  //                              will call SslHandshake again.
}
void SslConnection::SslShutdown() {
  if ( p_ssl_ == NULL ) {
    return;
  }
  int result = SSL_shutdown(p_ssl_);
  if ( result < 0 ) {
    int error = SSL_get_error(p_ssl_, result);
    ECONNLOG << "SSL_shutdown => " << SslErrorName(error)
             << " , error: " << SslLastError();
  }
  WCONNLOG << "SslShutdown performed, BIO_pending(p_bio_write_): "
           << BIO_pending(p_bio_write_);
  // Read from SSL --> write to TCP (the last SSL close signal)
  while ( BIO_pending(p_bio_write_) > 0 ) {
    char buf[1024];
    int32 read = BIO_read(p_bio_write_, buf, sizeof(buf));
    ssl_out_count_ += (read < 0 ? 0 : read);
    //WCONNLOG << "BIO read: " << read << " bytes, BIO total:"
    //            " in " << ssl_in_count_ << " / out " << ssl_out_count_;
    if ( read <= 0 ) {
      // should never happen, we use plain memory BIO
      ECONNLOG << "BIO_read failed, closing connection";
      ForceClose();
      return;
    }
    int32 write = tcp_connection_->outbuf()->Write(buf, read);
    CHECK(write == read) << "Memory stream should be unlimited";
    DCONNLOG << "TCP <<<< " << read << " bytes <<<< SSL";
  }
}

#endif    // Open SSL
}
//////////////////////////////////////////////////////////////////////
