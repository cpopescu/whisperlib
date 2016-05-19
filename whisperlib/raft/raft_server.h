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

#ifndef __WHISPERLIB_RAFT_RAFT_SERVER_H__
#define __WHISPERLIB_RAFT_RAFT_SERVER_H__

#include <map>
#include <string>
#include <vector>

#include "whisperlib/raft/RaftProto.pb.h"
#include "whisperlib/base/types.h"
#include "whisperlib/net/selector.h"
#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/http/http_client_protocol.h"
#include "whisperlib/rpc/rpc_http_server.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/io/logio/logio.h"
#include "whisperlib/sync/mutex.h"

namespace whisper {
namespace raft {

struct Node;

class Server : public raft::pb::Raft {
    enum State {
        RAFT_STATE_NONE,
        RAFT_STATE_FOLLOWER,
        RAFT_STATE_CANDIDATE,
        RAFT_STATE_LEADER
    };
public:
    Server(rpc::HttpServer* http_server,
           const std::string& http_path,
           io::LogWriter* log_writer,     // we own this from now on
           whisper::Closure* commit_closure);    // we do not own this
    ~Server();

    bool Initialize(size_t node_id,
                    const std::vector<std::string>& nodes);

    /** @return my node id (between 0 and num_nodes - 1).
     */
    size_t node_id() const {
        return node_id_;
    }
    /** @return the number of nodes in the system
     */
    size_t num_nodes() const {
        return nodes_.size();
    }
    /** @param node The node's index
     * @return node pointed to by node index.
     */
    const Node* node(size_t node) const {
        CHECK_LT(node, nodes_.size());
        return nodes_[node];
    }
    /** @return node ID of who I voted for
     */
    size_t voted_for() const {
        return voted_for_;
    }

    /** @return the timeout for an election, in ms
     */
    int64 election_timeout_ms() const {
        return election_timeout_ms_;
    }
    void set_election_timeout_ms(int64 val) {
        election_timeout_ms_ = val;
    }

    /** @return the timeout for a regular request,in ms
     */
    int64 request_timeout_ms() const {
        return client_params_.default_request_timeout_ms_;
    }
    void set_request_timeout_ms(int64 val) {
        client_params_.default_request_timeout_ms_ = val;
    }

    /**
     * @return current term */
    int64 current_term() const {
        synch::MutexLocker l(&mutex_);
        return current_term_;
    }
    /** @return number of items within log
     */
    io::LogPos log_pos() const {
        synch::MutexLocker l(&mutex_);
        return log_writer_->Tell();
    }

    /** @return current commit log index
     */
    io::LogPos commit_pos() const {
        synch::MutexLocker l(&mutex_);
        return commit_pos_;
    }

    /** @return true iff follower */
    bool is_follower() const {
        return state_ == RAFT_STATE_FOLLOWER;
    }

    /** @return true if leader */
    bool is_leader() const {
        return state_ == RAFT_STATE_LEADER;
    }

    /** @return true iff candidate */
    bool is_candidate() const {
        return state_ == RAFT_STATE_CANDIDATE;
    }
    /** @return how many bytes we send in an AppendEntries */
    size_t max_entries_size() const {
        synch::MutexLocker l(&mutex_);
        return max_entries_size_;
    }
    /** sets max entries size we send in an AppendEntries */
    void set_max_entries_size(size_t val) {
        synch::MutexLocker l(&mutex_);
        max_entries_size_ = val;
    }

    //////////////////////////////////////// RPC interface

    void Vote(::google::protobuf::RpcController* controller,
              const raft::pb::RequestVote* request,
              raft::pb::RequestVoteResponse* response,
              ::google::protobuf::Closure* done);

    void Append(::google::protobuf::RpcController* controller,
                const raft::pb::AppendEntries* request,
                raft::pb::AppendEntriesResponse* response,
                ::google::protobuf::Closure* done);

    void Save(::google::protobuf::RpcController* controller,
              const raft::pb::Data* request,
              raft::pb::DataResponse* response,
              ::google::protobuf::Closure* done);

    std::string StatusString(bool include_nodes) const;

protected:
    /** Runs periodically to identify election timeouts / heartbeats */
    void PeriodicCheck();

    void BecomeCandidateLocked();
    void BecomeLeaderLocked();
    void BecomeFollowerLocked(int32 leader_id);

    void SendRequestVoteLocked();

    void SendHeartbeatToFollowersLocked();

    void SendAppendLocked(Node* node, const raft::pb::AppendEntries* req);
    void SendRequestVoteLocked(Node* node, const raft::pb::RequestVote* req);

