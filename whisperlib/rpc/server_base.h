// Copyright: Urban Engines, Inc. 2012 onwards.
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
// * Neither the name of Urban Engines inc nor the names of its
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
// cp@urbanengines.com
//
// Utility that allows someone to easily build an rpc server.
//

#ifndef __WHISPERLIB_RPC_SERVER_BASE_H__
#define __WHISPERLIB_RPC_SERVER_BASE_H__

#include "whisperlib/base/app.h"
#include "whisperlib/net/selector.h"
#include "whisperlib/net/connection.h"
#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/rpc/rpc_http_server.h"
#include <google/protobuf/message.h>

namespace whisper {
namespace rpc {

// Base class for a generic HTTP & RPC server.
// There's no real reason to derive from this.
//
// Usage:
//   ServerBase server(argc, argv, "MyServer");
//   server.set_net_params(http_threads, port);
//   server.mutable_http_params()-> .. tune HTTP if you like
//   server.Initialize();
//   server.mutable_http_server()->RegisterProcessor("/my/path", NewPermanentCallback(...));
//   server.mutable_http_server()->RegisterProcessor("/another/path", NewPermanentCallback(...));
//   ...
//   server.mutable_rpc_server()->RegisterService("rpc_service_name", rpc_service_instance)
//   server.mutable_rpc_server()->RegisterService("another_rpc_service", rpc_service2)
//   ...
//   server.Main(); // blocks indefinitely, serving HTTP/RPC requests
//

class ServerBase : public app::App {
public:
    ServerBase(int& argc, char**& argv, const std::string& server_name);

    virtual ~ServerBase();

    // Initialize all services here
    virtual int Initialize();

    void set_rpc_path(const std::string& rpc_path) { rpc_path_ = rpc_path; }
    void set_net_params(int num_threads, int port) {
        num_threads_ = num_threads;
        port_ = port;
    }
    void set_max_concurrent_requests(int max_concurrent_requests) {
        max_concurrent_requests_ = max_concurrent_requests;
    }

    net::Selector* mutable_selector() { return selector_; }
    http::Server* mutable_http_server() { return http_server_; }
    http::ServerParams* mutable_http_params() { return &http_params_; }
    rpc::HttpServer* mutable_rpc_server() { return rpc_server_; }

 protected:
    virtual void Run() {
        selector_->Loop();
    }

    // Stop all services here
    virtual void Stop();

    // Delete all your serving stuff here
    virtual void Shutdown();

    // In children override this to do the stopping (before
    // calling ServerBase::StopInSelectThread
    virtual void StopInSelectThread();

    // This is the good place where you can delete your rpc services and the
    // rpc server - after the http server is closed and loop is still active.
    virtual void StopServicesInSelectThread() {};

    // Utility to get the last path component as an in64
    int64 GetDebugIdParam(http::ServerRequest* http_req);
    // Another debug utility to print in a html table the provided data
    void ToDebugHtmlTable(
        int64 id_val,
        const std::vector< std::pair<std::string, google::protobuf::Message*> >& data,
        http::ServerRequest* http_req);

 protected:
    const std::string name_;
    int num_threads_;
    int port_;
    int max_concurrent_requests_;
    std::string rpc_path_;

    net::Selector* selector_;
    net::NetFactory* net_factory_;
    std::vector<net::SelectorThread*> working_threads_;
    http::Server* http_server_;
    rpc::HttpServer* rpc_server_;

    net::TcpAcceptorParams acceptor_params_;
    http::ServerParams http_params_;
    synch::Mutex stop_mutex_;

private:
    void TriggerStop() {
        StopInSelectThread();
    }

    DISALLOW_EVIL_CONSTRUCTORS(ServerBase);
};

}  // namespace rpc
}  // namespace whisper

#endif  // __WHISPERLIB_RPC_SERVER_BASE_H__
