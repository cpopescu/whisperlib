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
// Author: Cosmin Tudorache  & Catalin Popescu

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include "whisperlib/net/address.h"
#include "whisperlib/base/strutil.h"

using namespace std;

namespace whisper {

namespace net {

const uint8 IpAddress::kInvalidIpV6[16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

void IpAddress::Addr(sockaddr_storage* addr) const {
  memset(addr, 0, sizeof(*addr));
  if ( is_ipv4_ ) {
    addr->ss_family = AF_INET;
    sockaddr_in* p = reinterpret_cast<sockaddr_in*>(addr);
    // addr_.ipv4_ is in host byte order
    p->sin_addr.s_addr = htonl(addr_.ipv4_);
  } else {
    addr->ss_family = AF_INET6;
    sockaddr_in6* p = reinterpret_cast<sockaddr_in6*>(addr);
    // addr_.ipv6_ is in network byte order
    memcpy(p->sin6_addr.s6_addr, addr_.ipv6_, 16);
  }
}

string IpAddress::ToString() const {
  char addr[60] = {0,};
  if ( is_ipv4_ ) {
    int32 ipv4 = htonl(addr_.ipv4_);
    inet_ntop(AF_INET, &ipv4, addr, sizeof(addr));
  } else {
    // ipv6
    inet_ntop(AF_INET6, addr_.ipv6_, addr, sizeof(addr));
  }
  return addr;
}

/////////////////////////////////////////////////////////////////////////////

HostPort::HostPort(const struct sockaddr_storage* addr)
  : host_(), ip_(), port_(kInvalidPort) {
  if ( addr->ss_family == AF_INET ) {
    const struct sockaddr_in* p =
        reinterpret_cast<const struct sockaddr_in*>(addr);
    ip_.set_ipv4(ntohl(p->sin_addr.s_addr));
    port_ = ntohs(p->sin_port);
  } else if ( addr->ss_family == AF_INET6 ) {
    const struct sockaddr_in6* p =
        reinterpret_cast<const struct sockaddr_in6*>(addr);
    ip_.set_ipv6(p->sin6_addr.s6_addr);
    port_ = ntohs(p->sin6_port);
  }
}

bool HostPort::SockAddr(struct sockaddr_storage* addr) const {
  ip_.Addr(addr);
  if ( ip_.is_ipv4() ) {
    sockaddr_in* p = reinterpret_cast<sockaddr_in*>(addr);
    p->sin_port = htons(port_);
    return false;
  } else {
    sockaddr_in6* p = reinterpret_cast<sockaddr_in6*>(addr);
    p->sin6_port = htons(port_);
    return true;
  }
}

string HostPort::ToString() const {
  if ( host_.empty() ) {
    return strutil::StringPrintf("%s:%d",
                                 ip_.ToString().c_str(),
                                 static_cast<int>(port_));
  } else {
    return strutil::StringPrintf("%s:%d",
                                 host_.c_str(),
                                 static_cast<int>(port_));
  }
}

/////////////////////////////////////////////////////////////////////////////

// Adds an ip or a range to the filter. Returns true on OK
bool IpV4Filter::Add(const char* p) {
  const char* slash = strchr(p, '/');
  IpAddress ip;
  int32 bits = 32;
  if ( slash != NULL ) {
    ip = IpAddress(string(p, slash - p).c_str());
    slash++;
    bits = static_cast<int32>(strtol(slash, const_cast<char**>(&slash), 10));
  } else {
    ip = IpAddress(p);
  }
  return Add(ip, bits);
}

bool IpV4Filter::Add(const IpAddress& ip, int32 bits) {
  if ( ip.IsInvalid() || !ip.is_ipv4() ) {
    return false;
  }
  if ( bits > 32 || bits < 0 ) {
    return false;
  }
  const int32 shift_bits = 32 - bits;
  uint32 first_ip = ip.ipv4() & (0xffffffff << shift_bits);   // host order
  // CHECK THIS CRAP OUT:
  // when bits == 32 => 0xffffffff >> bits == 0xffffffff
  // when bits == 31 => 0xffffffff >> bits == 0x1
  uint32 last_ip = (shift_bits == 0 ?
                    first_ip : first_ip | (0xffffffff >> bits));
  FilterMap::iterator first_it = filter_.lower_bound(first_ip);
  if ( first_it != filter_.end() &&
       first_it->first > first_ip ) {
    --first_it;
  }
  FilterMap::iterator last_it = filter_.upper_bound(last_ip);

  const bool skip_insert_first =
    (first_it != filter_.end() && first_it->second == MARK_BEGIN) ||
    first_ip == last_ip;
  bool skip_insert_last =
    (last_it != filter_.end() && last_it->second == MARK_END);
  FilterMap::iterator it = first_it;
  if ( it != filter_.end() ) {
    ++it;
  }
  while ( it != last_it ) {
    FilterMap::iterator crt = it;
    ++it;
    filter_.erase(crt);
  }
  int32 first_found_ip = first_it->first;
  if ( !skip_insert_first ) {
    if ( first_it->first == first_ip &&
         ((first_it->second & MARK_END) == MARK_END) ) {
      if ( first_ip != last_ip ) {
        filter_.erase(first_it);
      } else {
        skip_insert_last = true;
      }
    }
    if ( first_found_ip != first_ip ) {
      filter_.insert(make_pair(first_ip, MARK_BEGIN));
    }
  }
  if ( !skip_insert_last ) {
    if ( first_ip != last_ip ) {
      filter_.insert(make_pair(last_ip, MARK_END));
    } else {
      filter_.insert(make_pair(last_ip, MARK_BOTH));
    }
  }
  return true;
}

// Matching function - returns true if it matches
bool IpV4Filter::Matches(const IpAddress& ip) const {
  if ( ip.IsInvalid() || !ip.is_ipv4() ) {
    return false;
  }
  FilterMap::const_iterator first_it = filter_.lower_bound(ip.ipv4());
  if ( first_it != filter_.end() &&
       first_it->first > ip.ipv4() ) {
    --first_it;
  }
  return ( first_it != filter_.end() &&
           first_it->first <= ip.ipv4() &&
           (first_it->second == MARK_BEGIN || first_it->first == ip.ipv4()) );
}
} // namespace net
} // namespace whisper
