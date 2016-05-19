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

#include "whisperlib/raft/raft_server.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/io/buffer/protobuf_stream.h"
#include "whisperlib/io/ioutil.h"
#include "whisperlib/io/file/file_input_stream.h"
#include "whisperlib/io/file/file_output_stream.h"
#include "whisperlib/rpc/client_net.h"
#include "whisperlib/rpc/rpc_http_client.h"

using namespace std;

#define LOG_RAFT \
    LOG_INFO << name_ << " (#" << node_id_ << ") : "

#ifdef NDEBUG
#define LOG_RAFT_DEBUG if (false) LOG_INFO
#else
#define LOG_RAFT_DEBUG LOG_RAFT
#endif

namespace whisper {
namespace raft {

////////////////////////////////////////////////////////////////////////////////

void PosToProto(const io::LogPos& l, raft::pb::LogPos* p) {
    if (l.IsNull()) {
        p->set_is_null(true);
    } else {
        if (l.file_num_) p->set_file_num(l.file_num_);
        if (l.block_num_) p->set_block_num(l.block_num_);
        if (l.record_num_) p->set_record_num(l.record_num_);
    }
}

void PosFromProto(const raft::pb::LogPos& p, io::LogPos* l) {
    if (p.is_null()) {
        *l = io::LogPos();
    } else {
        l->file_num_ = p.file_num();
        l->block_num_ = p.block_num();
        l->record_num_ = p.record_num();
    }
}

io::LogPos PosFromProto(const raft::pb::LogPos& p) {
    if (p.is_null()) {
        return io::LogPos();
    }
    return io::LogPos(p.file_num(), p.block_num(), p.record_num());
}

////////////////////////////////////////////////////////////////////////////////

/**
 * The server keeps one structure for each server in the game.
 */
struct Node {
    ////////// Networking related members
    net::Selector* const selector_;
    rpc::ClientNet* client_net_;
    rpc::HttpClient* client_;
    raft::pb::Raft_Stub* stub_;

    ////////// Node identification

    /** Node name - the server:port identification */
    const std::string name_;
    /** The node id - index in the node array */
    const size_t node_id_;
    /** Did this guy voted for me in the last election round ? */
    bool votes_for_me_;

    ////////// Log positioning

    /** Where the next entry to send to this node is coming from */
    io::LogPos next_log_pos_;

    /** The position of the entry before next_log_pos_ */
    io::LogPos last_log_pos_;
    /** The term of the entry before last_log_term_ */
    int64 last_log_term_;

    /** To where the log to this guy is synced */
    io::LogPos match_log_pos_;

    ////////// Log reading

    /** Log to be read for this node */
    io::LogReader* log_reader_;

    /** The last entry that was read from the log */
    pb::DataEntry entry_;

    /** Used for reading data from the buffer */
    io::MemoryStream buffer_;

    bool in_transfer_;


    Node(net::Selector* selector,  // we just use it (no owning)
         const std::string& name, size_t node_id,
         // we take control of the reader:
         io::LogReader* log_reader)
        : selector_(selector), client_net_(NULL), client_(NULL), stub_(NULL),
          name_(name), node_id_(node_id), votes_for_me_(false), last_log_term_(0),
          log_reader_(log_reader), in_transfer_(false) {
    }
    ~Node() {
        if (stub_) {
            selector_->DeleteInSelectLoop(stub_);
            client_->StartClose();
            selector_->DeleteInSelectLoop(client_net_);
        }
        delete log_reader_;
    }

    void InitRpc(const http::ClientParams* params, const std::string& path) {
        client_net_ = new rpc::ClientNet(selector_, params,
                                         rpc::ClientNet::ToServerVec(name_),
                                         NULL,
                                         3, /* retries */
                                         2000, /* timeout */
                                         200 /* reopen */);
        client_ = new rpc::HttpClient(client_net_->fsc(), path);
        stub_ = new raft::pb::Raft_Stub(
            client_, ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL);
    }

