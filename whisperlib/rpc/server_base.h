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
// Utility that allows someone to easily build an rpc server.
//

#ifndef __WHISPERLIB_RPC_SERVER_BASE_H__
#define __WHISPERLIB_RPC_SERVER_BASE_H__

#include <whisperlib/base/app.h>
#include <whisperlib/net/selector.h>
#include <whisperlib/net/connection.h>
#include <whisperlib/http/http_server_protocol.h>
#include <google/protobuf/message.h>

namespace rpc {

class ServerBase : public app::App {
public:
    ServerBase(int argc, char** argv,
               const string& name);

    virtual ~ServerBase();

protected:
    virtual void Run() {
        selector_->Loop();
    }
    void set_net_params(int num_threads, int port) {
        num_threads_ = num_threads;
        port_ = port;
    }

    // Initialize all services here
    virtual int Initialize();

    // Stop all services here
    virtual void Stop();

    // Delete all your serving stuff here
    virtual void Shutdown();

    // In children override this to do the stopping (before
    // calling ServerBase::StopInSelectThread
    virtual void StopInSelectThread();

    // Utility to get the last path component as an in64
    int64 GetDebugIdParam(http::ServerRequest* http_req);
    // Another debug utility to print in a html table the provided data
    void ToDebugHtmlTable(int64 id_val,
                          const vector< pair<string, google::protobuf::Message*> >& data,
                          http::ServerRequest* http_req);

    const string name_;
    int num_threads_;
    int port_;

    net::Selector* selector_;
    net::NetFactory* net_factory_;
    vector<net::SelectorThread*> working_threads_;
    http::Server* http_server_;

    net::TcpAcceptorParams acceptor_params_;
    http::ServerParams server_params_;
    synch::Mutex stop_mutex_;
private:
    void TriggerStop() {
        StopInSelectThread();
    }

    DISALLOW_EVIL_CONSTRUCTORS(ServerBase);
};

}

#endif  // __WHISPERLIB_RPC_SERVER_BASE_H__
