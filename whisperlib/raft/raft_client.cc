/**
 * Copyright (c) 2014, Urban Engines inc.
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

#include "whisperlib/raft/raft_client.h"

namespace whisper {
namespace raft {

Client::Client(net::Selector* selector,
               const std::vector<std::string>& replicas,
               const std::string& http_path)
    : selector_(selector),
      replicas_(replicas),
      http_path_(http_path),
      num_retries_(2),
      request_timeout_ms_(40000),
      reopen_connection_interval_ms_(500),
      next_index_(0),
      client_net_(0),
      client_(0),
      stub_(0), close_count_(0), deleting_(false), restarting_(false) {
}
Client::~Client() {
    deleting_ = true;
    Close();
}

void Client::Close() {
    if (stub_) {
        selector_->DeleteInSelectLoop(stub_);
        stub_ = NULL;

        CHECK_NOT_NULL(client_);
        client_->StartClose();
        client_ = NULL;

        CHECK_NOT_NULL(client_net_);
        selector_->DeleteInSelectLoop(client_net_);
        client_net_ = NULL;
    }
    if (deleting_) {
        CHECK(active_requests_.empty());
    } else {
        selector_->RunInSelectLoop(whisper::NewCallback(this, &Client::IncrementCloseCount));
    }
}

void Client::IncrementCloseCount() {
    ++close_count_;   // increment after the deletions to make sure the requests
                      // started in between will get the old close_count_
}

void Client::InitializeNetwork() {
    CHECK(client_net_ == NULL);
    if (leader_name_.empty()) {
        crt_server_ = replicas_[next_index_];
        next_index_ = (next_index_ + 1) % replicas_.size();
    } else {
        crt_server_ = leader_name_;
    }

    client_net_ = new rpc::ClientNet(selector_, &http_params_,
                                     rpc::ClientNet::ToServerVec(crt_server_),
                                     NULL, num_retries_, request_timeout_ms_,
                                     reopen_connection_interval_ms_);
    client_ = new rpc::HttpClient(client_net_->fsc(), http_path_ + "/raft.pb.Raft");
    stub_ = new raft::pb::Raft_Stub(client_, ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL);
}

void Client::SendData(const std::string& data, Callback1<bool>* commit_callback) {
    selector_->RunInSelectLoop(whisper::NewCallback(this, &Client::StartRequest,
                                                    new RequestData(data, commit_callback)));
}

void Client::StartRequest(RequestData* data) {
    if (active_requests_.find(data) == active_requests_.end()) {
        active_requests_.insert(data);   // may be a reinsertion on retries
    }
    data->close_count_ = close_count_;
    if (stub_ == NULL) {
        InitializeNetwork();
    }
    stub_->Save(&data->controller_, &data->req_, &data->resp_,
                ::google::protobuf::internal::NewCallback(this, &Client::ProcessResponse, data));
}

void Client::CompleteData(RequestData* data, bool success) {
    // This means the data was written (possibly not committed).
    if (data->commit_callback_) {
        data->commit_callback_->Run(success);
    }
    CHECK(active_requests_.erase(data));
    delete data;
}

void Client::ProcessResponse(RequestData* data) {
    if (!data->controller_.Failed()) {
        if (data->resp_.has_pos()) {
            CompleteData(data, true);
            return;   // we are done for good
        }
        // Data not written for some reason. We may have a leader or so.
        if (data->resp_.has_leader_name()) {
            leader_name_ = data->resp_.leader_name();
        } else {
            leader_name_.clear();
        }
        LOG_INFO << " Raft client failed - may retry.. ";
    } else {
        leader_name_.clear();
        LOG_ERROR << "Raft client failed with: " << data->controller_.ErrorText()
                  << " at current server: " << crt_server_;
    }
    if (deleting_) {
        // This means we are a failure anyway - no time to complete, and we are
        // done with this guy.
        CompleteData(data, false);
    } else  if (close_count_ == data->close_count_ && !restarting_) {
        restarting_ = true;
        // We are the first guys to notice error, close and restart all.
        selector_->RunInSelectLoop(whisper::NewCallback(this, &Client::CloseAndRestart));
    } //  else -> others will be retried by the RestartActive scheduled above
}
void Client::CloseAndRestart() {
    Close();
    // Running in select loop will make sure the restart happens after
    // the closing
    selector_->RunInSelectLoop(whisper::NewCallback(this, &Client::RestartActive));
}

void Client::RestartActive() {
    restarting_ = false;
    leader_name_.clear();
    for (hash_set<RequestData*>::const_iterator it = active_requests_.begin();
         it != active_requests_.end(); ++it) {
        // This means they were not just started in between:
        if (close_count_ > (*it)->close_count_) {
            // we should have had these completed per Close -> failure
            // -- it may also be not even started..
            // CHECK((*it)->controller_.IsFinalized());
            (*it)->PrepareForRetry();
            StartRequest(*it);
        }
    }
}

}  // namespace raft
}  // namespace whisper
