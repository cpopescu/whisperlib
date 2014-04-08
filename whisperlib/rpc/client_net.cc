// Copyright: 1618labs, Inc. 2013 onwards.
// All rights reserved.
// cp@1618labs.com
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
//

#include <whisperlib/rpc/client_net.h>
#include <whisperlib/http/http_client_protocol.h>
#include <whisperlib/http/failsafe_http_client.h>

namespace {
http::BaseClientConnection* CreateConnection(net::Selector* selector,
                                             net::NetFactory* net_factory,
                                             net::PROTOCOL net_protocol) {
    return new http::SimpleClientConnection(selector, *net_factory, net_protocol);
}
}


namespace rpc {

ClientNet::ClientNet(net::Selector* selector,
                     const http::ClientParams* params,
                     const vector<net::HostPort>& servers)
    : selector_(selector),
      net_factory_(selector),
      fsc_(new http::FailSafeClient(selector_, params, servers,
              NewPermanentCallback(&CreateConnection, selector_, &net_factory_, net::PROTOCOL_TCP),
              true, 4, 40000, 5000, "")) {
}
ClientNet::ClientNet(net::Selector* selector,
                     const http::ClientParams* params,
                     const string& server_name, int num_connections, const char* host_header)
    : selector_(selector),
      net_factory_(selector),
      fsc_(new http::FailSafeClient(selector_, params, ToServerArray(server_name, num_connections),
              NewPermanentCallback(&CreateConnection, selector_, &net_factory_, net::PROTOCOL_TCP),
              true, 5, 40000, 5000, host_header == NULL ? "" : host_header)) {
}
vector<net::HostPort> ClientNet::ToServerArray(const string& server_name,
                                               int num_connections) {
  vector<net::HostPort> servers;
  CHECK(!server_name.empty());
  for (int i = 0; i < num_connections; ++i) {
      servers.push_back(net::HostPort(server_name));
  }
  return servers;
}

ClientNet::~ClientNet() {
    selector_->DeleteInSelectLoop(fsc_);
}

}