    void SendAppendEntriesToNodeLocked(Node* node, bool is_entry_filled, size_t max_size);
    void FillAppendEntriesLocked(raft::pb::AppendEntries* req, const Node* node) const;

    void SetElectionElapseTimeout();


    struct AppendEntriesData {
        Node* node_;
        rpc::Controller controller_;
        const raft::pb::AppendEntries* req_;
        raft::pb::AppendEntriesResponse resp_;
        bool detached_;
        AppendEntriesData(Node* node, const raft::pb::AppendEntries* req)
            : node_(node), req_(req), detached_(false) {
        }
        ~AppendEntriesData() {
            delete req_;  // we do not own node_;
        }
    };
    void ProcessAppendEntriesResponse(AppendEntriesData* data);

    struct RequestVoteData {
        Node* node_;
        rpc::Controller controller_;
        const raft::pb::RequestVote* req_;
        raft::pb::RequestVoteResponse resp_;
        bool detached_;
        RequestVoteData(Node* node, const raft::pb::RequestVote* req)
            : node_(node), req_(req), detached_(false) {
        }
    };
    void ProcessRequestVoteResponse(RequestVoteData* data);

    void UpdateCurrentTermLocked(int64 term, int32 leader_id);
    void DegradeNodeLocked(AppendEntriesData* data);
    bool MaybeTrucateLogAfterLocked(int64 prev_term, const io::LogPos& prev_pos);
    bool AppendRequestEntriesLocked(const raft::pb::AppendEntries* request);
    bool MaybeAdvanceCommitLocked();

    std::string GetStateFilename() const;
    void SaveStateLocked(const char* caller) const;
    bool LoadStateLocked();
    std::string StatusStringLocked(bool include_nodes) const;

    void AdvanceWaitersLocked();
    void ClearWaitersLocked();

    State state() const {
        return state_;
    }

    void set_state(State val) {
        state_ = val;
    }

    mutable synch::Mutex mutex_;

    /** Exposes our service to clients and other nodes */
    rpc::HttpServer* const http_server_;
    /** Shortcut to http_server_->http_serve()->selector(). All operations
     * happen in this selector loop - we synchronize on that thread.
     */
    net::Selector* const selector_;
    /** parameter of client http connections to other nodes. */
    http::ClientParams client_params_;
    /** path of rpc serving under http_server_ */
    const std::string http_path_;
    /** our name (normally matches log_writer_->file_base()) */
    const std::string name_;

    /** runs every so often 1/4 of election_timeout_ms_ */
    Closure* heartbeat_alarm_;

    /** the local copy of the log which is replicated */
    io::LogWriter* log_writer_;

    /** We call this function each time we update the commit position */
    whisper::Closure* commit_closure_;

    /***************************************** Configuration params: */

    /** my node ID */
    size_t node_id_;
    /** The server / nodes we are talking to (including ourselves). */
    std::vector<Node*> nodes_;

    /***************************************** Persistent state: */

    /** the server's best guess of what the current term is
     * starts at zero */
    int64 current_term_;

    /** The candidate the server voted for in its current term,
     * or -1 if it hasn't voted for any.  */
    int32 voted_for_;

    /** pos of candidate's last log entry applied */
    io::LogPos last_log_pos_;

    /** term of the last log entry applied */
    int64 last_log_term_;

    /** The last known leader */
    int32 leader_id_;

    /***************************************** Volatile state: */

    /* follower/leader/candidate indicator */
    State state_;

    /* pos of highest log entry known to be committed - this is less or equal to
     * last_log_pos_ */
    io::LogPos commit_pos_;

    /** When the new election should be started (we go into candidate) */
    int64 election_timeout_elapsed_ns_;

    /** When we should send a new hartbeat to followers */
    int64 heartbeat_elapsed_ns_;

    /** Timeout for a new election to start */
    int64 election_timeout_ms_;

    /** Requests waiting for commit to reach a log position */
    typedef std::map<io::LogPos, std::pair<raft::pb::DataResponse*,
                                 ::google::protobuf::Closure*> > WaitersMap;
    WaitersMap commit_waiters_;

    /** How many candidates votes are out ? */
    int32 candidate_votes_out_;

    /** How many bytes of entries we send in a append entries block */
    size_t max_entries_size_;

    /** Pending AppendEntriesData requests */
    std::set<AppendEntriesData*> pending_appends_;

    /** Pending AppendEntriesData requests */
    std::set<RequestVoteData*> pending_votes_;

    DISALLOW_EVIL_CONSTRUCTORS(Server);
};

}   // namepspace raft
}   // namespace whisper

#endif   //  __WHISPERLIB_RAFT_SERVER_H__
