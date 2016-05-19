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

#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/system.h"

#include "whisperlib/net/address.h"

int main(int argc, char* argv[]) {
  whisper::common::Init(argc, argv);
  {
    whisper::net::IpAddress ip1;
    CHECK(ip1.IsInvalid());
  }
  {
    whisper::net::IpAddress ip1("::1");
    CHECK(!ip1.IsInvalid());
    CHECK(!ip1.is_ipv4());
    CHECK_EQ(ip1.ToString(), std::string("::1"));
  }
  {
    whisper::net::IpAddress ip1(0x01020304);
    whisper::net::IpAddress ip2("1.2.3.4");
    CHECK(!ip1.IsInvalid());
    CHECK(!ip2.IsInvalid());
    CHECK_EQ(ip1.ipv4(), ip2.ipv4());
    CHECK_EQ(ip1.ToString(), std::string("1.2.3.4"));
    sockaddr_storage addr;
    ip1.Addr(&addr);
    CHECK_EQ((reinterpret_cast<const sockaddr_in&>(addr)).sin_addr.s_addr,
             htonl(0x01020304));
  }
  {
    whisper::net::HostPort hp1;
    CHECK(hp1.IsInvalid());
  }
  {
    whisper::net::HostPort hp1("1.2.3.4", 1024);
    CHECK(hp1.ip_object().is_ipv4());
    CHECK_EQ(hp1.ip_object().ipv4(), 0x01020304);
    CHECK_EQ(hp1.port(), 1024);
  }
  {
    whisper::net::HostPort hp1(0xfcfdfeff, 0xffff);
    CHECK(hp1.ip_object().is_ipv4());
    CHECK_EQ(hp1.ip_object().ipv4(), 0xfcfdfeff);
    CHECK_EQ(hp1.port(), 0xffff);
    CHECK_EQ(hp1.ToString(), std::string("252.253.254.255:65535"));
    sockaddr_storage addr;
    hp1.SockAddr(&addr);
    CHECK_EQ((reinterpret_cast<const sockaddr_in&>(addr)).sin_addr.s_addr,
             htonl(0xfcfdfeff));
  }
  {
    whisper::net::HostPort addr("1.2.3.4:5678");
    CHECK(addr.ip_object().is_ipv4());
    CHECK_EQ(addr.ip_object().ipv4(), 0x01020304);
    CHECK_EQ(addr.port(), 5678);
    CHECK_STREQ(addr.ToString().c_str(), "1.2.3.4:5678");
  }

  {
    whisper::net::IpV4Filter filter;
    CHECK(filter.Add("1.2.3.0/26"));
    CHECK(filter.Matches(whisper::net::IpAddress("1.2.3.35")));
    CHECK(filter.Add("1.2.4.0/24"));
    CHECK(!filter.Matches(whisper::net::IpAddress("1.2.3.254")));
    CHECK(filter.Matches(whisper::net::IpAddress("1.2.4.254")));
    CHECK(!filter.Matches(whisper::net::IpAddress("1.2.5.0")));
    CHECK(filter.Add("1.2.5.240/28"));
    CHECK(filter.Matches(whisper::net::IpAddress("1.2.5.254")));
    CHECK(!filter.Matches(whisper::net::IpAddress("1.2.5.23")));
    CHECK(filter.Add("1.2.5.0/24"));
    CHECK(filter.Matches(whisper::net::IpAddress("1.2.5.0")));
    CHECK(filter.Matches(whisper::net::IpAddress("1.2.4.255")));
    CHECK(!filter.Matches(whisper::net::IpAddress("1.2.7.124")));
    CHECK(filter.Add("1.2.5.0/28"));
    CHECK(filter.Matches(whisper::net::IpAddress("1.2.5.254")));
    CHECK(filter.Matches(whisper::net::IpAddress("1.2.5.23")));
  }

  LOG_INFO << "PASS";
  whisper::common::Exit(0);
}
