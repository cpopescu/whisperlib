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

#ifndef __WHISPERLIB_RPC_CLIENT_NET_H__
#define __WHISPERLIB_RPC_CLIENT_NET_H__
#include <whisperlib/base/types.h>

#include <whisperlib/net/selector.h>
#include <whisperlib/net/address.h>
#include <whisperlib/net/connection.h>

namespace http {

struct ClientParams;
class FailSafeClient;
class BaseClientConnection;
}

namespace rpc {

/** Class that wraps the common network stuff for creating
 * an rpc client for general purpose
 */
class ClientNet {
public:
    ClientNet(net::Selector* selector,
              const http::ClientParams* params,
              const vector<net::HostPort>& servers);
    ClientNet(net::Selector* selector,
              const http::ClientParams* params,
              const string& server_name, int num_connections = 1,
              const char* host_header = NULL);
    ~ClientNet();

    http::FailSafeClient* fsc() const {
        return fsc_;
    }

    static vector<net::HostPort> ToServerArray(const string& server_name,
                                               int num_connections);

private:
    net::Selector* const selector_;
    net::NetFactory net_factory_;
    http::FailSafeClient* const fsc_;
};

}

#endif  // __WHISPERLIB_RPC_CLIENT_NET_H__
