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

#include <stdlib.h>

#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/base/scoped_ptr.h>

#include <whisperlib/net/address.h>
#include <whisperlib/net/selector.h>
#include <whisperlib/net/connection.h>

//////////////////////////////////////////////////////////////////////


DEFINE_bool(just_server,
            false,
            "True runs only the server.");
DEFINE_bool(just_client,
            false,
            "True runs only the clients.");

DEFINE_bool(ssl_enable, false,
            "If true we use SSL instead of pure TCP");
DEFINE_string(ssl_key, "",
              "SSL key file.");
DEFINE_string(ssl_certificate, "",
              "SSL certificate file.");

DEFINE_string(host,
              "127.0.0.1",
              "For client only: connect to this address");
DEFINE_int32(port,
             9989,
             "Our server uses this port");
DEFINE_int32(num_connections,
             20,
             "Number of connections for our test");

DEFINE_int32(min_num_pieces,
             10,
             "In our clients use at least these many pieces");

DEFINE_int32(max_num_pieces,
             20,
             "In our clients use at most these many pieces");

DEFINE_int32(min_piece_size,
             4000,
             "In our clients send pieces of at least this size");

DEFINE_int32(max_piece_size,
             8000,
             "In our clients send pieces of at most this size");

DEFINE_int32(time_between_connections_ms,
             1000,
             "Start a new connection after this time ... ");

//////////////////////////////////////////////////////////////////////

static int32 global_num_clients = 0;

#undef DCONNLOG
#define DCONNLOG DLOG_INFO << net_connection_->PrefixInfo()
#undef ICONNLOG
#define ICONNLOG LOG_INFO << net_connection_->PrefixInfo()
#undef ECONNLOG
#define ECONNLOG LOG_ERROR << net_connection_->PrefixInfo()

class EchoServerConnection {
  static const int32 kReadEvent  = 1;
  static const int32 kWriteEvent = 2;

  static const int64 kReadTimeout = 5000;  // milliseconds
  static const int64 kWriteTimeout = 5000;  // milliseconds

 public:
  explicit EchoServerConnection(net::Selector* selector,
                                net::NetConnection* connection)
      : selector_(selector),
        net_connection_(connection),
        expected_size_(-1),
        timeouter_(selector, NewPermanentCallback(
            this, &EchoServerConnection::TimeoutHandler)) {
    net_connection_->SetReadHandler(NewPermanentCallback(
        this, &EchoServerConnection::ConnectionReadHandler), true);
    net_connection_->SetWriteHandler(NewPermanentCallback(
        this, &EchoServerConnection::ConnectionWriteHandler), true);
    net_connection_->SetCloseHandler(NewPermanentCallback(
        this, &EchoServerConnection::ConnectionCloseHandler), true);
    timeouter_.SetTimeout(kReadEvent, kReadTimeout);
  }
  virtual ~EchoServerConnection() {
    delete net_connection_;
    net_connection_ = NULL;
  }

  void TimeoutHandler(int64 timeout_id) {
    ECONNLOG << "Timeout timeout_id: " << timeout_id;
    net_connection_->ForceClose();
  }

 protected:

  bool ConnectionReadHandler() {
    DCONNLOG << " SERVER - Handle Read";
    CHECK_EQ(net_connection_->state(), net::NetConnection::CONNECTED);
    // Normally we would not crash on this one,
    // but as we are the clients, we do
    CHECK_NE(expected_size_, 0);
    if ( expected_size_ == -1 ) {
      if ( net_connection_->inbuf()->Size() >= sizeof(expected_size_) ) {
        expected_size_ = io::NumStreamer::ReadInt32(net_connection_->inbuf(),
                                                    common::kByteOrder);
      } else {
        return true;  // wait for now..
      }
    }
    const int32 size = net_connection_->inbuf()->Size();
    if ( size ) {
      expected_size_ -= size;
      net_connection_->Write(net_connection_->inbuf());
      DCONNLOG << " server prepared to read: "
              << size << " expected left: " << expected_size_
              << " In buffer: " << net_connection_->outbuf()->Size()
              << " remains: " << net_connection_->inbuf()->Size();
      timeouter_.SetTimeout(kWriteEvent, kWriteTimeout);
    }
    if ( expected_size_ == 0 ) {
      net_connection_->RequestReadEvents(false);
      timeouter_.UnsetTimeout(kReadEvent);
    } else {
      timeouter_.SetTimeout(kReadEvent, kReadTimeout);
    }
    CHECK(expected_size_ >= 0) << " Size: " << expected_size_
                               << " here: " << size;  // same as above..
    return true;
  }
  bool ConnectionWriteHandler() {
    DCONNLOG << "SERVER - Handle Write: " << net_connection_->outbuf()->Size();
    if ( net_connection_->outbuf()->IsEmpty() ) {
      timeouter_.UnsetTimeout(kWriteEvent);
      if ( expected_size_ == 0 ) {
        ICONNLOG << "Server connection ending normally.";
        net_connection_->FlushAndClose();
        return true;
      }
    } else {
      timeouter_.SetTimeout(kWriteEvent, kWriteTimeout);
    }
    return true;
  }
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what) {
    LOG_WARNING << "ConnectionCloseHandler err=" << err
                << " what=" <<  net::NetConnection::CloseWhatName(what);
    if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
      net_connection_->FlushAndClose();
      return;
    }
    LOG_WARNING << "Auto deleting EchoServerConnection...";
    selector_->DeleteInSelectLoop(this);
  }
 private:
  net::Selector* selector_;
  net::NetConnection* net_connection_; // either server or client
  int32 expected_size_;
  net::Timeouter timeouter_;
};

