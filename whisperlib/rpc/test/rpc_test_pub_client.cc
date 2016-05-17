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
#include "whisperlib/io/file/file_input_stream.h"

DEFINE_string(server, "127.0.0.1:8222",
              "Talk to this server");
DEFINE_string(post, "",
              "Text to post - not empty");
DEFINE_string(post_file, "",
              "Text to post - not empty");

using namespace whisper;

struct ClientData {
    net::Selector selector;
    http::ClientParams params;
    rpc::ClientNet* client_net;
    rpc::HttpClient* client_rpc;
    rpc::TestStreamService_Stub* client_stub;

    rpc::TestPubReq req;
    rpc::TestPubReply reply;
    rpc::Controller controller;

    ClientData()
        : client_net(new rpc::ClientNet(
                         &selector, &params, rpc::ClientNet::ToServerVec(FLAGS_server))),
          client_rpc(new rpc::HttpClient(
                         client_net->fsc(), "/rpc/rpc.TestStreamService")),
          client_stub(new rpc::TestStreamService_Stub(
                          client_rpc, ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL)) {
        params.max_chunk_size_ = 1 << 30;
    }
    ~ClientData() {
    }
    void Start(const char* text) {
        req.mutable_data()->set_s(text);
        client_stub->Publish(&controller, &req, &reply,
                             ::google::protobuf::internal::NewCallback(this, &ClientData::Done));
    }
    void Close() {
        selector.DeleteInSelectLoop(client_stub);
        client_rpc->StartClose();
        selector.DeleteInSelectLoop(client_net);
        selector.MakeLoopExit();
    }
private:
    void Done() {
        if (controller.Failed()) {
            LOG_ERROR << " Post error: " << controller.ErrorText();
        } else {
            LOG_INFO << " Post succeeded: " << reply.DebugString();
        }
        Close();
    }
};

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_post.empty() || !FLAGS_post_file.empty()) << "Please set a text to post";
  std::string data = FLAGS_post;
  if (!FLAGS_post_file.empty()) {
      data = io::FileInputStream::ReadFileOrDie(FLAGS_post_file);
  }
  ClientData client;
  client.selector.RunInSelectLoop(
      ::NewCallback(&client, &ClientData::Start, data.c_str()));
  client.selector.Loop();

}