    bool ReadCurrent() {
        buffer_.Clear();
        if (!log_reader_->GetNextRecord(&buffer_)) {
            return false;
        }
        entry_.Clear();
        if (!io::ParseProto(&entry_, &buffer_)) {
            LOG_RAFT << "Log file corrupted at position: "
                     << log_reader_->TellAtBlock().ToString();
            return false;
        }
        return true;
    }
    void PullNext() {
        CHECK(log_reader_->Seek(next_log_pos_))
            << " for " << name_ << " crt_node " << node_id_ << " pos: "
            << next_log_pos_.ToString();
        CHECK(ReadCurrent())
            << " for " << name_ << " crt_node " << node_id_ << " pos: "
            << next_log_pos_.ToString();

    }
    void SetPositionsFromEntry() {
        next_log_pos_ = log_reader_->TellAtBlock();
        last_log_pos_ = PosFromProto(entry_.pos());
        last_log_term_ = entry_.term();
    }
    std::string ToString() const {
        return strutil::StringPrintf(
            "  Node #%zd %s [%s] last_term: %" PRId64 " votes_for_me: %d "
            "\n      next_pos:  %s"
            "\n      last_pos:  %s"
            "\n      match_pos: %s",
            node_id_, in_transfer_ ? "TRANS" : "",
            name_.c_str(), last_log_term_, int(votes_for_me_),
            next_log_pos_.ToString().c_str(),
            last_log_pos_.ToString().c_str(),
            match_log_pos_.ToString().c_str());
    }
};


////////////////////////////////////////////////////////////////////////////////

Server::Server(rpc::HttpServer* http_server,
               const string& path,
               io::LogWriter* log_writer,
               whisper::Closure* commit_closure)
    : mutex_(true),
      http_server_(http_server),
      selector_(http_server->http_server()->selector()),
      http_path_(path),
      name_(log_writer->file_base()),
      heartbeat_alarm_(nullptr),
      log_writer_(log_writer),
      commit_closure_(commit_closure),
      node_id_(0),
      current_term_(0),
      voted_for_(-1),
      last_log_term_(0),
      leader_id_(-1),
      state_(RAFT_STATE_FOLLOWER),
      election_timeout_elapsed_ns_(0),
      heartbeat_elapsed_ns_(0),
      election_timeout_ms_(2000),
      max_entries_size_(1 << 20) {
    CHECK(commit_closure == nullptr || commit_closure->is_permanent());
}

Server::~Server() {
    http_server_->UnregisterService(http_path_, this);
    synch::MutexLocker l(&mutex_);
    for (auto& pending : pending_appends_) {
        pending->detached_ = true;
    }
    for (auto& pending : pending_votes_) {
        pending->detached_ = true;
    }
    ClearWaitersLocked();  // TODO(cp) - we may not need to do anything - per http closing
    if (heartbeat_alarm_) {
        selector_->UnregisterAlarm(heartbeat_alarm_);
        delete heartbeat_alarm_;
    }
    delete log_writer_;
    for (size_t i = 0; i < nodes_.size(); ++i) {
        delete nodes_[i];
    }
    nodes_.clear();
}

void Server::PeriodicCheck() {
    const int64 now = timer::TicksNsec();
    {
        synch::MutexLocker l(&mutex_);
        if (is_leader()) {
            if (heartbeat_elapsed_ns_ < now) {
                SendHeartbeatToFollowersLocked();
                heartbeat_elapsed_ns_ = now +  election_timeout_ms_ * 1e6 * .2;
            }
        } else if (election_timeout_elapsed_ns_ < now) {
            BecomeCandidateLocked();
        }
    }
    if (is_candidate()) {
        selector_->RegisterAlarm(heartbeat_alarm_,
                                 election_timeout_ms_ * .2 +
                                 (now % (int(election_timeout_ms_ * .05))));  // randomize
    } else {
        selector_->RegisterAlarm(heartbeat_alarm_, election_timeout_ms_ * .25);
    }
}

string Server::StatusString(bool include_nodes) const {
    synch::MutexLocker l(&mutex_);
    return StatusStringLocked(include_nodes);
}

string Server::StatusStringLocked(bool include_nodes) const {
    string s = strutil::StringPrintf(" Server Id: %zd [ state: %d"
                                     " / leader_id: %d / voted_for: %d"
                                     " / term: %" PRId64
                                     " / last term: %" PRId64 "]"
                                     "\n      commit_pos: %s"
                                     "\n      last pos:   %s"
                                     "\n      log_pos:    %s\n",
                                     node_id_, int(state_), int(leader_id_),
                                     int(voted_for_),
                                     current_term_,
                                     last_log_term_,
                                     commit_pos_.ToString().c_str(),
                                     last_log_pos_.ToString().c_str(),
                                     log_writer_->Tell().ToString().c_str());
    if (include_nodes) {
        for (size_t i = 0; i < nodes_.size(); ++i) {
            s += nodes_[i]->ToString();
            s += "\n";
        }
    }
    return s;
}

////////////////////////////////////////////////////////////////////////////////
//
//

string Server::GetStateFilename() const {
    return strutil::JoinPaths(log_writer_->log_dir(),
                              string("_raft_state_") + log_writer_->file_base());
}

bool Server::LoadStateLocked() {
    const string state_file(GetStateFilename());
    string contents;
    if (!io::FileInputStream::TryReadFile(state_file, &contents)) {
        io::LogPos file_pos = log_writer_->Tell();
        if (file_pos.block_num_ > 0) {
            LOG_WARN << " Cannot read state for log: " << log_writer_->file_name()
                      << " but log pos is: " << file_pos.ToString()
                      << " - something is really fishy.";
            return false;
        }
        return true;
    }
    raft::pb::RaftState state;
    if (!state.ParseFromString(contents)) {
        LOG_WARN << " Cannot parse raft state from state file - bad."
                  << state_file;
        return false;
    }
    current_term_ = state.current_term();
    if (state.has_voted_for()) {
        voted_for_ = state.voted_for();
    } else {
        voted_for_ = -1;
    }
    if (state.has_last_log_pos()) {
        PosFromProto(state.last_log_pos(), &last_log_pos_);
        last_log_term_ = state.last_log_term();
    } else {
        last_log_pos_ = io::LogPos();
        last_log_term_ = 0;
    }
    if (state.has_commit_pos()) {
        PosFromProto(state.commit_pos(), &commit_pos_);
    } else {
        commit_pos_ = io::LogPos();
    }
    LOG_RAFT << " State read: " << state.ShortDebugString();

    return true;
}

// TODO(cp): write / read data with checkpoints ?
void Server::SaveStateLocked(const char* caller) const {
    const string state_file(GetStateFilename());
    const string state_file_tmp(state_file + "_tmp");

    raft::pb::RaftState state;
    state.set_current_term(current_term_);
    if (voted_for_ >= 0) {
        state.set_voted_for(voted_for_);
    }
    PosToProto(last_log_pos_, state.mutable_last_log_pos());
    state.set_last_log_term(last_log_term_);
    PosToProto(commit_pos_, state.mutable_commit_pos());

    string val;
    CHECK(state.SerializeToString(&val));
    io::FileOutputStream::WriteFileOrDie(state_file_tmp.c_str(), val);
    if (::rename(state_file_tmp.c_str(), state_file.c_str())) {
        // Note - this is too slow - does a bunch of checks we don't need
        // CHECK(io::Rename(state_file_tmp, state_file, true));
        LOG_FATAL << "Cannot rename state file: " << state_file_tmp << " -> " << state_file;
    }

    // LOG_RAFT << " State written: " << caller << " => " << state.ShortDebugString();
}

////////////////////////////////////////////////////////////////////////////////
//
// Initialization after construction - do not continue if this returns false
//

bool Server::Initialize(size_t node_id, const std::vector<std::string>& nodes) {
    synch::MutexLocker l(&mutex_);

    CHECK(nodes_.empty());
    CHECK_LT(node_id, nodes.size());

    if (!LoadStateLocked()) {
        return false;
    }

    node_id_ = node_id;
    nodes_.resize(nodes.size());

    http_server_->RegisterService(http_path_, this);
    const string full_path = strutil::JoinPaths(
        strutil::JoinPaths(http_server_->path(), http_path_, '/'),
        "raft.pb.Raft", '/');
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes_[i] = new Node(selector_, nodes[i], i, log_writer_->NewReader());
        if (i != node_id_) {
            nodes_[i]->InitRpc(&client_params_, full_path);
        }
    }