class EchoServer {
public:
  EchoServer(net::Selector* selector,
             net::NetFactory* net_factory,
             net::PROTOCOL net_protocol)
    : selector_(selector),
      net_factory_(net_factory),
      net_acceptor_(net_factory->CreateAcceptor(net_protocol)) {
    net_acceptor_->SetFilterHandler(NewPermanentCallback(
        this, &EchoServer::AcceptorFilterHandler), true);
    net_acceptor_->SetAcceptHandler(NewPermanentCallback(
        this, &EchoServer::AcceptorAcceptHandler), true);
    selector_->RunInSelectLoop(NewCallback(this, &EchoServer::StartServer));
  }
  virtual ~EchoServer() {
    delete net_acceptor_;
    net_acceptor_ = NULL;
  }

  void StartServer() {
    net::HostPort local_addr("0.0.0.0", int16(FLAGS_port));
    bool success = net_acceptor_->Listen(local_addr);
    CHECK(success);
  }

  bool AcceptorFilterHandler(const net::HostPort & client_addr) {
    return true; // accept all
  }
  void AcceptorAcceptHandler(net::NetConnection * client_connection) {
    LOG_INFO << net_acceptor_->PrefixInfo() << "Client accepted from "
             << client_connection->remote_address();
    // auto-deletes on close
    new EchoServerConnection(selector_, client_connection);
  }
private:
  net::Selector* selector_;
  net::NetFactory* net_factory_;
  net::NetAcceptor* net_acceptor_;
};

//////////////////////////////////////////////////////////////////////

#undef DCONNLOG
#define DCONNLOG DLOG_INFO << net_connection_->PrefixInfo()
#undef ICONNLOG
#define ICONNLOG LOG_INFO << net_connection_->PrefixInfo()
#undef ECONNLOG
#define ECONNLOG LOG_ERROR << net_connection_->PrefixInfo()

class EchoClientConnection {
  static const int64 kConnectEvent = 1;
  static const int64 kReadEvent = 2;
  static const int64 kWriteEvent = 3;
  static const int64 kSendPiece = 4;

