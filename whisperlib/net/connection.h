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
// Authors: Cosmin Tudorache & Catalin Popescu
//

#ifndef __NET_BASE_CONNECTION_H__
#define __NET_BASE_CONNECTION_H__

#include <string>
#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"

#include "whisperlib/io/buffer/memory_stream.h"

#include "whisperlib/net/address.h"
#include "whisperlib/net/selectable.h"
#include "whisperlib/net/timeouter.h"
#include "whisperlib/net/dns_resolver.h"


// TcpConnection
// -------------
// Encapsulates a socket file descriptor of the underneath connection.
// This class provides the basic mechanisms for handling events received
// from the selector. It implements reading and writing methods for
// reading/writing from/to the fd. It also uses read/write buffers ,
// so you don't have to re-buffer.
// To use this class:
// - obtain an instance, either by direct constructor, or by factory
// - set appropriate handlers (connect, read, write, close)
// - call Connect to connect to open connection to desired address.
// You will be notified on "connect" handler when connection is
// fully established, or connect failed.
// In fully connected mode, you will be notified on "read" handler when new data
// arrives (read it from inbuf() stream), and on "close" handler when
// the connection got closed (either because you or your party closed it).
// NOTE possible pitfalls:
//  -- all handlers should return as fast as possible, selector's asynchronous
//     execution efficiency depends on it.
//  -- to implement timeouts, use net::Timeouter class w/ the selector
//  -- to implement flow-control you should check the size of inbuf(),
//     and/or outbuf() and enable/disable reading or writing as required
//     for your protocol.

// TcpAcceptor
// -----------
// Encapsulates a listening socket file descriptor.
// To use this class:
// - obtain an instance, either by direct constructor, or by factory
// - set appropriate handlers (accept, filter)
// - call Listen to open local address in listen mode
// You will be notified on "filter" handler when someone wants to connect
// and on "accept" handler when someone connected.
namespace whisper {
namespace net {

////////////////////////////////////////////////////////////////////////////

struct NetConnectionParams {
  NetConnectionParams(int block_size = io::DataBlock::kDefaultBufferSize)
    : block_size_(block_size) {
  }
  int block_size_;
};

struct NetAcceptorParams {
  NetAcceptorParams()
      : next_client_thread_(0),
        client_threads_(NULL) {
  }
  NetAcceptorParams(const NetAcceptorParams& params)
      : next_client_thread_(0),
        client_threads_(params.client_threads_) {
  }
  void set_client_threads(const vector<SelectorThread*>* client_threads) {
    CHECK(client_threads_ == NULL);
    client_threads_ = client_threads;
  }
  Selector* GetNextSelector() {
    if ( client_threads_ == NULL ) {
      return NULL;
    }
    if ( next_client_thread_ >= client_threads_->size() ) {
      next_client_thread_ = 0;
    }
    Selector* const chosen =
        (*client_threads_)[next_client_thread_]->mutable_selector();
    next_client_thread_++;
    return chosen;
  }
 private:
  int next_client_thread_;
  const vector<SelectorThread*>* client_threads_;
};

////////////////////////////////////////////////////////////////////////////

class Selector;
class NetConnection;

class NetAcceptor {
 public:
  enum State {
    DISCONNECTED,
    LISTENING,
  };
  static const char* StateName(State s) {
    switch (s) {
      CONSIDER(DISCONNECTED);
      CONSIDER(LISTENING);
      default: return "UNKNOWN";
    }
  }

 public:
  explicit NetAcceptor(const NetAcceptorParams& /*params = NetAcceptorParams()*/)
    : state_(DISCONNECTED),
      local_address_(),
      last_error_code_(0),
      filter_handler_(NULL),
      own_filter_handler_(false),
      accept_handler_(NULL),
      own_accept_handler_(false) {
  }
  virtual ~NetAcceptor() {
    // The SUPER class should call Close() in destructor;
    // we cannot call it, because Close is virtual.
    CHECK_EQ(state(), DISCONNECTED);
    DetachAllHandlers();
  }

  // opens acceptor in listen mode
  virtual bool Listen(const whisper::net::HostPort& local_addr) = 0;

  // closes acceptor
  virtual void Close() = 0;

