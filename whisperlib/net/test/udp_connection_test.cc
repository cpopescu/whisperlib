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
// Author: Cosmin Tudorache

#include <stdlib.h>

#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/system.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/base/scoped_ptr.h"

#include "whisperlib/net/timeouter.h"
#include "whisperlib/net/address.h"
#include "whisperlib/net/selector.h"
#include "whisperlib/net/udp_connection.h"

using namespace std;

//////////////////////////////////////////////////////////////////////


DEFINE_bool(just_server,
            false,
            "True runs only the server.");
DEFINE_bool(just_client,
            false,
            "True runs only the client.");

DEFINE_int32(server_port,
             9254,
             "The UDP port where the server listens.");

DEFINE_int32(num_clients,
             1,
             "How many clients to run.");
DEFINE_int32(num_packets_per_client,
             3,
             "The number of packets a client should send.");

#define LOG_TEST  LOG_INFO << ClassName() << "("     \
  << (this->udp_connection_ == NULL ? 0 : \
      this->udp_connection_->local_addr().port()) << "): "

uint32 g_num_clients;

uint32 RandomInt(uint32 min, uint32 max) {
  return min + static_cast<uint32>((max - min) * (1.0 * ::rand() / RAND_MAX));
}
uint8* RandomData(uint32 min_size, uint32 max_size, uint32* out_size) {
  uint32 size = RandomInt(min_size, max_size);
  if ( size == 0 ) {
    *out_size = 0;
    return NULL;
  }
  uint8* data = new uint8[size];
  CHECK(data) << "Out of memory";
  for ( uint32 i = 0; i < size; i++ ) {
    data[i] = ::rand() % 255;
  }
  *out_size = size;
  return data;
}
bool operator==(const whisper::io::MemoryStream& ca, const whisper::io::MemoryStream& cb) {
  return ca.Equals(cb);
}
bool operator!=(const whisper::io::MemoryStream& a, const whisper::io::MemoryStream& b) {
  return !(a==b);
}

// This UDP server listens on FLAGS_server_port and replies to all UDP packets
// with the same packet. Receives UDP packets from any address.
class EchoServer {
public:
  static const char* ClassName() { return "SERVER"; };
public:
  EchoServer(whisper::net::Selector* selector)
    : selector_(selector),
      udp_connection_(NULL) {
    LOG_TEST << "++EchoServer";
    selector_->RunInSelectLoop(whisper::NewCallback(this, &EchoServer::Start));
  }
  virtual ~EchoServer() {
    LOG_TEST << "--EchoServer";
    delete udp_connection_;
    udp_connection_ = NULL;
  }

  void Start() {
    CHECK_NULL(udp_connection_);
    udp_connection_ = new whisper::net::UdpConnection(selector_);
    udp_connection_->SetReadHandler(whisper::NewPermanentCallback(this,
        &EchoServer::ConnectionReadHandler), true);
    udp_connection_->SetWriteHandler(whisper::NewPermanentCallback(this,
        &EchoServer::ConnectionWriteHandler), true);
    udp_connection_->SetCloseHandler(whisper::NewPermanentCallback(this,
        &EchoServer::ConnectionCloseHandler), true);
    // listen on FLAGS_server_port, receive from anywhere
    if ( !udp_connection_->Open(whisper::net::HostPort(0, FLAGS_server_port)) ) {
      LOG_ERROR << " Failed to open udp_connection on port: " << FLAGS_server_port;
      Stop();
    }
  }
  void Stop() {
    selector_->DeleteInSelectLoop(this);
  }
  bool ConnectionReadHandler() {
    while ( true ) {
      const whisper::net::UdpConnection::Datagram* p = udp_connection_->PopRecvDatagram();
      if ( p == NULL ) {
        break;
      }
      scoped_ptr<const whisper::net::UdpConnection::Datagram> auto_del_p(p);
      LOG_TEST << "Message from " << p->addr_ << ", #" << p->data_.Size()
               << " bytes, replying with echo...";
      udp_connection_->SendDatagram(p->data_, p->addr_);
    }
    return true;
  }
  bool ConnectionWriteHandler() {
    return true;
  }
  void ConnectionCloseHandler(int err) {
    LOG_TEST << "ConnectionCloseHandler(), err: " << err;
    Stop();
  }
private:
  whisper::net::Selector* selector_;
  whisper::net::UdpConnection* udp_connection_;
};