  static const int32 kConnectTimeout = 10000;
  static const int32 kReadTimeout = 8000;
  static const int32 kWriteTimeout = 8000;
 public:
  EchoClientConnection(net::Selector* selector,
                       net::NetFactory* net_factory,
                       net::PROTOCOL net_protocol,
                       int32 piece_size,
                       int32 num_pieces,
                       int64 time_between_pieces)
      : selector_(selector),
        net_factory_(net_factory),
        net_connection_(net_factory_->CreateConnection(net_protocol)),
        piece_size_(piece_size),
        num_pieces_(num_pieces),
        time_between_pieces_(time_between_pieces),
        pieces_sent_(0),
        has_timeout_(0),
        expected_data_(),
        timeouter_(selector, NewPermanentCallback(
            this, &EchoClientConnection::TimeoutHandler)) {
    net_connection_->SetReadHandler(NewPermanentCallback(
        this, &EchoClientConnection::ConnectionReadHandler), true);
    net_connection_->SetWriteHandler(NewPermanentCallback(
        this, &EchoClientConnection::ConnectionWriteHandler), true);
    net_connection_->SetConnectHandler(NewPermanentCallback(
        this, &EchoClientConnection::ConnectionConnectHandler), true);
    net_connection_->SetCloseHandler(NewPermanentCallback(
        this, &EchoClientConnection::ConnectionCloseHandler), true);
    selector_->RunInSelectLoop(
        NewCallback(this, &EchoClientConnection::StartClient,
                    net::HostPort(FLAGS_host.c_str(), FLAGS_port)));
  }
  virtual ~EchoClientConnection() {
    if ( time_between_pieces_ < 4500 && has_timeout_ ) {
      LOG_FATAL << "Timeout when not necessary: " << has_timeout_;
    }
    if ( !has_timeout_ && time_between_pieces_ < 4500 ) {
      CHECK(expected_data_.IsEmpty())
          << net_connection_->PrefixInfo()
          << " - Leftover size: " << expected_data_.Size()
          << " num_pieces: " << num_pieces_
          << " pieces_sent: " << pieces_sent_
          << " time_between_pieces: " << time_between_pieces_
          << " remote_address: " << net_connection_->remote_address()
          << " local_address: " << net_connection_->local_address();
    }
    delete net_connection_;
    net_connection_ = NULL;
    global_num_clients--;
    if ( global_num_clients == 0 ) {
      LOG_INFO << " Stopping the server in 2 secs !";
      selector_->RegisterAlarm(
          NewCallback(selector_, &net::Selector::MakeLoopExit),
          2000);
    }
    LOG_INFO << "EchoClientConnection deleted, "
             << global_num_clients << " still running";
  }
  void TimeoutHandler(int64 timeout_id) {
    if ( kSendPiece == timeout_id ) {
      SendPiece();
    } else {
      has_timeout_ = timeout_id;
      net_connection_->ForceClose();
    }
  }
 private:
  void ConnectionConnectHandler() {
    LOG_INFO << "ConnectionConnectHandler";
    timeouter_.UnsetTimeout(kConnectEvent);
    int32 timeout = random() % time_between_pieces_;
    timeouter_.SetTimeout(kSendPiece, timeout);
  }
  bool ConnectionReadHandler() {
    const char* buf;
    int32 size;
    int32 total_size = 0;
    while ( net_connection_->inbuf()->ReadNext(&buf, &size) ) {
      total_size += size;
      char* cmp_buf = new char[size];
      CHECK_GE(expected_data_.Size(), size);
      CHECK_EQ(expected_data_.Read(cmp_buf, size), size);
      for ( int32 i = 0; i < size; i++ ) {
        CHECK_EQ((int32)buf[i], (int32)cmp_buf[i]);
      }
      delete []cmp_buf;
    }
    DCONNLOG << "Read: " << total_size << " bytes, Still to come: "
             << expected_data_.Size() << " bytes.";
    if ( !expected_data_.IsEmpty() ) {
      // We want more - reenable stuff..
      timeouter_.SetTimeout(kReadEvent, kReadTimeout);
      //net_connection_->RequestReadEvents(true);
    } else {
      timeouter_.UnsetTimeout(kReadEvent);
      //net_connection_->RequestReadEvents(false);
      if ( pieces_sent_ == num_pieces_ ) {
        // Nothing more expected - close
        ICONNLOG << "CLIENT got all data - closing";
        return false;  // done -> closing..
      }
    }
    return true;
  }
  bool ConnectionWriteHandler() {
    if ( net_connection_->outbuf()->IsEmpty() ) {
      timeouter_.UnsetTimeout(kWriteEvent);
    } else {
      timeouter_.SetTimeout(kWriteEvent, kWriteTimeout);
    }
    return true;
  }
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what) {
    LOG_WARNING << "ConnectionCloseHandler err=" << err
                << " what=" <<  net::NetConnection::CloseWhatName(what);
    if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
      net_connection_->FlushAndClose();
      return;
    }
    LOG_WARNING << "Auto deleting EchoClientConnection..";
    selector_->DeleteInSelectLoop(this);
  }

 private:
  void SendPiece() {
    if ( pieces_sent_ == 0 ) {
      DCONNLOG << " Appending expected size: "
               << static_cast<int32>(piece_size_ * num_pieces_);
      CHECK_EQ(io::NumStreamer::WriteInt32(
                   net_connection_->outbuf(),
                   static_cast<int32>(piece_size_ * num_pieces_),
                   common::kByteOrder),
               sizeof(pieces_sent_));
      CHECK_EQ(net_connection_->outbuf()->Size(), sizeof(pieces_sent_));
    }
    char* buffer = new char[piece_size_];
    char* p = buffer;
    for ( int32 i = 0; i < piece_size_; i++ ) {
      *p++ = static_cast<char>(random() % 256);
    }
    io::MemoryStream s;
    s.AppendRaw(buffer, piece_size_);
    expected_data_.AppendStreamNonDestructive(&s);
    net_connection_->Write(&s);
    pieces_sent_++;
    DCONNLOG << "========> SEND PIECE IN CLIENT !!"
             << " piece_size_: " << piece_size_
             << " expecting: " << expected_data_.Size()
             << " Num left to send: " << num_pieces_ - pieces_sent_;
    // Set us for a new piece - if necessary
    if ( pieces_sent_ < num_pieces_ ) {
      timeouter_.SetTimeout(kSendPiece, random() % time_between_pieces_);
    }
    // Buffer should clean quite fast..
    timeouter_.SetTimeout(kWriteEvent, kWriteTimeout);
    // And we should get some data from the server back
    timeouter_.SetTimeout(kReadEvent, kReadTimeout);
  }
  void StartClient(net::HostPort remote_address) {
    if ( !net_connection_->Connect(remote_address) ) {
      return;  // Nothing to do ..
    }
    timeouter_.SetTimeout(kConnectEvent, kConnectTimeout);
  }
 private:
  net::Selector* selector_;
  net::NetFactory* net_factory_;
  net::NetConnection* net_connection_;
  const int32 piece_size_;
  const int32 num_pieces_;
  const int64 time_between_pieces_;
  int32 pieces_sent_;
  int has_timeout_;
  io::MemoryStream expected_data_;
  net::Timeouter timeouter_;
};

