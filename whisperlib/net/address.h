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
// Author: Cosmin Tudorache & Catalin Popescu
//
#ifndef __NET_BASE_ADDRESS_H__
#define __NET_BASE_ADDRESS_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <map>
#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/strutil.h>

struct in_addr;

namespace net {

class IpAddress {
 public:
  static const int32 kInvalidIpV4 = 0;
  static const uint8 kInvalidIpV6[16];

  IpAddress()
      : is_ipv4_(true) {
    memset(&addr_, 0, sizeof(addr_));
  }
  //  ip: 4 bytes containing the IP address
  //      (host byte order: b1.b2.b3.b4 <==> 0xb1b2b3b4)
  explicit IpAddress(int32 ip)
      : is_ipv4_(true) {
    addr_.ipv4_ = ip;
  }
  IpAddress(uint8 b0 , uint8 b1 , uint8 b2 , uint8 b3 ,
            uint8 b4 , uint8 b5 , uint8 b6 , uint8 b7 ,
            uint8 b8 , uint8 b9 , uint8 b10, uint8 b11,
            uint8 b12, uint8 b13, uint8 b14, uint8 b15)
     : is_ipv4_(false) {
    addr_.ipv6_[ 0] = b0;  addr_.ipv6_[ 1] = b1;
    addr_.ipv6_[ 2] = b2;  addr_.ipv6_[ 3] = b3;
    addr_.ipv6_[ 4] = b4;  addr_.ipv6_[ 5] = b5;
    addr_.ipv6_[ 6] = b6;  addr_.ipv6_[ 7] = b7;
    addr_.ipv6_[ 8] = b8;  addr_.ipv6_[ 9] = b9;
    addr_.ipv6_[10] = b10; addr_.ipv6_[11] = b11;
    addr_.ipv6_[12] = b12; addr_.ipv6_[13] = b13;
    addr_.ipv6_[14] = b14; addr_.ipv6_[15] = b15;
  }
  // parsing constructor
  // Use IsInvalid() to check for error.
  explicit IpAddress(const string& s) {
    bool error = false;
    if ( s.size() >= 3 && s.find(':') != string::npos ) {
      is_ipv4_ = false;
      // NOTE: inet_pton delivers address in network byte order
#ifdef HAVE_INET_PTON
      error = inet_pton(AF_INET6, s.c_str(), addr_.ipv6_) <= 0;
      // we keep addr_.ipv6_ in network byte order
#else
      LOG_ERROR << "inet_pton not supported on this system.";
      error = -1;
#endif
    } else {
      is_ipv4_ = true;
#ifdef HAVE_INET_PTON
      // NOTE: inet_pton delivers address in network byte order
      error = inet_pton(AF_INET, s.c_str(), &addr_.ipv4_) <= 0;
#else
      LOG_ERROR << "inet_pton not supported on this system.";
      error = -1;
#endif
      // we keep addr_.ipv4_ in host byte order
      addr_.ipv4_ = ntohl(addr_.ipv4_);
    }
    if ( error ) {
      memset(&addr_, 0, sizeof(addr_));
    }
  }
  IpAddress(const IpAddress& other)
    : is_ipv4_(other.is_ipv4_),
      addr_(other.addr_) {
  }
  IpAddress& operator=(const IpAddress& addr) {
    is_ipv4_ = addr.is_ipv4_;
    memcpy(&addr_, &addr.addr_, sizeof(addr_));
    return *this;
  }
  IpAddress& operator=(int32 ip) {
    is_ipv4_ = true;
    addr_.ipv4_ = ip;
    return *this;
  }
  // comparison needed if you intend to use set<IpAddress>
  bool operator<(const IpAddress& other) const {
    if ( is_ipv4() != other.is_ipv4() ) {
      return is_ipv4(); // ipv4 < ipv6
    }
    if ( is_ipv4() ) {
      return ipv4() < other.ipv4();
    }
    // ipv6 - // memcmp("a","d",1) == -3
    return ::memcmp(addr_.ipv6_, other.addr_.ipv6_, sizeof(addr_.ipv6_)) < 0;
  }
  bool operator==(const IpAddress& other) const {
    if ( is_ipv4() != other.is_ipv4() ) {
      return false;
    }
    if ( is_ipv4() ) {
      return ipv4() == other.ipv4();
    }
    return ::memcmp(addr_.ipv6_, other.addr_.ipv6_, sizeof(addr_.ipv6_)) == 0;
  }