  // returns a description of this acceptor, useful as log line header
  // Usage example:
  //  LOG_INFO << acceptor.PrefixInfo() << "foo";
  // yields a log line like:
  //  "LISTENING : [0.0.0.0:5665 (fd: 7)] foo"
  virtual string PrefixInfo() const = 0;

  const HostPort& local_address() const {
    return local_address_;
  }
  State state() const {
    return state_;
  }
  const char* StateName() const {
    return StateName(state_);
  }

  int last_error_code() const {
    return last_error_code_;
  }

 protected:
  void set_state(State state) {
    state_ = state;
  }
  void set_local_address(const whisper::net::HostPort& local_address) {
    local_address_ = local_address;
  }
  void set_last_error_code(int last_error_code) {
    last_error_code_ = last_error_code;
  }

 public:
  typedef ResultCallback1<bool, const whisper::net::HostPort&> FilterHandler;
  typedef Callback1<NetConnection*> AcceptHandler;

  void SetFilterHandler(FilterHandler* filter_handler, bool own);
  void DetachFilterHandler();
  void SetAcceptHandler(AcceptHandler* accept_handler, bool own);
  void DetachAcceptHandler();
  void DetachAllHandlers();

 protected:
  bool InvokeFilterHandler(const whisper::net::HostPort& incoming_peer_address);
  void InvokeAcceptHandler(NetConnection* new_connection);

 private:
  // The state we are in
  State state_;

  // socket local address
  HostPort local_address_;

  // the code of the last error. Just for log & debug.
  int last_error_code_;

  // We call this handler to notify the application that a new client wants
  //  to connect.
  // If the application returns true -> we proceed with connection setup
  // else -> we reject this guy.
  FilterHandler* filter_handler_;
  bool own_filter_handler_;
  // We call this handler to deliver a fully connected client to application.
  AcceptHandler* accept_handler_;
  bool own_accept_handler_;
};

class NetConnection {
 public:
  enum State {
    DISCONNECTED,
    RESOLVING,
    CONNECTING,
    CONNECTED,
    FLUSHING,
  };
  static const char* StateName(State s) {
    switch (s) {
      CONSIDER(DISCONNECTED);
      CONSIDER(RESOLVING);
      CONSIDER(CONNECTING);
      CONSIDER(CONNECTED);
      CONSIDER(FLUSHING);
    }
    return "UNKNOWN";
  }

  enum CloseWhat {
    CLOSE_READ,        // close read half of the connection
    CLOSE_WRITE,       // close write half of the connection
    CLOSE_READ_WRITE,  // close both read & write
  };
  static const char* CloseWhatName(CloseWhat what) {
    switch ( what ) {
      CONSIDER(CLOSE_READ);
      CONSIDER(CLOSE_WRITE);
      CONSIDER(CLOSE_READ_WRITE);
    }
    return "UNKNOWN";
  }

 public:
  NetConnection(Selector* net_selector,
                const NetConnectionParams& params)
      : net_selector_(net_selector),
        state_(DISCONNECTED),
        last_error_code_(0),
        connect_handler_(NULL),
        read_handler_(NULL),
        write_handler_(NULL),
        close_handler_(NULL),
        own_connect_handler_(false),
        own_read_handler_(false),
        own_write_handler_(false),
        own_close_handler_(false),
        count_bytes_written_(0),
        count_bytes_read_(0),
        inbuf_(params.block_size_),
        outbuf_(params.block_size_) {
  }
  virtual ~NetConnection() {
    DetachAllHandlers();
  }

  // connect to remote address.
  // returns: false -> connect failed.
  //          true -> connect pending.
  //                  Later on: if the connect procedure succeeds,
  //                            the ConnectHandler will be called.
  //                            If it fails, the CloseHandler will be called.
  virtual bool Connect(const whisper::net::HostPort& addr) = 0;
  virtual void FlushAndClose() = 0;
  virtual void ForceClose() = 0;
  // tune connection
  virtual bool SetSendBufferSize(int size) = 0;
  virtual bool SetRecvBufferSize(int size) = 0;
  // These functions registers/unregisters the connection for read/write
  // events with the selector.
  virtual void RequestReadEvents(bool enable) = 0;
  virtual void RequestWriteEvents(bool enable) = 0;
  // Return local/remote connection address.
  virtual const HostPort& local_address() const = 0;
  virtual const HostPort& remote_address() const = 0;
  // returns a description of this acceptor, useful as log line header
  // Usage example:
  //  LOG_INFO << connection.PrefixInfo() << "foo";
  // yields a log line like:
  //  "CONNECTED : [12.34.56.78:5665 -> 87.65.43.21:6556 (fd: 7)] foo"
  virtual string PrefixInfo() const = 0;