// This client sends FLAGS_num_packets_per_client UDP packets of random size
// to the server localhost:FLAGS_server_port and expects replies containing
// the same packets. If timeout occurs for a packet, it will be retransmitted
// for a max amount of times.
class EchoClient {
public:
  static const char* ClassName() { return "CLIENT"; };
  struct Packet {
    uint32 try_num_;
    whisper::io::MemoryStream data_;
    Packet() : try_num_(0), data_() {}
  };
  // map: packet number -> packet
  typedef std::map<uint32, Packet*> PacketMap;
  static const int64 kRecvTimeoutIdBase = 1;
  static const int64 kRecvTimeoutValue = 5000;
  static const int64 kMsBetweenPackets = 1000;
  static const uint32 kRetransmission = 5;
  static const uint32 kPacketMinSize = 4; // must include a uint32 in every packet
  static const uint32 kPacketMaxSize = whisper::net::UdpConnection::kDatagramMaxSize/10;
public:
  EchoClient(whisper::net::Selector* selector)
    : selector_(selector),
      udp_connection_(NULL),
      step_callback_(whisper::NewPermanentCallback(this, &EchoClient::Step)),
      server_addr_("127.0.0.1", FLAGS_server_port),
      packet_min_size_(kPacketMinSize),
      packet_max_size_(kPacketMaxSize),
      packets_to_send_(FLAGS_num_packets_per_client),
      ms_between_packets_(kMsBetweenPackets),
      packet_max_try_num_(kRetransmission + 1),
      next_packet_num_(0),
      sent_packets_(),
      timeouter_(selector,
          whisper::NewPermanentCallback(this, &EchoClient::TimeoutHandler)) {
    LOG_TEST << "++EchoClient";
    selector_->RunInSelectLoop(whisper::NewCallback(this, &EchoClient::Start));
  }
  virtual ~EchoClient() {
    LOG_TEST << "--EchoClient, " << (g_num_clients-1) << " clients left";
    CHECK(sent_packets_.empty()) << "Client error: sent_packets_ not empty"
        ", there are still pending replies to receive";
    delete udp_connection_;
    udp_connection_ = NULL;
    CHECK_GT(g_num_clients, 0);
    g_num_clients--;
    if ( g_num_clients == 0 ) {
      LOG_TEST << "All clients finished. Terminating selector..";
      selector_->MakeLoopExit();
    }
  }

  void Start() {
    udp_connection_ = new whisper::net::UdpConnection(selector_);
    udp_connection_->SetReadHandler(whisper::NewPermanentCallback(this,
        &EchoClient::ConnectionReadHandler), true);
    udp_connection_->SetWriteHandler(whisper::NewPermanentCallback(this,
        &EchoClient::ConnectionWriteHandler), true);
    udp_connection_->SetCloseHandler(whisper::NewPermanentCallback(this,
        &EchoClient::ConnectionCloseHandler), true);
    if ( !udp_connection_->Open(whisper::net::HostPort(0, 0)) ) {
      LOG_ERROR << " Failed to open udp_connection";
      Stop();
    }
    // start with a random delay.. so that not all clients start at once
    selector_->RegisterAlarm(step_callback_, RandomInt(0, ms_between_packets_));
  }
  void Stop() {
    selector_->DeleteInSelectLoop(this);
  }

  Packet* GenPacket(uint32* out_packet_num) {
    CHECK_LT(next_packet_num_, packets_to_send_);
    CHECK_GE(packet_min_size_, 4);
    CHECK_GE(packet_max_size_, packet_min_size_);

    uint32 size = 0;
    uint8* data = RandomData(packet_min_size_ - 4, packet_max_size_ - 4, &size);
    scoped_ptr<uint8> auto_del_data(data);
    Packet* p = new Packet();
    whisper::io::NumStreamer::WriteUInt32(&p->data_, next_packet_num_, whisper::common::kByteOrder);
    p->data_.Write(data, size);

    std::pair<PacketMap::iterator, bool> result =
      sent_packets_.insert(make_pair(next_packet_num_, p));
    CHECK(result.second) << " Duplicate packet_num: " << next_packet_num_;
    next_packet_num_++;

    *out_packet_num = next_packet_num_ - 1;
    return p;
  }
  void VerifyPacket(const whisper::io::MemoryStream& reply, uint32* out_packet_num) {
    CHECK_GE(reply.Size(), 4);
    uint32 packet_num = whisper::io::NumStreamer::PeekUInt32(&reply, whisper::common::kByteOrder);
    PacketMap::iterator it = sent_packets_.find(packet_num);
    CHECK(it != sent_packets_.end()) << " Reply not found in sent packets: " << reply.DebugString();
    Packet* p = it->second;
    if ( reply != p->data_ ) {
      LOG_FATAL << " Reply not identical to sent packet, packet: "
                << p->data_.DebugString() << ", reply: " << reply.DebugString();
    }
    delete p;
    sent_packets_.erase(it);
    *out_packet_num = packet_num;
  }

