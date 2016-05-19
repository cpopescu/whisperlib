/**
 * Copyright (c) 2014, Urban Engines inc.
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


#include <stdlib.h>

#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/system.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/io/ioutil.h"
#include "whisperlib/sync/thread.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/raft/raft_server.h"
#include "whisperlib/raft/raft_client.h"
#include "whisperlib/rpc/rpc_http_server.h"
#include "whisperlib/io/buffer/protobuf_stream.h"
#include "whisperlib/io/logio/logio.h"

#include "whisperlib/net/selector.h"
#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/http/http_client_protocol.h"

DEFINE_int32(port, 7800, "Start servers from this port");
DEFINE_int32(num_servers, 3, "Number of replicas");
DEFINE_int32(num_clients, 3, "Number of clients");
DEFINE_string(raft_dir, "/tmp/raft_test", "Logs base");
DEFINE_int64(message_id, 0, "Start sending messages w. this id");

std::string LogName(size_t node_id) {
    return strutil::StringPrintf("raft_%02zd", node_id);
}

////////////////////////////////////////////////////////////////////////////////

class RaftServerWrap {
public:
    RaftServerWrap(const std::vector<std::string>& replicas, size_t node_id)
        : replicas_(replicas),
          node_id_(node_id),
          net_factory_(NULL),
          http_server_(NULL),
          rpc_server_(NULL),
          raft_(NULL) {
    }
    ~RaftServerWrap() {
    }

    void Start() {
        net_factory_ = new whisper::net::NetFactory(selector_.mutable_selector());
        net_factory_->SetTcpParams(acceptor_params_);
        server_params_.max_reply_buffer_size_ = 1 << 20;

        http_server_ = new whisper::http::Server(replicas_[node_id_].c_str(),
                                        selector_.mutable_selector(), *net_factory_,
                                        server_params_);
        http_server_->AddAcceptor(
            whisper::net::PROTOCOL_TCP, whisper::net::HostPort(0, FLAGS_port + node_id_));

        rpc_server_ = new whisper::rpc::HttpServer(http_server_, NULL, "/test", true, 1000, "");
        whisper::io::LogWriter* writer = new whisper::io::LogWriter(
            FLAGS_raft_dir, LogName(node_id_), 160, 10000, false, false);
        CHECK(writer->Initialize());
        raft_ = new whisper::raft::Server(rpc_server_, "raft", writer, NULL);

        selector_.mutable_selector()->RunInSelectLoop(
            whisper::NewCallback(http_server_, &whisper::http::Server::StartServing));
        CHECK(raft_->Initialize(node_id_, replicas_));

        selector_.Start();
    }

    void Stop() {
        if (!http_server_) {
            return;
        }
        selector_.CleanAndCloseAll();
        selector_.mutable_selector()->RunInSelectLoop(
            whisper::NewCallback(http_server_, &whisper::http::Server::StopServing));
        selector_.Stop();
    }
    whisper::raft::Server* raft() {
        return raft_;
    }
private:
    const std::vector<std::string> replicas_;
    const size_t node_id_;

    whisper::net::TcpAcceptorParams acceptor_params_;
    whisper::http::ServerParams server_params_;

    whisper::net::SelectorThread selector_;
    whisper::net::NetFactory* net_factory_;
    whisper::http::Server* http_server_;
    whisper::rpc::HttpServer* rpc_server_;
    whisper::raft::Server* raft_;
};

////////////////////////////////////////////////////////////////////////////////

class RaftClientWrap {
public:
    RaftClientWrap(const std::vector<std::string>& replicas, int client_id)
        : replicas_(replicas), client_id_(client_id),
          raft_(NULL) {

    }
    ~RaftClientWrap() {
        CHECK(raft_ == NULL) << " Should stop before delete";
    }
    void Start() {
        raft_ = new whisper::raft::Client(selector_.mutable_selector(), replicas_, "/test/raft");
        raft_->mutable_params()->default_request_timeout_ms_ = 40000;
        selector_.Start();
    }
    void Stop() {
        selector_.mutable_selector()->DeleteInSelectLoop(raft_);
        selector_.Stop();
        raft_ = NULL;
    }
    void SendMessages(int64 start_time, int64 start, int32 num) {
        if (num > 0) {
            selector_.mutable_selector()->RunInSelectLoop(
                whisper::NewCallback(this, &RaftClientWrap::StartSend, start_time, start, num));
        }
    }
    whisper::raft::Client* raft() {
        return raft_;
    }
private:
    void StartSend(int64 start_time, int64 start, int32 num) {
        raft_->SendData(strutil::StringPrintf("%010" PRId64, start),
                        whisper::NewCallback(this, &RaftClientWrap::DataCommitted,
                                             start_time, start, num));
    }
    void DataCommitted(int64 start_time, int64 message_id, int32 num, bool success) {
        const int64 now = whisper::timer::TicksNsec();
        if (success) {
            printf("# [%d] Message committed: %" PRId64 " [%.2f sec]\n",
                   client_id_, message_id, (now - start_time) * 1e-9);
        } else {
            printf("# [%d] Message NOT committed: %" PRId64 " [%.2f sec]\n",
                   client_id_, message_id, (now - start_time) * 1e-9);
        }
        SendMessages(start_time, message_id + 1, num - 1);
    }

    const std::vector<std::string> replicas_;
    int client_id_;
    whisper::net::SelectorThread selector_;
    whisper::raft::Client* raft_;
};

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    whisper::common::Init(argc, argv);

    std::vector<std::string> replicas;
    std::vector<RaftServerWrap*> servers;
    std::vector<RaftClientWrap*> clients;
    whisper::net::SelectorThread selector;

    for (int i = 0; i < FLAGS_num_servers; ++i) {
        replicas.push_back(strutil::StringPrintf("127.0.0.1:%d", FLAGS_port + i));
    }
    for (int i = 0; i < FLAGS_num_servers; ++i) {
        RaftServerWrap* s = new RaftServerWrap(replicas, i);
        servers.push_back(s);
        s->Start();
    }
    for (int i = 0; i < FLAGS_num_clients; ++i) {
        RaftClientWrap* c = new RaftClientWrap(replicas, i);
        clients.push_back(c);
        c->Start();
    }

    char command[1024];
    int64 message_id = FLAGS_message_id;
    while (!std::cin.eof()) {
        printf("===> ");
        std::cin.getline(command, sizeof(command));
        std::string scommand = strutil::StrTrim(command);
        std::vector <std::string> comp;
        strutil::SplitStringOnAny(scommand, " \t\n\r", &comp, true);
        if (comp.empty()) {
            continue;
        }
        if (comp[0] == "quit") {
            break;
        } else if ((comp[0] == "p" || comp[0] == "print") && comp.size() > 1) {
            const int log_id = atoi(comp[1].c_str());
            whisper::io::LogReader reader(
                FLAGS_raft_dir, LogName(log_id), 160, 10000);
            whisper::io::MemoryStream buffer;
            whisper::io::LogPos pos = reader.Tell();
            printf("#===================== LOG id: %d\n", log_id);
            while (reader.GetNextRecord(&buffer)) {
                whisper::raft::pb::DataEntry entry;
                CHECK(whisper::io::ParseProto(&entry, &buffer));
                printf(" @%s >>> %s\n", pos.ToString().c_str(),
                       entry.ShortDebugString().c_str());
                pos = reader.Tell();
            }
        } else if (comp[0] == "down" && comp.size() > 1) {
            const int idx = atoi(comp[1].c_str());
            if (idx >= 0 && size_t(idx) < servers.size()) {
                printf("# Taking down server %d\n", idx);
                servers[idx]->Stop();
                delete servers[idx];
                servers[idx] = NULL;
            }
        } else if (comp[0] == "up" && comp.size() > 1) {
            const int idx = atoi(comp[1].c_str());
            if (idx >= 0 && size_t(idx) < servers.size() &&
                servers[idx] == NULL) {
                printf("# Putting up server %d\n", idx);
                servers[idx] = new RaftServerWrap(replicas, idx);
                servers[idx]->Start();
            }
        } else if (comp[0] == "status" || comp[0] == "stat") {
            const bool detailed = (comp[0] == "status");
            if (comp.size() > 1) {
                const int idx = atoi(comp[1].c_str());
                if (idx >= 0 && size_t(idx) < servers.size() &&
                    servers[idx] != NULL) {
                    printf("%s\n", servers[idx]->raft()->StatusString(
                               detailed).c_str());
                }
            } else {
                for (size_t i = 0; i < servers.size(); ++i) {
                    printf("#==================== STATUS Server %zd\n", i);
                    if (servers[i]) {
                        printf("%s\n", servers[i]->raft()->StatusString(detailed).c_str());
                    } else {
                        printf("DOWN\n");
                    }
                }
            }
        } else if (comp[0] == "send") {
            const int idx = comp.size() > 2 ? atoi(comp[2].c_str()) : -1;
            const int num = comp.size() > 1 ? atoi(comp[1].c_str()) : 1;
            if (num > 0) {
                if (idx >= 0 && size_t(idx) < clients.size()) {
                    printf(
                        "# Client %d sending %d messages start: %" PRId64 "\n",
                        idx, num, message_id);
                    clients[idx]->SendMessages
                        (whisper::timer::TicksNsec(), message_id, num);
                    message_id += num;
                } else {
                    for (size_t i = 0; i < clients.size(); ++i) {
                        printf("# Client %zd sending %d messages start: %" PRId64 "\n",
                               i, num, message_id);
                        clients[i]->SendMessages(
                            whisper::timer::TicksNsec(), message_id, num);
                        message_id += num;
                    }
                }
            }
        }


    }
    for (size_t i = 0; i < clients.size(); ++i) {
        clients[i]->Stop();
        delete clients[i];
    }
    for (size_t i = 0; i < servers.size(); ++i) {
        if (servers[i]) {
            servers[i]->Stop();
            delete servers[i];
        }
    }
    printf("DONE\n");
    return 0;
}