  typedef Closure ConnectHandler;
  typedef ResultClosure<bool> ReadHandler;
  typedef ResultClosure<bool> WriteHandler;
  typedef Callback2<int, CloseWhat> CloseHandler;

  // Means of communication with the outside world.
  // Applications should set this handlers to receive connection notifications.
  // own: true = the handler is automatically deleted.
  //      false = the caller still owns the handler.
  void SetConnectHandler(ConnectHandler* connect_handler, bool own);
  void DetachConnectHandler();
  void SetReadHandler(ReadHandler* read_handler, bool own);
  void DetachReadHandler();
  void SetWriteHandler(WriteHandler* write_handler, bool own);
  void DetachWriteHandler();
  void SetCloseHandler(CloseHandler* close_handler, bool own);
  void DetachCloseHandler();
  void DetachAllHandlers();

 protected:
  void InvokeConnectHandler();
  bool InvokeReadHandler();
  bool InvokeWriteHandler();
  void InvokeCloseHandler(int err, CloseWhat what);

 public:
  Selector* net_selector() const {
    return net_selector_;
  }
  State state() const {
    return state_;
  }
  const char* StateName() const {
    return StateName(state_);
  }

  // Write basics: copies data to oubuf & enables write requests.
  void Write(const void* buf, int32 size) {
    outbuf()->Write(buf, size);
    RequestWriteEvents(true);
  }
  void Write(io::MemoryStream* ms) {
    outbuf()->AppendStream(ms);
    RequestWriteEvents(true);
  }
  void Write(const string& str) {
    Write(str.c_str(), str.size());
  }
  void Write(const char * sz_str) {
    Write(sz_str, ::strlen(sz_str));
  }

  int64 count_bytes_written() const { return count_bytes_written_; }
  int64 count_bytes_read() const { return count_bytes_read_; }

  io::MemoryStream* inbuf() { return &inbuf_; }
  io::MemoryStream* outbuf() { return &outbuf_; }

  int last_error_code() const { return last_error_code_; }

 protected:
  void set_net_selector(Selector* net_selector) {
    CHECK(net_selector_ == NULL);
    net_selector_ = net_selector;
  }
  void set_state(State state) {
    state_ = state;
  }
  void set_last_error_code(int err) {
    last_error_code_ = err;
  }
  void inc_bytes_written(int64 value) {
    count_bytes_written_ += value;
  }
  void inc_bytes_read(int64 value) {
    count_bytes_read_ += value;
  }

 private:
  Selector* net_selector_;
  // connection state
  State state_;

  int last_error_code_;

  ConnectHandler* connect_handler_;
  ReadHandler* read_handler_;
  WriteHandler* write_handler_;
  CloseHandler* close_handler_;

  bool own_connect_handler_;
  bool own_read_handler_;
  bool own_write_handler_;
  bool own_close_handler_;

  // Statistics
  int64 count_bytes_written_;
  int64 count_bytes_read_;

  // Buffered connection
  io::MemoryStream inbuf_;
  io::MemoryStream outbuf_;
};

/////////////////////////////////////////////////////////////////////////////

struct TcpConnectionParams : public NetConnectionParams {
  TcpConnectionParams(int block_size = io::DataBlock::kDefaultBufferSize,
                      int send_buffer_size = -1,
                      int recv_buffer_size = -1,
                      int64 shutdown_linger_timeout_ms = 5000,
                      int read_limit = -1,
                      int write_limit = -1)
    : NetConnectionParams(block_size),
      send_buffer_size_(send_buffer_size),
      recv_buffer_size_(recv_buffer_size),
      shutdown_linger_timeout_ms_(shutdown_linger_timeout_ms),
      read_limit_(read_limit),
      write_limit_(write_limit) {
  }
  int send_buffer_size_; // if -1 the system default is used
  int recv_buffer_size_; // if -1 the system default is used
  int64 shutdown_linger_timeout_ms_;