  void SendPacket(Packet* p, uint32 packet_num) {
    p->try_num_++;
    LOG_TEST << "Sending packet " << packet_num << " containing #"
             << p->data_.Size() << " bytes to: " << server_addr_
             << " try: " << p->try_num_;
    udp_connection_->SendDatagram(p->data_, server_addr_);
    timeouter_.SetTimeout(kRecvTimeoutIdBase + packet_num, kRecvTimeoutValue);
  }
  void Step() {
    uint32 packet_num = 0;
    Packet* p = GenPacket(&packet_num);
    SendPacket(p, packet_num);
    if ( next_packet_num_ == packets_to_send_ ) {
      LOG_TEST << "All packets sent, waiting for replies..";
      return;
    }
    selector_->RegisterAlarm(step_callback_, ms_between_packets_);
  }

  void TimeoutHandler(int64 timeout_id) {
    if ( timeout_id >= kRecvTimeoutIdBase &&
         timeout_id <  kRecvTimeoutIdBase + packets_to_send_ ) {
      // recv timeout. Find packet_num from timeout_id.
      uint32 packet_num = timeout_id - kRecvTimeoutIdBase;
      LOG_WARNING << "Timeout waiting for reply on packet: " << packet_num;

      PacketMap::iterator it = sent_packets_.find(packet_num);
      CHECK(it != sent_packets_.end()) << " Timeout packet_num_: "
        << packet_num << " not found in sent packets";
      Packet* p = it->second;
      if ( p->try_num_ >= packet_max_try_num_ ) {
        LOG_FATAL << "Retry limit reached for packet_num: " << packet_num;
      }
      SendPacket(p, packet_num);
    }
  }

  bool ConnectionReadHandler() {
    while ( true ) {
      const whisper::net::UdpConnection::Datagram* p = udp_connection_->PopRecvDatagram();
      if ( p == NULL ) {
        break;
      }
      scoped_ptr<const whisper::net::UdpConnection::Datagram> auto_del_p(p);
      LOG_TEST << "Reply from " << p->addr_ << ", #" << p->data_.Size()
               << " bytes, still " << (packets_to_send_-next_packet_num_)
               << " packets to send, and " << sent_packets_.size()
               << " packets to receive.";
      uint32 packet_num;
      VerifyPacket(p->data_, &packet_num);
      timeouter_.UnsetTimeout(kRecvTimeoutIdBase + packet_num);
    }
    if ( next_packet_num_ == packets_to_send_ && sent_packets_.empty() ) {
      LOG_TEST << "All packets received, stopping client..";
      Stop();
    }
    return true;
  }
  bool ConnectionWriteHandler() {
    return true;
  }
  void ConnectionCloseHandler(int err) {
    LOG_TEST << "ConnectionCloseHandler(), err: " << err;
  }
private:
  whisper::net::Selector* selector_;
  whisper::net::UdpConnection* udp_connection_;
  whisper::Closure* step_callback_;

  const whisper::net::HostPort server_addr_;

  const uint32 packet_min_size_;
  const uint32 packet_max_size_;

  const uint32 packets_to_send_;
  const int64 ms_between_packets_;
  const uint32 packet_max_try_num_;

  // stats on 0, incremented with every packet sent.
  // This is the first uint32 in every packet. Helps identifying replies.
  uint32 next_packet_num_;
  // packets which were sent, and we're waiting reply for
  PacketMap sent_packets_;

  whisper::net::Timeouter timeouter_;
};

int main(int argc, char ** argv) {
  whisper::common::Init(argc, argv);
  whisper::net::Selector selector;

  if ( !FLAGS_just_client ) {
    new EchoServer(&selector); // auto deletes
  }
  if ( !FLAGS_just_server ) {
    CHECK_GT(FLAGS_num_clients, 0);
    g_num_clients = FLAGS_num_clients;
    for ( uint32 i = 0; i < g_num_clients; i++ ) {
      new EchoClient(&selector); // auto deletes
    }
  }

  selector.Loop();
  LOG_INFO << "Pass";
  whisper::common::Exit(0);
}
