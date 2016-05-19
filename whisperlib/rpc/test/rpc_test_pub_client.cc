/** Copyright (c) 2014, Urban Engines inc.
 * All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Urban Engines inc nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Catalin Popescu
 */

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