  // Buffered read operations are limited to this size
  int read_limit_;
  // Buffered write operations are limited to this size
  int write_limit_;
};

struct TcpAcceptorParams : public NetAcceptorParams {
  TcpAcceptorParams(
      // insert NetAcceptorParams here, with default values
      const TcpConnectionParams& tcp_connection_params = TcpConnectionParams(),
      int backlog = 100)
        : NetAcceptorParams(),
          tcp_connection_params_(tcp_connection_params),
          backlog_(backlog) {
  }
  // parameter for spawned connections
  TcpConnectionParams tcp_connection_params_;
  int backlog_;

};

/////////////////////////////////////////////////////////////////////////////

class TcpConnection;

class TcpAcceptor : public NetAcceptor, private Selectable {
 public:
  TcpAcceptor(Selector* selector,
              const TcpAcceptorParams& tcp_params = TcpAcceptorParams());
  virtual ~TcpAcceptor();

  //////////////////////////////////////////////////////////////////////
  //
  // NetAcceptor methods
  //
 public:
  virtual bool Listen(const HostPort& local_addr);
  virtual void Close(); // for both NetAcceptor and Selectable interfaces
  virtual string PrefixInfo() const;

  //////////////////////////////////////////////////////////////////////
  //
  // Overridden selectable interface
  //
 private:
  virtual bool HandleReadEvent(const SelectorEventData& event);
  virtual bool HandleWriteEvent(const SelectorEventData& event);
  virtual bool HandleErrorEvent(const SelectorEventData& event);
  virtual int GetFd() const { return fd_; }
  //virtual void Close(); // already defined above
  //////////////////////////////////////////////////////////////////////

 private:
  // Initializes a new connection in the provided selector
  void InitializeAcceptedConnection(Selector* selector, int client_fd);

  // read local_address from socket
  void InitializeLocalAddress();

  // Sets normal socket options for our purposes:
  //  non-blocking, fast bind reusing
  bool SetSocketOptions();

  // returns the errno associated with local fd_
  int ExtractSocketErrno();

  // close internal socket
  void InternalClose(int err);

 private:
  TcpAcceptorParams tcp_params_;

  // The fd of the socket
  int fd_;
};

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

class TcpConnection : public NetConnection, private Selectable {
 private:
  static const int64 kShutdownTimeoutId = -100;

 public:
  TcpConnection(Selector* selector,
                const TcpConnectionParams& tcp_params = TcpConnectionParams());
  virtual ~TcpConnection();

  void Close(CloseWhat what);

 private:
  // Use an already connected fd. Usually obtained by an acceptor.
  // The local and peer addresses are obtained from fd.
  bool Wrap(int fd);
  friend class TcpAcceptor;

  //////////////////////////////////////////////////////////////////////
  //
  // NetConnection interface methods
  //
 public:
  virtual bool Connect(const HostPort& remote_addr);
  virtual void FlushAndClose();
  virtual void ForceClose();
  virtual bool SetSendBufferSize(int size);
  virtual bool SetRecvBufferSize(int size);
  virtual void RequestReadEvents(bool enable);
  virtual void RequestWriteEvents(bool enable);
  virtual const HostPort& local_address() const;
  virtual const HostPort& remote_address() const;
  virtual string PrefixInfo() const;
  //////////////////////////////////////////////////////////////////////

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
  bool read_closed() const { return read_closed_; }
  bool write_closed() const { return write_closed_; }
  void set_read_closed(bool read_closed) { read_closed_ = read_closed; }
  void set_write_closed(bool write_closed) { write_closed_ = write_closed; }

  // sets socket options for our purposes
  //  - non-blocking
  //  - disable Nagel algorithm
  //  - apply tcp parameters
  bool SetSocketOptions();

  // returns the errno associated with the given socket
  static int ExtractSocketErrno(int fd);
  int ExtractSocketErrno() {
    return ExtractSocketErrno(fd_);
  }

  // This function reads the local address from the socket and sets
  // it into local_address_
  void InitializeLocalAddress();
  // This function reads the remote address from the socket and sets
  // it into remote_address_
  void InitializeRemoteAddress();