    SetElectionElapseTimeout();
    heartbeat_alarm_ = whisper::NewPermanentCallback(this, &Server::PeriodicCheck);
    selector_->RunInSelectLoop(
        whisper::NewCallback(selector_, &whisper::net::Selector::RegisterAlarm,
                      heartbeat_alarm_, int64(election_timeout_ms_ * .25) +
                      (timer::TicksNsec() % 100)));
    return true;
}

void Server::SetElectionElapseTimeout() {
    const int64 now = timer::TicksNsec();
    election_timeout_elapsed_ns_ = timer::TicksNsec() +
        election_timeout_ms_ * 1e6 * (1 + (now % 10) * .1);
}

////////////////////////////////////////////////////////////////////////////////
//
// State changing
//
void Server::BecomeCandidateLocked() {
    CHECK(!is_leader());

    set_state(RAFT_STATE_CANDIDATE);
    voted_for_ = node_id_;
    leader_id_ = -1;

    for (size_t i = 0; i < nodes_.size(); ++i) {
        nodes_[i]->votes_for_me_ = false;
    }
    nodes_[node_id_]->votes_for_me_ = true;

    ++current_term_;

    SetElectionElapseTimeout();
    SendRequestVoteLocked();
    SaveStateLocked("Become Candidate");

    LOG_RAFT << " Become candidate: " << StatusStringLocked(false);
}

void Server::BecomeLeaderLocked() {
    CHECK(is_candidate());

    set_state(RAFT_STATE_LEADER);
    voted_for_ = node_id_;
    leader_id_ = node_id_;

    CHECK(nodes_[node_id_]->log_reader_->Seek(last_log_pos_))
        << "Cannot seek at previous position: " << last_log_pos_.ToString();
    if (!last_log_pos_.IsNull()) {
        CHECK(nodes_[node_id_]->ReadCurrent());
        nodes_[node_id_]->next_log_pos_  = nodes_[node_id_]->log_reader_->TellAtBlock();
    } else {
        nodes_[node_id_]->next_log_pos_ = io::LogPos(0, 0, 0);
    }

    // Now we have last entry in entry_ for node_id_.
    nodes_[node_id_]->last_log_pos_  = last_log_pos_;
    nodes_[node_id_]->last_log_term_ = last_log_term_; // nodes_[node_id_]->entry_.term();
    nodes_[node_id_]->match_log_pos_ = last_log_pos_;

    Node* me = nodes_[node_id_];
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (i != node_id_) {
            nodes_[i]->next_log_pos_  = me->next_log_pos_;
            nodes_[i]->last_log_pos_  = me->last_log_pos_;
            nodes_[i]->last_log_term_ = me->last_log_term_;
            nodes_[i]->match_log_pos_ = io::LogPos();
        }
    }
    SaveStateLocked("Become Leader");
    LOG_RAFT << " Become leader: " << StatusStringLocked(false);

