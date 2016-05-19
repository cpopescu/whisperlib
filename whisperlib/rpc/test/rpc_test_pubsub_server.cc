/** Copyright (c) 2014, Urban Engines inc.
 * All rights reserved.
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
#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/rpc/rpc_http_server.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/rpc/server_base.h"
#include "whisperlib/rpc/test/rpc_test_proto.pb.h"

DEFINE_int32(port, 8222, "Serve on this port");
DEFINE_int32(num_threads, 4, "Serve with these may worker threads");
DEFINE_bool(add_publish_fluff, false, "Add extra fluffy data in the reply for publish");

//
// Simple server for messages published by clients, redistributed on streams by us.
//
using namespace whisper;

class TestStreamServiceImpl : public rpc::TestStreamService {
    struct StreamData {
        StreamData(size_t next_seq, size_t last_seq, rpc::Controller* controller,
                   ::google::protobuf::Closure* closure)
            : next_seq_(next_seq), last_seq_(last_seq),
              controller_(controller), closure_(closure), proc_closure_(NULL) {
        }
        ~StreamData() {
            controller_->set_server_streaming_callback(NULL);
            controller_->PushStreamedMessage(NULL);
            delete proc_closure_;
        }
        size_t next_seq_;
        size_t last_seq_;
        rpc::Controller* controller_;
        ::google::protobuf::Closure* closure_;
        Closure* proc_closure_;
        bool needs_next_;
    };

public:
    TestStreamServiceImpl() : data_mutex_(true) {
    }
    virtual ~TestStreamServiceImpl() {
        LOG_INFO << " ------ Deleting the server";
        for (int i = 0; i < data_.size(); ++i) {
            delete data_[i];
        }
    }
    virtual void Publish(::google::protobuf::RpcController* controller,
                         const rpc::TestPubReq* request,
                         rpc::TestPubReply* response,
                         ::google::protobuf::Closure* done) {
        {
            synch::MutexLocker l(&data_mutex_);
            response->set_seq(data_.size());
            if (FLAGS_add_publish_fluff) {
                for (size_t i = 0; i < data_.size(); ++i) {
                    response->add_fluff(data_[i]->DebugString());
                }
            }
            LOG_INFO << "ID: " << data_.size()
                     << " Data published: " << request->data().ShortDebugString();
            data_.push_back(new rpc::TestPubData(request->data()));
            vector<StreamData*> to_update;
            for (hash_map<StreamData*, bool>::const_iterator it = streams_.begin();
                 it != streams_.end(); ++it) {
                if (it->second) {
                    to_update.push_back(it->first);
                }
            }
            for (size_t i = 0; i < to_update.size(); ++i) {
                StreamNext(to_update[i]);
            }
        }
        done->Run();
    }
    void Subscribe(::google::protobuf::RpcController* rpc_controller,
                   const rpc::TestSubReq* request,
                   rpc::TestSubReply* response,
                   ::google::protobuf::Closure* done) {
        rpc::Controller* const controller = static_cast<rpc::Controller*>(rpc_controller);
        if (!controller->is_streaming()) {
            {
                synch::MutexLocker l(&data_mutex_);
                if (request->seq() >= 0 && request->seq() < data_.size()) {
                    response->set_seq(request->seq());
                    response->mutable_data()->CopyFrom(*data_[request->seq()]);
                } else {
                    controller->SetFailed("Invalid sequence");
                }
            }
            done->Run();
        } else {
            StreamData* data = new StreamData(std::max(int64(request->seq()), int64(0)),
                                              request->end_seq(), controller, done);
            data->proc_closure_ = ::NewPermanentCallback(
                this, &TestStreamServiceImpl::StreamNext, data);
            controller->set_server_streaming_callback(data->proc_closure_);
            StreamNext(data);
        }
    }

private:
    void EndStream(StreamData* data) {
        synch::MutexLocker l(&data_mutex_);
        streams_.erase(data);
        delete data;
    }
    void StreamNext(StreamData* data) {
        ::google::protobuf::Closure* done = NULL;
        // First check if error was encountered. If yes, this is our chance to cleanup.
        if (data->controller_->Failed()) {
            EndStream(data);
        } else {
            synch::MutexLocker l(&data_mutex_);
            if (data->next_seq_ < data_.size()) {
                rpc::TestSubReply* resp = new rpc::TestSubReply();
                resp->mutable_data()->CopyFrom(*data_[data->next_seq_]);
                resp->set_seq(data->next_seq_);
                data->next_seq_++;
                data->controller_->PushStreamedMessage(resp);
                done = data->closure_;         // before ending
            }
            streams_[data] = data->next_seq_ >= data_.size();   // need more data
            if (data->last_seq_ && data->next_seq_ >= data->last_seq_) {
                done = data->closure_;         // before ending
                EndStream(data);
            }
        }
        if (done) {
            done->Run();
        }
    }

    synch::Mutex data_mutex_;
    vector<rpc::TestPubData*> data_;
    hash_map<StreamData*, bool> streams_;
};

////////////////////////////////////////////////////////////////////////////////


class PubSubServer : public rpc::ServerBase {
public:
    PubSubServer(int &argc, char **&argv)
        : rpc::ServerBase(argc, argv, "PubSubServer"),
          rpc_server_(NULL),
          service_() {
        set_net_params(FLAGS_num_threads, FLAGS_port);
        // http_params_.dlog_level_ = true;
    }

    ~PubSubServer() {
        CHECK_NULL(rpc_server_);
        CHECK_NULL(service_);
    }

protected:
    int Initialize() {
        rpc::ServerBase::Initialize();
        rpc_server_ = new rpc::HttpServer(http_server_, NULL, "/rpc", true, 1000, "");
        service_ = new TestStreamServiceImpl();
        CHECK(rpc_server_->RegisterService("", service_));
        return 0;
    }

    virtual void StopInSelectThread() {
        CHECK(rpc_server_->UnregisterService("", service_));
        rpc::ServerBase::StopInSelectThread();
    }
    virtual void StopServicesInSelectThread() {
        // Delete these after all the registered functions
        selector_->DeleteInSelectLoop(service_); service_ = NULL;
        selector_->DeleteInSelectLoop(rpc_server_); rpc_server_ = NULL;
    }

    virtual void Shutdown() {
        ServerBase::Shutdown();
    }

private:
    rpc::HttpServer* rpc_server_;
    TestStreamServiceImpl* service_;

    DISALLOW_EVIL_CONSTRUCTORS(PubSubServer);
};

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    PubSubServer app(argc, argv);
    app.Main();
    LOG_INFO << "DONE";
}