  // because NetConnection::InvokeConnectHandler is protected we wrap it here.
  void InvokeConnectHandler() {
    NetConnection::InvokeConnectHandler();
  }
  // because NetConnection::InvokeCloseHandler is protected we wrap it here.
  void InvokeCloseHandler(int err, CloseWhat what) {
    if ( what == CLOSE_READ || what == CLOSE_READ_WRITE ) {
      CHECK(read_closed()) << " what=" << what;
    }
    if ( what == CLOSE_WRITE || what == CLOSE_READ_WRITE ) {
      CHECK(write_closed()) << " what=" << what;
    }
    NetConnection::InvokeCloseHandler(err, what);
  }

  // immediate close fd
  void InternalClose(int err, bool call_close_handler);

  // The timeouter handler
  void HandleTimeoutEvent(int64 timeout_id);

  // The DNS handler
  void HandleDnsResult(scoped_ref<DnsHostInfo> info);

 private:
  // parameters for this connection
  TcpConnectionParams tcp_params_;

  // The fd of the socket
  int fd_;

  whisper::net::HostPort local_address_;
  whisper::net::HostPort remote_address_;

  // true = the write half of the connection is closed; no more transmissions.
  bool write_closed_;
  // true = the read half of the connection is closed; no more receptions.
  bool read_closed_;

  Timeouter timeouter_;

  // timestamp of the last read/write event
  int64 last_read_ts_;
  int64 last_write_ts_;

  // permanent callback to HandleDnsResult
  DnsResultHandler* handle_dns_result_;
};
}  // namespace net
}  // namespace whisper

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// SSL connection
//
////////////////////////////////////////////////////////////////////////

#if defined(USE_OPENSSL)
// apt-get install libssl-dev
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif // USE_OPENSSL

namespace whisper {
namespace net {

#if defined(USE_OPENSSL)

struct SslConnectionParams : public NetConnectionParams {
  SslConnectionParams(
      SSL_CTX* ssl_context = NULL,
      int block_size = io::DataBlock::kDefaultBufferSize,
      int read_limit = -1,
      int write_limit = -1)
        : NetConnectionParams(block_size),
          ssl_context_(ssl_context),
          read_limit_(read_limit),
          write_limit_(write_limit) {
  }
  SSL_CTX* ssl_context_;
  // Buffered read operations are limited to this size
  int read_limit_;
  // Buffered write operations are limited to this size
  int write_limit_;
};

struct SslAcceptorParams : public NetAcceptorParams {
  SslAcceptorParams(
      // insert NetAcceptorParams params here
      const SslConnectionParams& ssl_connection_params = SslConnectionParams()
      // insert specific SslAcceptorParams params here
      )
        : ssl_connection_params_(ssl_connection_params) {
  }
  // parameter for spawned connections
  SslConnectionParams ssl_connection_params_;
};

/////////////////////////////////////////////////////////////////////////

class SslConnection;

class SslAcceptor : public NetAcceptor {
 public:
  SslAcceptor(Selector* selector,
              const SslAcceptorParams& ssl_params = SslAcceptorParams());
  virtual ~SslAcceptor();

  bool TcpAcceptorFilterHandler(const whisper::net::HostPort& peer_addr);
  void TcpAcceptorAcceptHandler(NetConnection* tcp_connection);

  void SslConnectionConnectHandler(SslConnection* ssl_connection);
  void SslConnectionCloseHandler(SslConnection* ssl_connection,
      int err, NetConnection::CloseWhat what);

  //////////////////////////////////////////////////////////////////////
  //
  // NetAcceptor interface methods
  //
  virtual bool Listen(const whisper::net::HostPort& local_addr);
  virtual void Close();
  virtual string PrefixInfo() const;

 private:
  // Initialize SSL members
  bool SslInitialize();
  // Cleanup SSL members
  void SslClear();

  whisper::net::Selector* selector() { return selector_; }
  // Initializes a new connection in the provided selector
  // void InitializeAcceptedConnection(Selector* selector, int client_fd);
 private:
  whisper::net::Selector* selector_;
  // parameters for this acceptor
  const SslAcceptorParams params_;

