// Copyright (c) 2010, Whispersoft s.r.l.
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
// Authors: Cosmin Tudorache
//

#include <config.h>

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

#if defined(HAVE_SYS_EPOLL_H)
#include "sys/epoll.h"
#elif defined(HAVE_POLL_H)
#include "poll.h"
#elif defined(HAVE_SYS_POLL_H)
#include "sys/poll.h"
#else
#error "need poll or epoll to compile"
#endif


#include <whisperlib/base/errno.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/scoped_ptr.h>
#include <whisperlib/base/timer.h>
#include <whisperlib/base/gflags.h>

#include <whisperlib/net/udp_connection.h>

DECLARE_bool(net_connection_debug);
#define IF_NET_DEBUG if ( !FLAGS_net_connection_debug ); else

#define DCONNLOG  IF_NET_DEBUG DLOG_INFO << this->PrefixInfo()
#define ICONNLOG  IF_NET_DEBUG LOG_INFO << this->PrefixInfo()
#define WCONNLOG  IF_NET_DEBUG LOG_WARNING << this->PrefixInfo()
#define ECONNLOG               LOG_ERROR << this->PrefixInfo()
#define FCONNLOG               LOG_FATAL << this->PrefixInfo()


namespace net {

UdpConnection::UdpConnection(Selector* selector)
  : Selectable(selector),
    read_handler_(NULL),
    write_handler_(NULL),
    close_handler_(NULL),
    own_read_handler_(false),
    own_write_handler_(false),
    own_close_handler_(false),
    state_(DISCONNECTED),
    fd_(INVALID_FD_VALUE),
    local_addr_(0, 0),
    data_buffer_(new uint8[kDatagramMaxSize]),
    data_buffer_size_(kDatagramMaxSize),
    out_queue_(),
    in_queue_(),
    count_bytes_written_(0),
    count_bytes_read_(0),
    count_datagrams_sent_(0),
    count_datagrams_read_(0) {
  CHECK_NOT_NULL(data_buffer_) << " Out of memory";
}
UdpConnection::~UdpConnection() {
  InternalClose(0, true);
  CHECK_EQ(state(), DISCONNECTED);
  CHECK_EQ(fd_, INVALID_FD_VALUE);
  DetachAllHandlers();
  ClearInQueue();
  ClearOutQueue();
  delete data_buffer_;
  data_buffer_ = NULL;
}

bool UdpConnection::Open(const HostPort& local_addr) {
  CHECK(state() == DISCONNECTED) << " Invalid state";
  CHECK(fd_ == INVALID_FD_VALUE) << " Already open";

  // create socket
  fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if ( fd_ < 0 ) {
    ECONNLOG << "::socket failed, err: " << GetLastSystemErrorDescription();
    return false;
  }

  // set socket options: non-blocking, ...
  if ( !SetSocketOptions() ) {
    ECONNLOG << "SetSocketOptions failed, aborting Open.. ";
    ::close(fd_);
    fd_ = INVALID_FD_VALUE;
    return false;
  }

  // bind to local address
  struct sockaddr_storage addr;
  const bool is_ipv6 = local_addr.SockAddr(&addr);
  if ( ::bind(fd_, reinterpret_cast<const sockaddr*>(&addr),
              is_ipv6 ? sizeof(addr) : sizeof(struct sockaddr)) ) {
    ECONNLOG << "Error binding fd: " << fd_ << " to "
             << local_addr << " : " << GetLastSystemErrorDescription();
    ::close(fd_);
    fd_ = INVALID_FD_VALUE;
    return false;
  }

  // register with selector
  if ( !selector_->Register(this) ) {
    ECONNLOG << "Failed to register with selector, aborting Open.. ";
    ::close(fd_);
    fd_ = INVALID_FD_VALUE;
    return false;
  }

  local_addr_ = local_addr;
  set_state(CONNECTED);

  LearnLocalAddress();

  RequestWriteEvents(true);
  RequestReadEvents(true);

  DCONNLOG << "Open succeeded";
  return true;
}

bool UdpConnection::IsOpen() const {
  return fd_ != INVALID_FD_VALUE;
}

bool UdpConnection::SetSendBufferSize(int size) {
  if ( ::setsockopt(fd_, SOL_SOCKET, SO_SNDBUF,
                    reinterpret_cast<const char *>(&size), sizeof(size)) ) {
    ECONNLOG << "::setsockopt failed: " << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}
bool UdpConnection::SetRecvBufferSize(int size) {
  if ( ::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF,
                    reinterpret_cast<const char *>(&size), sizeof(size)) ) {
    ECONNLOG << "::setsockopt failed: " << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}

void UdpConnection::SendDatagram(const io::MemoryStream& data,
    const net::HostPort& addr) {
  CHECK_EQ(state(), CONNECTED);
  CHECK_LT(data.Size(), data_buffer_size_);
  out_queue_.push_back(new Datagram(data, addr));
  RequestWriteEvents(true);
}
void UdpConnection::SendDatagram(const void* data, uint32 data_size,
    const net::HostPort& addr) {
  CHECK_EQ(state(), CONNECTED);
  CHECK_LT(data_size, data_buffer_size_);
  out_queue_.push_back(new Datagram(data, data_size, addr));
  RequestWriteEvents(true);
}

const UdpConnection::Datagram* UdpConnection::PopRecvDatagram() {
  if ( in_queue_.empty() ) {
    return NULL;
  }
  const Datagram* p = in_queue_.front();
  in_queue_.pop_front();
  return p;
}

void UdpConnection::FlushAndClose() {
  if ( fd_ == INVALID_FD_VALUE ) {
    CHECK_EQ(state(), DISCONNECTED);
    return;
  }

  set_state(FLUSHING);
  RequestWriteEvents(true);
  // NOTE: when out_queue_ gets empty we execute ::shutdown(write)
  //       and set write_closed_ = true

}
void UdpConnection::ForceClose() {
  InternalClose(0, true);
}

string UdpConnection::PrefixInfo() const {
  return strutil::StringPrintf("[%s (fd: %d)] ",
      local_addr_.ToString().c_str(),
      fd_);
}

bool UdpConnection::SetSocketOptions() {
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
  return true;
}
void UdpConnection::LearnLocalAddress() {
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);
  if ( ::getsockname(fd_, reinterpret_cast<sockaddr*>(&addr), &len) ) {
    ECONNLOG << "::getsockname failed: " << GetLastSystemErrorDescription();
    return;
  }
  local_addr_ = HostPort(&addr);
}

// static
int UdpConnection::ExtractSocketErrno(int fd) {
  int error;
  socklen_t len = sizeof(error);
  if ( getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) ) {
    return errno;
  }
  return error;
}
int UdpConnection::ExtractSocketErrno() {
  return ExtractSocketErrno(fd_);
}


