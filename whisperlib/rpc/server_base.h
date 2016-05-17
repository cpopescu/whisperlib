// Copyright: Urban Engines, Inc. 2012 onwards.
// All rights reserved.
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
    ServerBase(int& argc, char**& argv, const string& server_name);

    virtual ~ServerBase();

    // Initialize all services here
    virtual int Initialize();

    void set_rpc_path(const string& rpc_path) { rpc_path_ = rpc_path; }
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
    void ToDebugHtmlTable(int64 id_val,
                          const vector< std::pair<string, google::protobuf::Message*> >& data,
                          http::ServerRequest* http_req);

 protected:
    const string name_;
    int num_threads_;
    int port_;
    int max_concurrent_requests_;
    string rpc_path_;

    net::Selector* selector_;
    net::NetFactory* net_factory_;
    vector<net::SelectorThread*> working_threads_;
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