  // the TCP acceptor
  TcpAcceptor tcp_acceptor_;
};

class SslConnection : public NetConnection {
 public:
  SslConnection(Selector* selector,
                const SslConnectionParams& ssl_params = SslConnectionParams());
  virtual ~SslConnection();

 private:
  // Use an already established tcp connection. Usually obtained by an acceptor.
  // We take ownership of tcp_connection.
  void Wrap(TcpConnection* tcp_connection);
  friend class SslAcceptor;

  //////////////////////////////////////////////////////////////////////
  //
  // NetConnection interface methods
  //
 public:
  virtual bool Connect(const HostPort& remote_addr);
  virtual void FlushAndClose();
  virtual void ForceClose();
  virtual bool SetSendBufferSize(int size);
  virtual bool SetRecvBufferSize(int size);
  virtual void RequestReadEvents(bool enable);
  virtual void RequestWriteEvents(bool enable);
  virtual const HostPort& local_address() const;
  virtual const HostPort& remote_address() const;
  virtual string PrefixInfo() const;
  //////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////
  //
  // Selectable interface methods
  //
 private:
  virtual bool HandleReadEvent(const SelectorEventData& event);
  virtual bool HandleWriteEvent(const SelectorEventData& event);
  virtual bool HandleErrorEvent(const SelectorEventData& event);
  virtual void Close();
  virtual int GetFd() const;
  //////////////////////////////////////////////////////////////////////

 private:
  // Working with the underlying tcp_connection
  void TcpConnectionConnectHandler();
  bool TcpConnectionReadHandler();
  bool TcpConnectionWriteHandler();
  void TcpConnectionCloseHandler(int err, CloseWhat what);

  // The timeouter handler
  void HandleTimeoutEvent(int64 timeout_id);

  // because NetConnection::InvokeConnectHandler is protected we wrap it here.
  void InvokeConnectHandler() {
    NetConnection::InvokeConnectHandler();
  }

 public:
  static const char* SslErrorName(int err);
  static const char* SslWantName(int want);
  // Returns a description of the last SSL error. This function pops error
  // values from SSL stack, so don't call it twice.
  static std::string SslLastError();

  // Globally initialize SSL library.
  static const void SslLibraryInit();

  // Returns a new X509 structure, or NULL on failure.
  // You have to call X509_free(..) on the valid result.
  static X509* SslLoadCertificateFile(const string& filename);
  // Returns a new EVP_PKEY structure, or NULL on failure.
  // You have to call EVP_PKEY_free(..) on the valid result.
  static EVP_PKEY* SslLoadPrivateKeyFile(const string& filename);

  // Clone X509. Never fail. Use X509_free(..).
  static X509* SslDuplicateX509(const X509& src);
  // Clone EVP_PKEY. Never fail. Use EVP_PKEY_free(..).
  static EVP_PKEY* SslDuplicateEVP_PKEY(const EVP_PKEY& src);

  // returns a description of all data buffered in "bio"
  static string SslPrintableBio(BIO* bio);

  // Returns a new SSL_CTX structure, or NULL on failure.
  // It also loads SSL certificate and key if non-empty strings.
  // You have to call SslDeleteContext(..) on the valid result.
  static SSL_CTX* SslCreateContext(const string& certificate_filename = "",
                                   const string& key_filename = "");
  // Free SSL context. This is the reverse of SslCreateContext(..).
  static void SslDeleteContext(SSL_CTX* ssl_ctx);

  // Used from the depth of the ssl verification callback to set the
  // verification status failed
  void SslSetVerificationFailed() { verification_failed_ = true; }

  static int SslVerificationIndex() { return verification_index_; }

 private:
  bool SslInitialize(bool is_server);
  void SslClear();

  // do SSL handshake
  void SslHandshake();
  // do SSL shutdown
  void SslShutdown();

 private:
  whisper::net::Selector* selector_;
  // parameters for this connection
  const SslConnectionParams ssl_params_;

  // The TCP connection
  TcpConnection* tcp_connection_;

  static const uint32 kSslBufferSize = 10000; //The maximum size of BIO buffers
  bool is_server_side_; // true = this is a server side connection
                        // false = this is a client side connection