void UdpConnection::ClearInQueue() {
  while ( !in_queue_.empty() ) {
    Datagram* p = in_queue_.front();
    in_queue_.pop_front();
    delete p;
  }
}
void UdpConnection::ClearOutQueue() {
  while ( !out_queue_.empty() ) {
    Datagram* p = out_queue_.front();
    out_queue_.pop_front();
    delete p;
  }
}

void UdpConnection::InternalClose(int err, bool call_close_handler) {
  if ( fd_ == INVALID_FD_VALUE ) {
    return;
  }
  DCONNLOG << "Unregistering connection.. ";
  selector_->Unregister(this);
  ::shutdown(fd_, SHUT_RDWR);
  DCONNLOG << "Performing the ::close... ";
  if ( ::close(fd_) < 0 ) {
    ECONNLOG << "Error closing fd: " << fd_ << " err: "
             << GetLastSystemErrorDescription();
  }
  fd_ = INVALID_FD_VALUE;
  set_state(DISCONNECTED);
  ClearInQueue();
  ClearOutQueue();
  if ( call_close_handler ) {
    InvokeCloseHandler(err);
  }
}

bool UdpConnection::HandleReadEvent(const SelectorEventData& event) {
  CHECK(state() != DISCONNECTED) << "Invalid state: " << StateName();

  // Read from network into inbuf_
  while ( true ) {
    struct sockaddr_storage addr = {0,};
    socklen_t addr_len = sizeof(addr);
    int32 read = ::recvfrom(fd_, data_buffer_, data_buffer_size_, 0,
                            reinterpret_cast<sockaddr*>(&addr), &addr_len);
    if ( read < 0 ) {
      if ( GetLastSystemError() == EAGAIN ) {
        read = 0;
      } else {
        const int err = ExtractSocketErrno();
        ECONNLOG << "Closing connection because ::recv failed, "
                 << " system: " << GetLastSystemErrorDescription()
                 << " - internal err no: " << err;
        InternalClose(err, true);
        return false;
      }
    }
    if ( read == 0 ) {
      break;
    }
    net::HostPort a(&addr);
    in_queue_.push_back(new Datagram(data_buffer_, read, a));

    DCONNLOG << "Received datagram: #" << read << " bytes, on port: "
             << local_addr().port() << ", from addr: " << a
             << ", in_queue_.size: " << in_queue_.size();

    // update local stats
    count_bytes_read_ += read;
    count_datagrams_read_++;
  }

  if ( !InvokeReadHandler() ) {
    WCONNLOG << "Closing connection because read_handler_ said so";
    InternalClose(0, true);
    return false;
  }
  return true;
}
bool UdpConnection::HandleWriteEvent(const SelectorEventData& event) {
  CHECK(state() != DISCONNECTED) << "Invalid state: " << StateName();
  if ( !InvokeWriteHandler() ) {
    WCONNLOG << "Closing because write_handler_ said so";
    InternalClose(0, true);
    return false;
  }

  if ( out_queue_.empty() ) {
    RequestWriteEvents(false);
    if ( state() == FLUSHING ) {
      // FLUSHING finished sending all buffered data.
      WCONNLOG << "Flushing finished, closing connection.";
      InternalClose(0, true);
      return false;
    }
    return true;
  }

  Datagram* p = out_queue_.front();
  out_queue_.pop_front();
  scoped_ptr<Datagram> auto_del_p(p);

  CHECK_LT(p->data_.Size(), data_buffer_size_);
  int32 read = p->data_.Read(data_buffer_, data_buffer_size_);
  CHECK(p->data_.IsEmpty());

  struct sockaddr_storage addr;
  const bool is_ipv6 = p->addr_.SockAddr(&addr);

  DCONNLOG << "Sending datagram: #" << read << " bytes, to addr: " << p->addr_
           << ", out_queue_.size: " << in_queue_.size();

  int32 write = ::sendto(fd_, data_buffer_, read, 0,
                         reinterpret_cast<const sockaddr*>(&addr),
                         is_ipv6 ? sizeof(addr) : sizeof(struct sockaddr));
  if ( write < 0 ) {
    const int err = ExtractSocketErrno();
    ECONNLOG << "Closing connection because ::send failed: "
             << " system: " << GetLastSystemErrorDescription()
             << " - internal err no: " << err;
    InternalClose(err, true);
    return false;
  }

  // update local stats
  count_bytes_written_ += write;
  count_datagrams_sent_++;

  if ( local_addr_.port() == 0 ) {
    // local port was 0 on Open(), so a random free port was
    // chosen on the first ::send()
    LearnLocalAddress();
  }
  return true;
}

bool UdpConnection::HandleErrorEvent(const SelectorEventData& event) {
  const int events = event.internal_event_;
#ifdef HAVE_SYS_EPOLL_H
  if ( (events & EPOLLERR) == EPOLLERR ) {
#else
  if ( (events & POLLERR) == POLLERR ) {
#endif
    const int err = ExtractSocketErrno();
    ECONNLOG << " HandleErrorEvent err=" << err
             << " - " << GetSystemErrorDescription(err);
    InternalClose(err, true);
    return false;
  }
  ECONNLOG << "HandleErrorEvent: unknown event: 0x" << std::hex << events;
  return true;
}
void UdpConnection::Close() {
  // this call comes from Selectable interface
  InternalClose(0, true);
}

} // namespace net