  bool IsInvalid() const {
    if ( is_ipv4_ ) {
      return addr_.ipv4_ == kInvalidIpV4;
    } else {
      return (0 == memcmp(addr_.ipv6_, kInvalidIpV6, sizeof(addr_.ipv6_)));
    }
  }
  bool is_ipv4() const {
    return is_ipv4_;
  }
  int32 ipv4() const {
    CHECK(is_ipv4_);
    return addr_.ipv4_;
  }
  void set_ipv4(int32 ip) {
    is_ipv4_ = true;
    addr_.ipv4_ = ip;
  }
  void set_ipv6(const uint8* ip) {
    is_ipv4_ = false;
    memcpy(addr_.ipv6_, ip, 16);
  }

  // returns the ip address in structure..
  void Addr(sockaddr_storage* addr) const;

  // Same as above, but returns a string;
  string ToString() const;

 private:
  bool is_ipv4_;
  union {
    int32 ipv4_;         // "b1.b2.b3.b4" <==> 0xb1b2b3b4 . host byte order
    uint8 ipv6_[16];     // {b1,b2,b3,..,b16} . network byte order
  } addr_;
};


class HostPort {
 public:
  static const uint16 kInvalidPort = 0;
  HostPort()
      : host_(), ip_(), port_(kInvalidPort) {
  }
  HostPort(int32 ip, uint16 port)
      : host_(), ip_(ip), port_(port) {
  }
  HostPort(const IpAddress& ip, uint16 port)
      : host_(), ip_(ip), port_(port) {
  }
  // Parsing constructor
  // e.g.: "10.205.9.85:2345" -> HOST="", IP=10.205.9.85, PORT=2345
  //       "10.205.9.85" -> HOST="", IP=10.205.9.85, PORT=0
  //       "google.com" -> HOST="google.com", IP=0, PORT=0
  //       "google.ro:80" -> HOST="google.ro", IP=0, PORT=80
  HostPort(const string& hostport)
      : host_(), ip_(), port_(kInvalidPort) {
    pair<string, string> a = strutil::SplitLast(hostport, ":");
    ip_ = IpAddress(a.first);
    if ( ip_.IsInvalid() ) {
      host_ = a.first;
    }
    long long int port = ::atoll(a.second.c_str());
    port_ = (port <= 0 || port > kMaxUInt16) ? kInvalidPort : port;
  }
  // separate host / port
  HostPort(const string& host, uint16 port)
      : host_(), ip_(host), port_(port) {
    if ( ip_.IsInvalid() ) {
      host_ = host;
    }
    port_ = port;
  }
  HostPort(const HostPort& other)
      : host_(other.host_), ip_(other.ip_), port_(other.port_) {
  }
  // From net order of addr converts to machine order in HostPort
  explicit HostPort(const struct sockaddr_storage* addr);

  HostPort& operator=(const HostPort& other) {
    host_ = other.host_;
    ip_ = other.ip_;
    port_ = other.port_;
    return *this;
  }

  const string& host() const { return host_; }

  // Accessors / setters
  const IpAddress& ip_object() const { return ip_; }

  void set_ipv4(int32 ip) { ip_ = ip; }
  void set_ip(const IpAddress& ip) { ip_ = ip; }

  uint16 port() const { return port_; }
  void set_port(uint16 port) { port_ = port; }

  bool IsInvalid() const {
    return host_.empty() && (ip_.IsInvalid() || IsInvalidPort());
  }
  bool IsInvalidPort() const {
    return port_ == kInvalidPort;
  }

  // Returns in addr the ip address in network byte order. Returns true on ipv6
  bool SockAddr(struct sockaddr_storage* addr) const;

  string ToString() const;

 private:
  string host_;
  IpAddress ip_; // sometimes obtained by DNS lookup of 'host_'
  uint16 port_;
};

inline ostream& operator<<(ostream& os, const IpAddress& ip) {
  return os << ip.ToString();
}
inline ostream& operator<<(ostream& os, const HostPort& hp) {
  return os << hp.ToString();
}

// This is a filter of addresses. Add ip strings like "12.23.4.2" or
// range specification "192.168.2.23/24" or IPV4Address -es, then
// check with a test IP if it matches.
class IpV4Filter {
 public:
  IpV4Filter() {
  }
  ~IpV4Filter() {
  }

  // Adds an ip or a range to the filter. Returns true on OK
  bool Add(const char* p);
  bool Add(const IpAddress& ip, int32 bits = 32);

  // Matching function - returns true if it matches
  bool Matches(const IpAddress& ip) const;

 private:
  enum ElemType {
    MARK_BEGIN = 1,
    MARK_END = 2,
    MARK_BOTH = 3,
  };
  typedef map<uint32, int32> FilterMap;
  FilterMap filter_;
};
}

#endif  // __NET_BASE_ADDRESS_H__