  // The OpenSSL structures
  SSL_CTX* p_ctx_;    // also contains the certificate and key
  BIO* p_bio_read_;   // Network --> SSL , use BIO_write(p_bio_read_, ...)
  BIO* p_bio_write_;  // Network <-- SSL , use BIO_read(p_bio_write_, ...)
  SSL* p_ssl_;

  bool handshake_finished_;

  // helpers for sequencing reads/writes of complete SSL packets.
  bool read_blocked_;
  bool read_blocked_on_write_;
  bool write_blocked_on_read_;

  // for debug, count output/input bytes
  uint64 ssl_out_count_;
  uint64 ssl_in_count_;

  Timeouter timeouter_;

  // If verification failed:
  bool verification_failed_;
  // Ssl index registerd for setting custom data to SSL object.
  static int verification_index_;
  synch::Mutex verification_mutex_;
};

#endif // USE_OPENSSL

enum PROTOCOL {
  PROTOCOL_TCP,
#if defined(USE_OPENSSL)
  PROTOCOL_SSL,
#else
  PROTOCOL_SSL = PROTOCOL_TCP,
#endif
};
class NetFactory {
public:
  NetFactory(net::Selector* selector)
    : selector_(selector),
      tcp_acceptor_params_(),
      tcp_connection_params_()
#if defined(USE_OPENSSL)
    , ssl_acceptor_params_(),
      ssl_connection_params_()
#endif
        {
  }
  ~NetFactory() {
  }
  void SetTcpParams(const TcpAcceptorParams&
                      tcp_acceptor_params = TcpAcceptorParams(),
                    const TcpConnectionParams&
                      tcp_connection_params = TcpConnectionParams()) {
    tcp_acceptor_params_ = tcp_acceptor_params;
    tcp_connection_params_ = tcp_connection_params;
  }
  void SetTcpConnectionParams(const TcpConnectionParams& params) {
    tcp_connection_params_ = params;
  }

#if defined(USE_OPENSSL)
  void SetSslParams(const SslAcceptorParams&
                      ssl_acceptor_params = SslAcceptorParams(),
                    const SslConnectionParams&
                      ssl_connection_params = SslConnectionParams()) {
    ssl_acceptor_params_ = ssl_acceptor_params;
    ssl_connection_params_ = ssl_connection_params;
  }
  void SetSslConnectionParams(const SslConnectionParams& params) {
    ssl_connection_params_ = params;
  }
#endif
  NetConnection* CreateConnection(PROTOCOL protocol) const {
    switch(protocol) {
      case PROTOCOL_TCP:
        return new TcpConnection(selector_, tcp_connection_params_);
#if defined(USE_OPENSSL)
      case PROTOCOL_SSL:
        return new SslConnection(selector_, ssl_connection_params_);
#endif
      default:
        LOG_FATAL << "Illegal protocol: " << protocol;
        return NULL;
    }
  }
  NetAcceptor* CreateAcceptor(PROTOCOL protocol) const {
    switch(protocol) {
      case PROTOCOL_TCP:
        return new TcpAcceptor(selector_, tcp_acceptor_params_);
#if defined(USE_OPENSSL)
      case PROTOCOL_SSL:
        return new SslAcceptor(selector_, ssl_acceptor_params_);
#endif
      default:
        LOG_FATAL << "Illegal protocol: " << protocol;
        return NULL;
    }
  }

  whisper::net::Selector* selector() const {
    return selector_;
  }

  const TcpAcceptorParams& tcp_acceptor_params() const {
    return tcp_acceptor_params_;
  }
  const TcpConnectionParams& tcp_connection_params() const {
    return tcp_connection_params_;
  }

#if defined(USE_OPENSSL)
  const SslAcceptorParams& ssl_acceptor_params() const {
    return ssl_acceptor_params_;
  }
  const SslConnectionParams& ssl_connection_params() const {
    return ssl_connection_params_;
  }
#endif

private:
  whisper::net::Selector* const selector_;

  TcpAcceptorParams tcp_acceptor_params_;
  TcpConnectionParams tcp_connection_params_;

#if defined(USE_OPENSSL)
  SslAcceptorParams ssl_acceptor_params_;
  SslConnectionParams ssl_connection_params_;
#endif
};
}  // namespace net
}  // namespace whisper

#endif  // __NET_BASE_CONNECTION_H__
