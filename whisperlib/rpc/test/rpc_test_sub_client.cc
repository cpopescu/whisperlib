// Copyright: Urban Engines, Inc. 2014 onwards.
// All rights reserved.

#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/system.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/base/timer.h"

#include "whisperlib/net/selector.h"
#include "whisperlib/http/http_client_protocol.h"
#include "whisperlib/rpc/rpc_http_client.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/rpc/client_net.h"
#include "whisperlib/rpc/test/rpc_test_proto.pb.h"

DEFINE_string(server, "127.0.0.1:8222",
              "Talk to this server");
DEFINE_int64(seq_start, 0,
             "Subscribe for this sequence");
DEFINE_int64(seq_end, 0,
             "Subscribe to this sequence");

using namespace whisper;

struct ClientData {
    net::Selector selector;
    http::ClientParams params;
    rpc::ClientNet* client_net;
    rpc::HttpClient* client_rpc;
    rpc::TestStreamService_Stub* client_stub;

    int64 next_id;
    rpc::TestSubReq req;
    rpc::TestSubReply reply;
    rpc::Controller* controller;
    google::protobuf::Closure* completion;
    ClientData()
        : client_net(new rpc::ClientNet(
                         &selector, &params, rpc::ClientNet::ToServerVec(FLAGS_server))),
          client_rpc(new rpc::HttpClient(
                         client_net->fsc(), "/rpc/rpc.TestStreamService")),
          client_stub(new rpc::TestStreamService_Stub(
                          client_rpc, ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL)),
          next_id(FLAGS_seq_start),
          controller(NULL),
          completion(google::protobuf::NewPermanentCallback(this, &ClientData::More)) {
        params.max_chunk_size_ = 1 << 30;
        // params.dlog_level_ = true;
    }
    ~ClientData() {
        delete controller; controller = NULL;
        delete completion;
    }
    void Start() {
        if (next_id) {
            req.set_seq(next_id);
        }
        if (FLAGS_seq_end) {
            req.set_end_seq(FLAGS_seq_end);
        }
        controller = new rpc::Controller();
        controller->set_is_streaming(true);
        client_stub->Subscribe(controller, &req, &reply, completion);
    }
    void Close() {
        selector.DeleteInSelectLoop(client_stub);
        client_rpc->StartClose();
        selector.DeleteInSelectLoop(client_net);
        selector.MakeLoopExit();
    }
private:
    void More() {
        if (controller->Failed()) {
            LOG_ERROR << " Subscription ended - will retry: " << controller->ErrorText();
            delete controller; controller = NULL;
            selector.RegisterAlarm(::NewCallback(this, &ClientData::Start), 5000);
        } else {
            while (controller->HasStreamedMessage()) {
                pair<google::protobuf::Message*, bool> data = controller->PopStreamedMessage();
                if (data.second) {
                    if (data.first) {
                        // LOG_INFO << " New Message: " << data.first->ShortDebugString();
                        rpc::TestSubReply* r = static_cast<rpc::TestSubReply*>(
                            data.first);
                        next_id = r->seq() + 1;
                        delete r;
                    } else {
                        LOG_INFO << " Stream ended. ";
                        Close();
                        return;
                    }
                } else {
                    break;
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  ClientData client;
  client.selector.RunInSelectLoop(::NewCallback(&client, &ClientData::Start));
  client.selector.Loop();

}