void StartEchoConnection(int32 num_to_start,
                         net::Selector* selector,
                         net::NetFactory* net_factory,
                         net::PROTOCOL net_protocol) {
  // TODO(cosmin): restore original code
  const bool should_timeout = false;//random() % 10 > 8;
  int64 time_between_pieces = (should_timeout
                               ? 5000 + (random() % 5000)
                               : 100 + (random() % 2000));
  const int32 piece_size = (FLAGS_min_piece_size +
                            random() % (FLAGS_max_piece_size -
                                        FLAGS_min_piece_size));
  const int32 num_pieces = (FLAGS_min_num_pieces +
                            random() % (FLAGS_max_num_pieces -
                                        FLAGS_min_num_pieces));
  LOG_INFO << "==========> Starting a new echo client: "
           << time_between_pieces
           << " - " << piece_size
           << " - " << num_pieces;

  new EchoClientConnection(selector, net_factory, net_protocol,
                           piece_size, num_pieces, time_between_pieces);
  global_num_clients++;
  num_to_start--;
  if ( num_to_start > 0 ) {
    selector->RegisterAlarm(NewCallback(&StartEchoConnection,
                                        num_to_start, selector,
                                        net_factory, net_protocol),
                            FLAGS_time_between_connections_ms);
  }
}

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  net::Selector selector;

  net::NetFactory net_factory(&selector);
  net::PROTOCOL net_protocol = net::PROTOCOL_TCP;

#if defined(HAVE_OPENSSL_SSL_H) && defined(USE_OPENSSL)
  SSL_CTX* ssl_context = NULL;
  if ( FLAGS_ssl_enable ) {
    LOG_WARNING << "Running the SSL test...";
    ssl_context = net::SslConnection::SslCreateContext(
        FLAGS_ssl_certificate, FLAGS_ssl_key);
    if ( ssl_context == NULL ) {
      LOG_FATAL << "Failed to create SSL context. Using"
                   " certificate file: [" << FLAGS_ssl_certificate << "]"
                   ", key file: [" << FLAGS_ssl_key << "]";
    }
    net::SslConnectionParams ssl_connection_params;
    ssl_connection_params.ssl_context_ = ssl_context;
    net_factory.SetSslParams(net::SslAcceptorParams(ssl_connection_params),
                             ssl_connection_params);
    net_protocol = net::PROTOCOL_SSL;
  } else {
#endif
    LOG_WARNING << "Running the TCP test...";
    net_protocol = net::PROTOCOL_TCP;
#if defined(HAVE_OPENSSL_SSL_H) && defined(USE_OPENSSL)
  }
#endif
  if ( !FLAGS_just_client ) {
    new EchoServer(&selector, &net_factory, net_protocol); // auto deletes
  }
  if ( !FLAGS_just_server ) {
    selector.RunInSelectLoop(
        NewCallback(&StartEchoConnection,
                    FLAGS_num_connections,
                    &selector,
                    &net_factory,
                    net_protocol));
  }

  selector.Loop();
#if defined(HAVE_OPENSSL_SSL_H) && defined(USE_OPENSSL)
  net::SslConnection::SslDeleteContext(ssl_context);
  ssl_context = NULL;
#endif
  LOG_INFO << "PASS";
  common::Exit(0);
}
