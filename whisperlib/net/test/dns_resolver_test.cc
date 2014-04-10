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

#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/base/scoped_ptr.h>

#include <whisperlib/net/timeouter.h>
#include <whisperlib/net/address.h>
#include <whisperlib/net/selector.h>
#include <whisperlib/net/dns_resolver.h>

//////////////////////////////////////////////////////////////////////

DEFINE_bool(just_server,
            false,
            "True runs only the server.");

#define LOG_TEST LOG_INFO

uint32 g_pending = 0;
net::Selector* g_selector = NULL;

void HandleDnsResult(string hostname, bool expected_success,
    scoped_ref<net::DnsHostInfo> info) {
  LOG_INFO << "Got info " << info->is_valid();
  CHECK(expected_success == info->is_valid())
      << "For hostname: [" << hostname
      << "], expected_success: " << strutil::BoolToString(expected_success)
      << ", result: " << info.ToString();

  LOG_TEST << "Resolved: [" << hostname << "], to: " << info.ToString()
           << ", expected_success: "
           << strutil::BoolToString(expected_success);

  CHECK_GT(g_pending, 0);
  g_pending--;

  if ( g_pending == 0 ) {
    g_selector->RunInSelectLoop(NewCallback(g_selector,
        &net::Selector::MakeLoopExit));;
  }
}

void TestDnsQuery(const string& hostname, bool expected_success) {
  g_pending++;
  net::DnsResolve(g_selector, hostname, NewPermanentCallback(&HandleDnsResult,
      hostname, expected_success));
}

int main(int argc, char ** argv) {
  common::Init(argc, argv);
  g_selector = new net::Selector();

  TestDnsQuery("google.com", true);
  TestDnsQuery("localhost", true);
  TestDnsQuery("asd90fas8df0as8d908asd0", false);

  g_selector->Loop();
  delete g_selector;
  g_selector = NULL;

  LOG_INFO << "Pass";
  common::Exit(0);
}
