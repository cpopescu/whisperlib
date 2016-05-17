// Copyright (c) 2011, Whispersoft s.r.l.
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
//
#ifndef __NET_BASE_DNS_RESOLVER_H__
#define __NET_BASE_DNS_RESOLVER_H__

#include <string>
#include <vector>
#include "whisperlib/base/types.h"
#include "whisperlib/base/ref_counted.h"
#include "whisperlib/base/callback/callback1.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/net/address.h"
#include "whisperlib/net/selector.h"
#include "whisperlib/sync/mutex.h"

using std::string;

namespace whisper {
namespace net {

// Asynchronous DNS resolver.
// Usage:
//   - DnsInit() // before resolving anything
//   - DnsResolve(..) // start a DNS query
//   either: - your result callback is called with the resolved query
//           - Or you call DnsCancel(..) to cancel the query.
// Implementation:
//   Uses a single separate thread (solver) to resolve the queries.
//   The solver receives queries through a limited ProducerConsumerQueue.
//   The actual resolve uses ::getaddrinfo().. but that may be changed.

struct DnsHostInfo : public RefCounted {
  bool valid_;
  string hostname_;
  int64 time_;
  std::vector<IpAddress> ipv4_;
  std::vector<IpAddress> ipv6_;
    DnsHostInfo(const string& hostname)
    : RefCounted(),
      valid_(false), hostname_(hostname), time_(timer::TicksNsec()){
  }
  DnsHostInfo(const string& hostname, std::set<IpAddress> ipv4, std::set<IpAddress> ipv6)
    : RefCounted(),
      valid_(true),
      hostname_(hostname),
      time_(timer::TicksNsec()),
      ipv4_(ipv4.begin(), ipv4.end()),
      ipv6_(ipv6.begin(), ipv6.end()) {}
  string ToString() const {
    return strutil::StringPrintf(
        "DnsHostInfo{hostname_: '%s', ipv4_: %s, ipv6_: %s}",
        hostname_.c_str(),
        strutil::ToString(ipv4_).c_str(),
        strutil::ToString(ipv6_).c_str());
  }
  bool is_valid() {
      return valid_;
  }
  bool is_expired() {
      return !valid_ && (timer::TicksNsec() - time_) > 5000000000LL;  // 5 sec
  }
};
typedef Callback1< scoped_ref<DnsHostInfo> > DnsResultHandler;

// Initialize DNS.
void DnsInit();

// Queue a DNS query. The result will be delivered through 'result_handler'
// on the given 'selector'.
// On success, the result is a vector containing all host IPs.
// On error, an the result is an empty vector.
void DnsResolve(net::Selector* selector, const string& hostname,
                DnsResultHandler* result_handler);

// Cancel a pending DNS query.
void DnsCancel(DnsResultHandler* result_handler);

// Stop DNS.
void DnsExit();

// Synchronous DNS query.
scoped_ref<DnsHostInfo> DnsBlockingResolve(const string& hostname);

}  // namespace net
}  // namespace whisper

#endif // __NET_BASE_DNS_RESOLVER_H__