    SendHeartbeatToFollowersLocked();
}

void Server::BecomeFollowerLocked(int32 leader_id) {
    set_state(RAFT_STATE_FOLLOWER);
    voted_for_ = leader_id;
    leader_id_ = leader_id;

    SaveStateLocked("Become Follower");

    LOG_RAFT << " Become follower: " << StatusStringLocked(false);
}

void Server::UpdateCurrentTermLocked(int64 term, int32 leader_id) {
    current_term_ = term;
    if (!is_follower()) {
        BecomeFollowerLocked(leader_id);
    } else {
        SaveStateLocked("Update Current Term");
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Voting
//

/** Processes a Vote request from a peer */
void Server::Vote(::google::protobuf::RpcController* controller,
                  const raft::pb::RequestVote* request,
                  raft::pb::RequestVoteResponse* response,
                  ::google::protobuf::Closure* done) {
    {
        synch::MutexLocker l(&mutex_);
        if (size_t(request->candidate_id()) >= nodes_.size()) {
            response->set_vote_granted(false);
        } else if (current_term_ < request->term() &&
                   PosFromProto(request->last_log_pos()) >= commit_pos_) {
            response->set_vote_granted(true);
        } else if (request->term() < current_term_ ||
                   voted_for_ != -1 ||
                   PosFromProto(request->last_log_pos()) < last_log_pos_) {
            response->set_vote_granted(false);
        } else {
            response->set_vote_granted(true);
        }
        if (response->vote_granted()) {
            voted_for_ = request->candidate_id();
            current_term_ = request->term();
            SaveStateLocked("Voted");
        }
        response->set_term(current_term_);

        LOG_RAFT << " Vote request " <<  request->ShortDebugString()
                 << " our term: " << current_term_
                 << " vote granted: " << response->vote_granted();
    }

    done->Run();
}

void Server::ProcessRequestVoteResponse(RequestVoteData* data) {
    if (data->detached_) {   // No need to lock for these - they happen in selector thread
        delete data; return;
    }
    synch::MutexLocker l(&mutex_);
    if (data->controller_.Failed()) {
        LOG_WARN << "Raft RequestVote conversation with: " << data->node_->name_
                  << " failed: " << data->controller_.ErrorText();
    } else if (is_candidate()) {
        if (data->resp_.vote_granted()) {
            data->node_->votes_for_me_ = true;
            size_t num_votes_for_me = 0;
            for (size_t i = 0; i < nodes_.size(); ++i) {
                num_votes_for_me += (nodes_[i]->votes_for_me_ = true) ? 1 : 0;
            }
            if (num_votes_for_me >= (nodes_.size() / 2) + 1) {
                BecomeLeaderLocked();
            }
        } else if (current_term_ < data->req_->term()) {
            LOG_RAFT << " We are at an old term - probably lost: " << data->req_->term()
                     << " vs. " << current_term_;
            // TODO(cp): do something about me ??
        }
    } else {
        LOG_RAFT << " Vote from a different state answered at term: " << data->req_->term()
                 << " vs. " << current_term_ << " I am: " << state_;
    }
    --candidate_votes_out_;
    if (candidate_votes_out_ == 0 && is_candidate()) {
        BecomeFollowerLocked(-1);
    }
    pending_votes_.erase(data);
    delete data;
}

////////////////////////////////////////////////////////////////////////////////
//
// Append Entries processing
//

void Server::Append(::google::protobuf::RpcController* controller,
                    const raft::pb::AppendEntries* request,
                    raft::pb::AppendEntriesResponse* response,
                    ::google::protobuf::Closure* done) {
    bool commit_pos_updated = false;
    {
        synch::MutexLocker l(&mutex_);

        if (request->term() >= current_term_) {
            if (request->term() == current_term_) {
                if (is_candidate()) {
                    BecomeFollowerLocked(request->leader_id());
                } else if (leader_id_ != request->leader_id()) {
                    leader_id_ = request->leader_id();
                    SaveStateLocked("Append - leader change");
                }
            } else {
                UpdateCurrentTermLocked(request->term(), request->leader_id());
            }

            io::LogPos last_log_pos = request->has_last_log_pos() ?
                PosFromProto(request->last_log_pos()) : io::LogPos();
            bool need_save = false;
            if (request->entry_size() == 0) {
                if (last_log_pos_ == last_log_pos &&
                    last_log_term_ == request->last_log_term()) {
                    response->set_success(true);
                }
            } else {
                if (MaybeTrucateLogAfterLocked(request->last_log_term(), last_log_pos)) {
                    if (AppendRequestEntriesLocked(request)) {
                        response->set_success(true);
                        need_save = true;
                    }
                } else {
                    LOG_RAFT << " Could not truncate log for request:  "
                             << " last_log_pos: " << request->last_log_pos().ShortDebugString()
                             << " or: " << last_log_pos.ToString()
                             << " / term: " << request->term()
                             << " / leader_id: " << request->leader_id()
                             << " / last_log_term: " << request->last_log_term()
                             << " / leader_commit: "
                             << request->leader_commit_pos().ShortDebugString();

                }
            }

            io::LogPos new_commit_pos = PosFromProto(request->leader_commit_pos());
            if (response->success()) {
                if (new_commit_pos > commit_pos_ && new_commit_pos <= last_log_pos_) {
                    commit_pos_ = new_commit_pos;
                    commit_pos_updated = true;
                    need_save = true;
                }
                if (need_save) {
                    SaveStateLocked("Append - save done");
                }
            }

            response->set_term(current_term_);
            PosToProto(log_writer_->Tell(), response->mutable_current_pos());
            PosToProto(commit_pos_, response->mutable_commit_pos());
        }
        response->set_term(current_term_);
        if (!response->has_success()) {
            response->set_success(false);
        }
        SetElectionElapseTimeout();
    }
    if (commit_pos_updated && commit_closure_) {
        commit_closure_->Run();
    }

    done->Run();
}


void Server::ProcessAppendEntriesResponse(AppendEntriesData* data) {
    if (data->detached_) {   // No need to lock for these - they happen in selector thread
        delete data; return;
    }
    bool commit_pos_updated = false; {
    synch::MutexLocker l(&mutex_);
    data->node_->in_transfer_ = false;
    if (data->controller_.Failed()) {
        LOG_WARN << "Raft AppendEntries conversation with: " << data->node_->name_
                  << " failed: " << data->controller_.ErrorText();
    } else if (is_leader()) {
        // LOG_INFO << " ----> Reply: " << data->node_->ToString()
        //          << " ->> " << data->resp_.ShortDebugString();
        if (data->resp_.term() > current_term_) {
            UpdateCurrentTermLocked(data->resp_.term(), -1);
        } else if (!data->resp_.success()) {
            DegradeNodeLocked(data);
        } else if (data->req_->entry_size() > 0) {
            CHECK(data->node_->match_log_pos_ <= data->node_->next_log_pos_)
                << " For: " << data->node_->match_log_pos_.ToString()
                << " vs: " << data->node_->next_log_pos_.ToString();
            PosFromProto(data->req_->entry(data->req_->entry_size() - 1).pos(),
                         &data->node_->match_log_pos_);
            data->node_->last_log_pos_ = data->node_->match_log_pos_;
            data->node_->last_log_term_ = data->req_->entry(data->req_->entry_size() - 1).term();

            commit_pos_updated = MaybeAdvanceCommitLocked();
            if (log_writer_->Tell() > data->node_->next_log_pos_) {
                SendAppendEntriesToNodeLocked(data->node_, false, max_entries_size_);
            }
        }
    }
    pending_appends_.erase(data);
    }
    delete data;
    if (commit_pos_updated && commit_closure_) {
        commit_closure_->Run();
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// Save (i.e. client) call
//

void Server::Save(::google::protobuf::RpcController* controller,
                  const raft::pb::Data* request,
                  raft::pb::DataResponse* response,
                  ::google::protobuf::Closure* done) {
    bool response_delayed = false;
    {
        synch::MutexLocker l(&mutex_);
        if (is_leader()) {
            io::LogPos new_last_pos = log_writer_->Tell();
            pb::DataEntry entry;   // what we write in our log

            entry.set_data(request->data());
            entry.set_term(current_term_);
            entry.set_last_log_term(last_log_term_);

            PosToProto(new_last_pos, entry.mutable_pos());
            PosToProto(last_log_pos_, entry.mutable_last_log_pos());

            response->set_term(current_term_);
            PosToProto(new_last_pos, response->mutable_pos());

            if (log_writer_->WriteRecord(&entry)) {
                log_writer_->Flush(true);
                last_log_pos_ = new_last_pos;
                last_log_term_ = current_term_;
                SaveStateLocked("Save");
                // LOG_RAFT << "Perormed a write, now at: " << log_writer_->Tell().ToString()
                //          << " / " << StatusStringLocked(false);

                for (size_t i = 0; i < nodes_.size(); ++i) {
                    if (i != node_id_ && !nodes_[i]->in_transfer_) {
                        // LOG_RAFT << "=======> Checking for send: " << i << " ==> "
                        //          << nodes_[i]->next_log_pos_.ToString()
                        //          << " / " << last_log_pos_.ToString();
                        if (nodes_[i]->next_log_pos_ == last_log_pos_) {
                            SendAppendEntriesToNodeLocked(nodes_[i], false, max_entries_size_);
                        }
                    }
                }
                if (request->wait_to_commit()) {
                    commit_waiters_.insert(make_pair(new_last_pos, make_pair(response, done)));
                    response_delayed = true;
                }
            } else {
                LOG_RAFT << " Error writing log at position: " << new_last_pos.ToString();
                response->set_was_committed(false);
            }
        } else {
            LOG_RAFT_DEBUG << " Received Save request while not leader - redirecting "
                           << leader_id_;
            if (leader_id_ >= 0) {
                response->set_leader_name(nodes_[leader_id_]->name_);
            }
            response->set_was_committed(false);
        }
    }
    if (!response_delayed) {
        done->Run();
    }
}

void Server::ClearWaitersLocked() {
    for (WaitersMap::const_iterator it = commit_waiters_.begin();
         it != commit_waiters_.end(); ++it) {
        it->second.second->Run();
    }
    commit_waiters_.clear();
}

void Server::AdvanceWaitersLocked() {
    WaitersMap::iterator it = commit_waiters_.begin();
    while (it != commit_waiters_.end() && it->first <= commit_pos_) {
        it->second.first->set_was_committed(true);
        it->second.second->Run();
        WaitersMap::iterator crt = it++;
        commit_waiters_.erase(crt);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Message sending helpers
//

void Server::SendAppendLocked(Node* node, const raft::pb::AppendEntries* req) {
    CHECK(!node->in_transfer_);
    node->in_transfer_ = true;
    AppendEntriesData* data = new AppendEntriesData(node, req);
    ::google::protobuf::Closure* done = ::google::protobuf::internal::NewCallback(
        this, &Server::ProcessAppendEntriesResponse, data);
    pending_appends_.insert(data);
    node->stub_->Append(&data->controller_, data->req_, &data->resp_, done);
}

void Server::SendRequestVoteLocked(Node* node, const raft::pb::RequestVote* req) {
    RequestVoteData* data = new RequestVoteData(node, req);
    ::google::protobuf::Closure* done = ::google::protobuf::internal::NewCallback(
        this, &Server::ProcessRequestVoteResponse, data);;
    pending_votes_.insert(data);
    node->stub_->Vote(&data->controller_, data->req_, &data->resp_, done);
}

////////////////////////////////////////

void Server::FillAppendEntriesLocked(raft::pb::AppendEntries* req,
                                     const Node* node) const {
    CHECK(is_leader());
    req->set_term(current_term_);
    req->set_leader_id(node_id_);

    req->set_last_log_term(node->last_log_term_);
    PosToProto(node->last_log_pos_, req->mutable_last_log_pos());
    PosToProto(commit_pos_, req->mutable_leader_commit_pos());
}

void Server::SendHeartbeatToFollowersLocked() {
    CHECK(is_leader());
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (node_id_ == i || nodes_[i]->in_transfer_) continue;
        SendAppendEntriesToNodeLocked(nodes_[i], false, max_entries_size_);
    }
    heartbeat_elapsed_ns_ = timer::TicksNsec() + election_timeout_ms_ * 1e6 * .2;
}

void Server::SendAppendEntriesToNodeLocked(Node* node, bool is_filled, size_t max_size) {
    if (node->in_transfer_) {
        return;
    }
    raft::pb::AppendEntries* req = new raft::pb::AppendEntries();
    FillAppendEntriesLocked(req, node);
    if (!is_filled) {
        size_t size = 0;
        while (node->next_log_pos_ < log_writer_->Tell() && size < max_size) {
            node->PullNext();
            req->add_entry()->CopyFrom(node->entry_);
            node->SetPositionsFromEntry();
            size += node->entry_.ByteSize();
        }
    } else {
        req->add_entry()->CopyFrom(node->entry_);
        node->SetPositionsFromEntry();

    }
    // LOG_INFO_IF(is_filled) << "Sending : " << req->ShortDebugString()
    //                       << " to: " << node->ToString();

    SendAppendLocked(node, req);
}


void Server::SendRequestVoteLocked() {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (node_id_ == i) continue;
        raft::pb::RequestVote* req = new raft::pb::RequestVote();
        req->set_term(current_term_);
        req->set_candidate_id(node_id_);
        req->set_last_log_term(last_log_term_);
        PosToProto(last_log_pos_, req->mutable_last_log_pos());

        ++candidate_votes_out_;
        SendRequestVoteLocked(nodes_[i], req);
    }
}


////////////////////////////////////////

bool Server::MaybeAdvanceCommitLocked() {
    CHECK(is_leader());
    std::vector<io::LogPos> positions(nodes_.size());
    for (size_t i = 0; i < nodes_.size(); ++i) {
        positions[i] = nodes_[i]->match_log_pos_;
    }
    positions[node_id_] = last_log_pos_;
    ::sort(positions.begin(), positions.end());
    const io::LogPos& can_commit = positions[(positions.size() - 1) / 2];
    if (can_commit <= commit_pos_) {
        return false;
    }
    /*
      LOG_RAFT << " Committing position: " << can_commit.ToString()
      << " from : " << commit_pos_.ToString()
      << " / " << ((positions.size() - 1) / 2)
      << " / " << positions.front().ToString()
      << " ==> " << positions.back().ToString();
    */
    commit_pos_ = can_commit;
    SaveStateLocked("Maybe advance commit");
    AdvanceWaitersLocked();
    return true;
}

bool Server::MaybeTrucateLogAfterLocked(int64 prev_term, const io::LogPos& prev_pos) {
    io::LogPos writer_pos = log_writer_->Tell();
    const bool is_first_pos = writer_pos == prev_pos && writer_pos == io::LogPos(0, 0, 0);
    if (writer_pos <= prev_pos && !is_first_pos) {
        LOG_RAFT << "We are before prev pos" << writer_pos.ToString()
                 << " / " << prev_pos.ToString();
        return false;
    }
    io::LogReader* reader = nodes_[node_id_]->log_reader_;  // my reader
    if (!reader->Seek(prev_pos)) {
        LOG_RAFT << " Cannot seek to prev_pos: " << prev_pos.ToString();
        return false;
    }
    // LOG_RAFT << "Maybe truncate log: " << prev_pos.ToString() << " / " << prev_term
    //          << " / " << reader->TellAtBlock().ToString();
    if (!prev_pos.IsNull() && !is_first_pos) {
        if (!nodes_[node_id_]->ReadCurrent()) {
            return false;
        }
        if (nodes_[node_id_]->entry_.term() != prev_term) {
            LOG_RAFT << " Previous terms do not match: " << nodes_[node_id_]->entry_.term()
                     << " vs. " << prev_term;
            return false;
        }
        // LOG_RAFT << " In truncate after read current: "
        //          << nodes_[node_id_]->entry_.ShortDebugString()
        //          << " - pos: " << reader->TellAtBlock().ToString();
    } else {
        if (writer_pos.IsNull() || writer_pos == io::LogPos(0, 0, 0)) {
            last_log_pos_ = prev_pos;
            return true;
        }
    }
    if (log_writer_->Tell() > reader->TellAtBlock()) {
        if (reader->TellAtBlock() < commit_pos_) {
            LOG_RAFT << "Asked to truncate to previous position: " << prev_pos.ToString()
                     << " / now commit @: " << commit_pos_.ToString()
                     << " / " << reader->TellAtBlock().ToString();
            return false;
        }
        last_log_pos_ = prev_pos;
        SaveStateLocked("Maybe truncate");
        log_writer_->TruncateAt(reader->TellAtBlock());
    } else {
        last_log_pos_ = prev_pos;
    }
    return true;
}

bool Server::AppendRequestEntriesLocked(const raft::pb::AppendEntries* request) {
    io::LogPos prev_pos;
    io::LogPos start_pos = log_writer_->Tell();
    int64 prev_term;
    for (int i = 0; i < request->entry_size(); ++i) {
        DCHECK(PosFromProto(request->entry(i).pos()) ==
               log_writer_->Tell());
        if (i == request->entry_size() - 1) {
            prev_pos = log_writer_->Tell();
        }
        if (!log_writer_->WriteRecord(&request->entry(i))) {
            log_writer_->TruncateAt(start_pos);
            return false;
        }
        prev_term = request->entry(i).term();
        log_writer_->Flush(true);
    }
    if (request->entry_size() > 0) {
        last_log_pos_ = prev_pos;
        last_log_term_ = prev_term;
    }
    return true;
}

void Server::DegradeNodeLocked(AppendEntriesData* data) {
    Node* node = data->node_;
    // bool degraded = false;
    // LOG_RAFT_DEBUG << " Degrading: " << node->ToString();
    node->next_log_pos_ = PosFromProto(data->req_->last_log_pos());
    if (node->next_log_pos_.IsNull()) {
        node->next_log_pos_ = io::LogPos(0, 0, 0);
        node->last_log_pos_ = io::LogPos();
        node->last_log_term_ = 0;

        SendAppendEntriesToNodeLocked(node, false, max_entries_size_);
        return;
    }
    if (data->resp_.has_commit_pos()) {
        io::LogPos commit_pos = PosFromProto(data->resp_.commit_pos());
        if (commit_pos < node->next_log_pos_) {
            node->next_log_pos_ = commit_pos;
            node->PullNext();
            node->last_log_pos_ = PosFromProto(node->entry_.pos());
            node->last_log_term_ = node->entry_.term();
            node->next_log_pos_ = node->log_reader_->TellAtBlock();
            SendAppendEntriesToNodeLocked(node, false, max_entries_size_);
            return;
        }
    }
    node->PullNext();
    node->last_log_pos_ = PosFromProto(node->entry_.last_log_pos());
    node->last_log_term_ = node->entry_.last_log_term();
    node->next_log_pos_ = PosFromProto(node->entry_.pos());
    // LOG_RAFT_DEBUG << " Degraded at: " << node->ToString();

    SendAppendEntriesToNodeLocked(node, true, 1);
}

}   // namespace raft
}   // namespace whisper
