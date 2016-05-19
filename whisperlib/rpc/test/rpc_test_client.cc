/** Copyright (c) 2011, Urban Engines inc.
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
#include "whisperlib/net/selector.h"
#include "whisperlib/http/failsafe_http_client.h"

#include "whisperlib/rpc/test/rpc_test_proto.pb.h"
#include "whisperlib/rpc/rpc_http_client.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/net/selector.h"

DEFINE_string(servers, "",
              "Rpc servers to use");
DEFINE_int32(num_requests, 1,
             "Number of requests to send");
DEFINE_int64(min_delay_ms, 100, "Delay the response by at least this time");
DEFINE_int64(max_delay_ms, 100 + 256, "Delay the response by at most this time");

using namespace whisper;

http::BaseClientConnection* CreateConnection(net::Selector* selector,
                                             net::NetFactory* net_factory,
                                             net::PROTOCOL net_protocol) {
  return new http::SimpleClientConnection(selector, *net_factory, net_protocol);
}

int num_requests_to_send;
int num_requests_to_receive;
Closure* send_callback;

struct ReqData {
    ReqData(Closure* end_run, int n)
        : end_run_(end_run),
          done_(::google::protobuf::internal::NewCallback(this, &ReqData::DoneCallback)) {
        request_.set_x(n);
        request_.set_y(2 * n);
    }
    ~ReqData() {
    }
    void Send(rpc::TestService_Stub* stub) {
        stub->Multiply(&controller_, &request_, &reply_, done_);
    }
    void DoneCallback() {
        if (controller_.Failed()) {
            LOG_ERROR << " RPC Failed !" << controller_.ErrorText()
                     << "\n request: " << request_.DebugString();
        } else {
            DLOG_INFO << " RPC OK "
                      << "\n request: " << request_.DebugString();
            CHECK_EQ(reply_.z(), request_.x() * request_.y());
        }

        --num_requests_to_receive;
        if (num_requests_to_receive <= 0) {
            LOG_INFO << "======================= Closing the client";
            end_run_->Run();
        }
        delete this;
    }
private:
    ::Closure* end_run_;
    ::rpc::Controller controller_;
    ::rpc::TestReq request_;
    ::rpc::TestReply reply_;
    ::google::protobuf::Closure* done_;
};

void SendRequest(net::Selector* selector,
                 rpc::TestService_Stub* stub,
                 ::Closure* end_run) {
    ReqData* data = new ReqData(end_run, num_requests_to_send);
    data->Send(stub);
    --num_requests_to_send;
    if (num_requests_to_send > 0) {
        if (FLAGS_max_delay_ms < FLAGS_min_delay_ms) {
            send_callback->Run();
        } else {
            const int64 delay = FLAGS_min_delay_ms + random() % (FLAGS_max_delay_ms -
                                                                 FLAGS_min_delay_ms);
            selector->RegisterAlarm(send_callback, delay);
        }
    }
}

void EndRun(net::Selector* selector,
            rpc::HttpClient* client,
            http::FailSafeClient* fsc) {
    selector->DeleteInSelectLoop(fsc);
    client->StartClose();
    selector->MakeLoopExit();
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  vector<net::HostPort> servers;
  vector<string> server_names;
  if ( FLAGS_servers.empty() ) {
    LOG_FATAL << " No --servers specified";
  }
  strutil::SplitString(FLAGS_servers, ",", &server_names);
  for ( int i = 0; i < server_names.size(); ++i  ) {
    servers.push_back(net::HostPort(server_names[i]));
  }

  net::Selector selector;
  net::NetFactory net_factory(&selector);
  http::ClientParams params;
  http::FailSafeClient* fsc = new http::FailSafeClient(&selector, &params, servers,
          NewPermanentCallback(&CreateConnection, &selector, &net_factory, net::PROTOCOL_TCP),
          true, 4, 400000, 5000, "");

  num_requests_to_send = FLAGS_num_requests;
  num_requests_to_receive = FLAGS_num_requests;
  rpc::HttpClient* client = new rpc::HttpClient(fsc, "/baserpc/test/rpc.TestService");
  rpc::TestService_Stub stub(client, ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL);

  ::Closure* end_run = ::NewCallback(&EndRun, &selector, client, fsc);
  send_callback = ::NewPermanentCallback(&SendRequest, &selector, &stub, end_run);

  selector.RunInSelectLoop(send_callback);

  selector.Loop();

  delete send_callback;
  LOG_INFO << "DONE";
}
