// Copyright: Urban Engines, Inc. 2011 onwards.
// All rights reserved.


#include "whisperlib/rpc/test/rpc_test_proto.pb.h"
#include "whisperlib/rpc/rpc_http_server.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/net/selector.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/base/system.h"

DEFINE_int32(port, 8980, "Serve on this port");
DEFINE_int32(num_threads, 4, "Serve with these may worker threads");
DEFINE_int64(min_delay_ms, 100, "Delay the response by at least this time");
DEFINE_int64(max_delay_ms, 100 + 256, "Delay the response by at most this time");

namespace whisper {
namespace rpc {
void Complete(::google::protobuf::Closure* done) {
    done->Run();
}

class TestServiceImpl : public TestService {
public:
    TestServiceImpl() {
    }

    virtual void Mirror(::google::protobuf::RpcController* controller,
                        const whisper::rpc::TestReq* request,
                        whisper::rpc::TestReq* response,
                        ::google::protobuf::Closure* done) {
        response->CopyFrom(*request);
        DelayedResponse(controller, done);
    }
    virtual void Sum(::google::protobuf::RpcController* controller,
                     const whisper::rpc::TestReq* request,
                     whisper::rpc::TestReply* response,
                     ::google::protobuf::Closure* done) {
        if (request->has_s()) {
            response->set_s(request->s());
        }
        response->set_z(int64(request->x()) + int64(request->y()));
        DelayedResponse(controller, done);
    }
    virtual void Multiply(::google::protobuf::RpcController* controller,
                          const whisper::rpc::TestReq* request,
                          whisper::rpc::TestReply* response,
                          ::google::protobuf::Closure* done) {
        if (request->has_s()) {
            response->set_s(request->s());
        }
        response->set_z(int64(request->x()) * int64(request->y()));
        DelayedResponse(controller, done);
    }

private:
    void DelayedResponse(::google::protobuf::RpcController* controller,
                         ::google::protobuf::Closure* done) {
        if (FLAGS_max_delay_ms < FLAGS_min_delay_ms) {
            done->Run();
        } else {
            const int64 delay = FLAGS_min_delay_ms + random() % (FLAGS_max_delay_ms -
                                                                 FLAGS_min_delay_ms);
            rpc::Controller* rpc_controller = reinterpret_cast<rpc::Controller*>(controller);
            rpc_controller->transport()->selector()->RegisterAlarm(
                whisper::NewCallback(&Complete, done), delay);
        }
    }

    DISALLOW_EVIL_CONSTRUCTORS(TestServiceImpl);
};
}
}

using namespace whisper;

void QuitServer(net::Selector* selector, http::ServerRequest* req) {
    selector->RunInSelectLoop(whisper::NewCallback(selector, &net::Selector::MakeLoopExit));
}


int main(int argc, char* argv[]) {
  whisper::common::Init(argc, argv);

  // Drives the networking show:
  whisper::net::Selector selector;

  // Prepare the serving therads
  vector<whisper::net::SelectorThread*> working_threads;
  for ( int i = 0; i < FLAGS_num_threads; ++i ) {
    working_threads.push_back(new net::SelectorThread());
    working_threads.back()->Start();
  }

  // Setup connection worker threads - these are the ones that process the requests
  whisper::net::NetFactory net_factory(&selector);
  whisper::net::TcpAcceptorParams acceptor_params;
  acceptor_params.set_client_threads(&working_threads);
  net_factory.SetTcpParams(acceptor_params);

  // Parameters for the http server (how requests from clients are processed)
  whisper::http::ServerParams params;
  params.max_reply_buffer_size_ = 1<<18;
  // params.dlog_level_ = true;

  // The server that knows to talk http
  whisper::http::Server server("Test Server",         // the name of the server
                      &selector,             // drives networking
                      net_factory,           // parametes for connections
                      params);               // parameters for http
  server.AddAcceptor(whisper::net::PROTOCOL_TCP, whisper::net::HostPort(0, FLAGS_port));

  whisper::rpc::TestServiceImpl service;
  whisper::rpc::HttpServer rpc_server(&server, NULL, "/baserpc", true, 100, "");
  CHECK(rpc_server.RegisterService("test", &service));

  // Register the quit function.
  server.RegisterProcessor("/quitquitquit",
                           whisper::NewPermanentCallback(&QuitServer, &selector),
                           true, true);

  // Start the server as soon as networking starts
  selector.RunInSelectLoop(whisper::NewCallback(&server, &whisper::http::Server::StartServing));

  // Write a message for the user:
  LOG_INFO << " ==========> Starting. Point your client to: "
           << "http://localhost:" << FLAGS_port << "/";

  selector.Loop();
  for (int i = 0; i < working_threads.size(); ++i) {
    delete working_threads[i];
  }

  LOG_INFO << "DONE";
}
