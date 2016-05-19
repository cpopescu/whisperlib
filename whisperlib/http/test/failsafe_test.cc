// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Catalin Popescu
//
// Gets a bunch of pages from servers..
//

#include <netdb.h>
#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/system.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/net/selector.h"
#include "whisperlib/http/failsafe_http_client.h"


DEFINE_string(servers,
              "",
              "Servers from where to get the pages (comma-separated)");
DEFINE_string(paths,
              "",
              "Servers from where to get the pages (comma-separated)");

DEFINE_int32(req_timeout_ms,
             60000,
             "Timeout request after this time...");
DEFINE_string(force_host_header,
              "",
              "Force a Host header (sometimes apache wants that)");
DEFINE_int32(cancel_every,
             0,
             "If non zero we cancel every N request");

int32 glb_num_request = 0;

void RequestDone(whisper::net::Selector* selector,
                 whisper::http::FailSafeClient* fsc,
                 whisper::http::ClientRequest* req) {
  CHECK(req->is_finalized());
  LOG_INFO << "=================================================";
  LOG_INFO << " Request " << req->name() << " finished w/ error: "
           << req->error_name();
  LOG_INFO << " Response received from server (so far): "
           << "\nHeader:\n"
           << req->request()->server_header()->ToString();
  std::string content = req->request()->server_data()->ToString();
  // LOG_INFO << "Body:\n" << content;

  LOG_INFO << "========= Done requests: " << glb_num_request;
  CHECK_GE(glb_num_request, 0);
  --glb_num_request;
  if ( glb_num_request <= 0 ) {
    selector->DeleteInSelectLoop(fsc);
    selector->RunInSelectLoop(
        whisper::NewCallback(selector, &whisper::net::Selector::MakeLoopExit));
  }
}

whisper::http::BaseClientConnection* CreateConnection(whisper::net::Selector* selector,
                                             whisper::net::NetFactory* net_factory,
                                             whisper::net::PROTOCOL net_protocol) {
  return new whisper::http::SimpleClientConnection(selector, *net_factory, net_protocol);
}

void CancelRequest(whisper::net::Selector* selector,
                   whisper::http::FailSafeClient* fsc,
                   whisper::http::ClientRequest* req) {
    if (fsc->CancelRequest(req)) {
        LOG_INFO << "#######################################\n Request canceled: "  << req->name();
        LOG_INFO << "========= Done requests: " << glb_num_request;
        --glb_num_request;
    } else {
        LOG_INFO << " Request already done..";
    }
    CHECK_GE(glb_num_request, 0);
    if ( glb_num_request <= 0 ) {
        selector->DeleteInSelectLoop(fsc);
        selector->RunInSelectLoop(
            whisper::NewCallback(selector, &whisper::net::Selector::MakeLoopExit));
    }
}

int main(int argc, char* argv[]) {
  whisper::common::Init(argc, argv);
  std::vector<whisper::net::HostPort> servers;
  std::vector<std::string> server_names;
  std::vector<std::string> server_paths;
  if ( FLAGS_servers.empty() ) {
    LOG_FATAL << " No --servers specified";
  }
  if ( FLAGS_paths.empty() ) {
    LOG_FATAL  << " No --path specified";
  }
  strutil::SplitString(FLAGS_servers, ",", &server_names);
  strutil::SplitString(FLAGS_paths, ",", &server_paths);

  for ( size_t i = 0; i < server_names.size(); ++i  ) {
    std::vector<std::string> comps;
    strutil::SplitString(server_names[i], ":", &comps);
    int port = 80;
    if ( comps.size() > 1 ) {
      port = atoi(comps[1].c_str());
    }

    struct hostent *hp = gethostbyname(comps[0].c_str());
    CHECK(hp != NULL)
        << " Cannot resolve: " << comps[0];
    CHECK(hp->h_addr_list[0] != NULL)
        << " Bad address received for: " << comps[0];
    servers.push_back(whisper::net::HostPort(ntohl(reinterpret_cast<struct in_addr*>(
                                              hp->h_addr_list[0])->s_addr),
                                    port));
  }
  whisper::net::Selector selector;
  whisper::net::NetFactory net_factory(&selector);
  whisper::http::ClientParams params;
  // params.dlog_level_ = true;
  whisper::http::FailSafeClient* fsc = new whisper::http::FailSafeClient(
      &selector, &params, servers,
      whisper::NewPermanentCallback(&CreateConnection, &selector,
                                    &net_factory, whisper::net::PROTOCOL_TCP),
      true,
      4,
      400000, 5000,
      FLAGS_force_host_header);
  std::vector<whisper::http::ClientRequest*> reqs;
  for ( size_t i = 0; i < server_paths.size(); ++i ) {
    LOG_INFO << "Scheduling request for: " << server_paths[i];
    whisper::http::ClientRequest* req = new whisper::http::ClientRequest(
        whisper::http::METHOD_GET, server_paths[i]);
    whisper::Closure* cb = whisper::NewCallback(
        &RequestDone, &selector, fsc, req);
    reqs.push_back(req);
    selector.RunInSelectLoop(
        whisper::NewCallback(fsc,
                             &whisper::http::FailSafeClient::StartRequest,
                             req, cb));
    glb_num_request++;
  }
  if (FLAGS_cancel_every > 0) {
    for ( size_t i = 0; i < server_paths.size(); ++i ) {
      if ((i % FLAGS_cancel_every) == 0) {
        whisper::Closure* cb = whisper::NewCallback(
            &CancelRequest, &selector, fsc, reqs[i]);
        selector.RegisterAlarm(cb, i * 150);
      }
    }
  }
  selector.Loop();
  printf("DONE\n");
  return 0;
}
