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

#ifndef __WHISPERLIB_RAFT_RAFT_CLIENT_H__
#define __WHISPERLIB_RAFT_RAFT_CLIENT_H__

#include "whisperlib/raft/RaftProto.pb.h"
#include "whisperlib/base/types.h"
#include "whisperlib/net/selector.h"
#include "whisperlib/http/http_client_protocol.h"
#include "whisperlib/rpc/rpc_http_client.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/rpc/client_net.h"
#include "whisperlib/sync/producer_consumer_queue.h"
#include "whisperlib/base/hash.h"
#include WHISPER_HASH_SET_HEADER
#include <vector>


namespace whisper {
namespace raft {

class Client {
public:
    Client(net::Selector* selector,
           const std::vector<std::string>& replicas,
           const std::string& http_path);
    // Destructor - make sure to delete this one in the selector_ loop.
    ~Client();

    /** Use this to tune your connection parameters */
    http::ClientParams* mutable_params() {
        return &http_params_;
    }

    /** Sends some data to the server. If commit_callback is not null,
     * it will wait for data to be committed.
     */
    void SendData(const std::string& data, Callback1<bool>* commit_callback);

    int num_retries() const { return num_retries_; }
    void set_num_retries(int val) { num_retries_ = val; }
    int32 request_timeout_ms() const { return request_timeout_ms_; }
    void set_request_timeout_ms(int32 val) { request_timeout_ms_ = val; }
    int32 reopen_connection_interval_ms() const { return reopen_connection_interval_ms_; }
    void set_reopen_connection_interval_ms(int32 val) { reopen_connection_interval_ms_ = val; }

private:
    struct RequestData {
        rpc::Controller controller_;
        raft::pb::Data req_;
        raft::pb::DataResponse resp_;
        Callback1<bool>* commit_callback_;
        int64 close_count_;
        size_t retries_;
        RequestData(const std::string& data, Callback1<bool>* commit_callback)
            : commit_callback_(commit_callback), close_count_(0), retries_(0) {
            req_.set_data(data);
            req_.set_wait_to_commit(commit_callback != NULL);
        }
        void PrepareForRetry() {
            ++retries_;
            controller_.Reset();
            resp_.Clear();
        }
    };
    void StartRequest(RequestData* req);
    void ProcessResponse(RequestData* req);

    void InitializeNetwork();
    void Close();

    void RestartActive();
    void CloseAndRestart();
    void IncrementCloseCount();
    void CompleteData(RequestData* data, bool success);

    /** Does networking for us - we do not own it */
    net::Selector* const selector_;
    /** Set of replicas for the log to send data to */
    const std::vector<std::string> replicas_;
    /** Where the server is serving (w/o the service identification */
    const std::string http_path_;
    /** Determines the parameters for connection (may want to tune the timeouts) */
    http::ClientParams http_params_;

    /** How many times to retry before failing a connection */
    int num_retries_;
    /** Total request timeout in milliseconds */
    int32 request_timeout_ms_;
    /** When to reopen a connection after a failure */
    int32 reopen_connection_interval_ms_;

    /** Confirmed leader name */
    std::string leader_name_;
    /** The next server to try when no leader is known */
    size_t next_index_;
    /** To which server we are currently connected */
    std::string crt_server_;
    /** Wraps network connections and such */
    rpc::ClientNet* client_net_;
    /** Maintains net conversations */
    rpc::HttpClient* client_;
    /** RPC interface persona  */
    raft::pb::Raft_Stub* stub_;

    /** When closing connections we increment this */
    int64 close_count_;
    /** When deleting the client we set this, so we do not retry */
    bool deleting_;
    /** While restarting the connection this is on */
    bool restarting_;
    /** Active requests */
    hash_set<RequestData*> active_requests_;
};

}  // namespace  raft
}  // namespace whisper

#endif
