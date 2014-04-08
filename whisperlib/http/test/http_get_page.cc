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

// Simple wget :)

#include <netdb.h>
#include <iostream>
#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/http/http_client_protocol.h>
#include <whisperlib/net/selector.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(url,
              "",
              "URL to download");

//////////////////////////////////////////////////////////////////////

void RequestDone(net::Selector* selector,
                 http::ClientRequest* req) {
  CHECK(req->is_finalized());
  LOG_INFO << " Request " << req->name() << " finished w/ error: "
           << req->error_name();
  LOG_INFO << " Response received from server (so far): "
           << "\nHeader:\n"
           << req->request()->server_header()->ToString();
  cout << req->request()->server_data()->ToString();
  selector->MakeLoopExit();
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_url.empty());
  URL url(FLAGS_url);
  CHECK(url.is_valid()) << " Invalid url given !";
  struct hostent *hp = gethostbyname(url.host().c_str());

  CHECK(hp != NULL)
    << " Cannot resolve: " << url.host();
  CHECK(hp->h_addr_list[0] != NULL)
    << " BAd address received for: " << url.host();

  net::HostPort server(
    ntohl(reinterpret_cast<struct in_addr*>(
            hp->h_addr_list[0])->s_addr),
    url.IntPort() == -1 ? 80 : url.IntPort());

  net::Selector selector;
  http::ClientParams params;
  params.dlog_level_ = true;

  // Initialize protocol parameters. We can use TCP or SSL under HTTP
  net::NetFactory net_factory(&selector);
  net::PROTOCOL protocol = net::PROTOCOL_TCP;

#ifdef USE_SSL
  SSL_CTX* ctx = NULL;
  // Deal with https (use SSL -> initialize and set parameters to use it)
  if (url.SchemeIsSecure()) {
    ctx = net::SslConnection::SslCreateContext();
    net_factory.SetSslConnectionParams(net::SslConnectionParams(ctx));
    protocol = net::PROTOCOL_SSL;
  }
#endif
  // Construct the connection
  http::SimpleClientConnection* const conn =
      new http::SimpleClientConnection(&selector, net_factory, protocol);

  http::ClientProtocol proto(&params, conn, server);
  http::ClientRequest req(http::METHOD_GET, &url);

  Closure* const done_callback =
    NewCallback(&RequestDone, &selector, &req);
  selector.RunInSelectLoop(
    NewCallback(&proto,
                &http::ClientProtocol::SendRequest,
                &req,
                done_callback));
  selector.Loop();
}
