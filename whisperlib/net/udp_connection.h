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
// Authors: Cosmin Tudorache
//

#ifndef __NET_BASE_UDP_CONNECTION_H__
#define __NET_BASE_UDP_CONNECTION_H__

#include <string>
#include <list>
#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>

#include <whisperlib/io/buffer/memory_stream.h>

#include <whisperlib/net/address.h>
#include <whisperlib/net/selectable.h>
#include <whisperlib/net/timeouter.h>

namespace net {

class UdpConnection : private Selectable {
 public:
  struct Datagram {
    io::MemoryStream data_;
    net::HostPort addr_;
    Datagram(const io::MemoryStream& data, const net::HostPort& addr)
      : data_(), addr_(addr) {
      data_.AppendStreamNonDestructive(&data);
    }
    Datagram(const void* data, uint32 data_size, const net::HostPort& addr)
      : data_(), addr_(addr) {
      data_.Write(data, data_size);
    }
  };

 public:
  enum State {
    DISCONNECTED,
    CONNECTED,
    FLUSHING,
  };
  static const char* StateName(State s) {
    switch (s) {
      CONSIDER(DISCONNECTED);
      CONSIDER(CONNECTED);
      CONSIDER(FLUSHING);
    }
    return "UNKNOWN";
  }
  // Observed max IP packet size (by wireshark on Linux 2.6.24-27).
  //static const uint32 kDatagramMaxSize = 16416;
  // IP limit imposed by "size" field (16 bit).
  static const uint32 kDatagramMaxSize = 65507;

 public:
  UdpConnection(Selector* selector);
  virtual ~UdpConnection();

  // Binds local and remote addresses.
  // If you just want to send data, and don't care for local address,
  //  then use: local_addr=0.0.0.0:0
  // If you want to receive data then you need a valid local port at least.
  // Returns success.
  bool Open(const HostPort& local_addr);
  bool IsOpen() const;

  bool SetSendBufferSize(int size);
  bool SetRecvBufferSize(int size);

  // Send datagram to remote address.
  void SendDatagram(const io::MemoryStream& data, const net::HostPort& addr);
  void SendDatagram(const void* data, uint32 data_size, const net::HostPort& addr);

  // Pop a received datagram from the in_queue_. You have to delete it.
  // Returns NULL if there are no more datagrams.
  const Datagram* PopRecvDatagram();

  void FlushAndClose();
  void ForceClose();

  const HostPort local_addr() const { return local_addr_; }
  uint32 out_queue_size() const { return out_queue_.size(); }
  uint32 in_queue_size() const { return in_queue_.size(); }

  // returns a description of this connection, useful as log line header
  // Usage example:
  //  LOG_INFO << conn.PrefixInfo() << "foo";
  // yields a log line like:
  //  "UDP [0.0.0.0:5665 (fd: 7)] foo"
  string PrefixInfo() const;

 private:
  // enable non-blocking operations, and possibly other options..
  bool SetSocketOptions();
  // query fd and set local_addr_
  void LearnLocalAddress();
  // returns the errno associated with local fd_
  static int ExtractSocketErrno(int fd);
  int ExtractSocketErrno();

  void ClearInQueue();
  void ClearOutQueue();

  void InternalClose(int err, bool call_close_handler);

 public:
  ////////////////////////////////////////////////////////////////////////
  //
  // Methods similar to NetConnection
  //
  typedef ResultClosure<bool> ReadHandler;
  typedef ResultClosure<bool> WriteHandler;
  typedef Callback1<int> CloseHandler;
  void SetReadHandler(ReadHandler* read_handler, bool own) {
    DetachReadHandler();
    read_handler_ = read_handler;
    own_read_handler_ = own;
  }
  void DetachReadHandler() {
    if ( own_read_handler_ ) {
      delete read_handler_;
    }
    read_handler_ = NULL;
  }
  void SetWriteHandler(WriteHandler* write_handler, bool own) {
    DetachWriteHandler();
    write_handler_ = write_handler;
    own_write_handler_ = own;
  }
  void DetachWriteHandler() {
    if ( own_write_handler_ ) {
      delete write_handler_;
    }
    write_handler_ = NULL;
  }
  void SetCloseHandler(CloseHandler* close_handler, bool own) {
    DetachCloseHandler();
    close_handler_ = close_handler;
    own_close_handler_ = own;
  }
  void DetachCloseHandler() {
    if ( own_close_handler_ ) {
      delete close_handler_;
    }
    close_handler_ = NULL;
  }
  void DetachAllHandlers() {
    DetachReadHandler();
    DetachWriteHandler();
    DetachCloseHandler();
  }
 private:
  bool InvokeReadHandler() {
    CHECK_NOT_NULL(read_handler_) << "no read_handler found";
    return read_handler_->Run();
  }
  bool InvokeWriteHandler() {
    CHECK_NOT_NULL(write_handler_) << "no write_handler found";
    return write_handler_->Run();
  }
  void InvokeCloseHandler(int err) {
    if ( !close_handler_ ) {
      LOG_FATAL << this->PrefixInfo() << "No close_handler_ found";
      FlushAndClose();
      return;
    }
    close_handler_->Run(err);
  }

 public:
  void RequestReadEvents(bool enable) {
    selector_->EnableReadCallback(this, enable);
  }
  void RequestWriteEvents(bool enable) {
    selector_->EnableWriteCallback(this, enable);
  }
  int64 count_bytes_written() const { return count_bytes_written_; }
  int64 count_bytes_read() const { return count_bytes_read_; }
  int64 count_datagrams_written() const { return count_datagrams_sent_; }
  int64 count_datagrams_read() const { return count_datagrams_read_; }
  State state() const { return state_; }

 private:
  void set_state(State state) { state_ = state; }
  const char* StateName() const { return StateName(state()); }

  //////////////////////////////////////////////////////////////////////
  //
  // Selectable interface methods
  //
 private:
  virtual bool HandleReadEvent(const SelectorEventData& event);
  virtual bool HandleWriteEvent(const SelectorEventData& event);
  virtual bool HandleErrorEvent(const SelectorEventData& event);
  virtual void Close();
  virtual int GetFd() const { return fd_; }
  //////////////////////////////////////////////////////////////////////

 private:
  // callbacks into the application
  ReadHandler* read_handler_;
  WriteHandler* write_handler_;
  CloseHandler* close_handler_;

  bool own_read_handler_;
  bool own_write_handler_;
  bool own_close_handler_;

  // connection state
  State state_;

  // The fd of the socket
  int fd_;

  // the local ip/port of the fd_ socket
  HostPort local_addr_;

  // used to read data from network ( datagram by datagram )
  uint8* data_buffer_;
  uint32 data_buffer_size_;

  typedef list<Datagram*> DatagramQueue;
  // list of datagrams to send to the network
  DatagramQueue out_queue_;
  // list of datagrams to send to application
  DatagramQueue in_queue_;

  // Statistics
  int64 count_bytes_written_;
  int64 count_bytes_read_;
  int64 count_datagrams_sent_;
  int64 count_datagrams_read_;
};

} // namespace net

#endif // __NET_BASE_UDP_CONNECTION_H__
