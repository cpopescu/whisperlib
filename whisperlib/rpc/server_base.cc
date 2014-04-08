// Copyright: 1618labs, Inc. 2013 onwards.
// All rights reserved.
// cp@1618labs.com
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
//

#include <whisperlib/rpc/server_base.h>

namespace rpc {
ServerBase::ServerBase(int argc, char** argv,
                       const string& name)
    : app::App(argc, argv),
      name_(name),
      num_threads_(0),
      port_(0),
      selector_(NULL),
      net_factory_(NULL),
      http_server_(NULL) {
}

ServerBase::~ServerBase() {
    CHECK_NULL(selector_);
    CHECK_NULL(net_factory_);
    CHECK_NULL(http_server_);
    CHECK(working_threads_.empty());
}

int ServerBase::Initialize() {
    selector_ = new net::Selector();

    for ( int i = 0; i < num_threads_; ++i ) {
        working_threads_.push_back(new net::SelectorThread());
        working_threads_.back()->Start();
    }
    net_factory_ = new net::NetFactory(selector_);
    acceptor_params_.set_client_threads(&working_threads_);
    net_factory_->SetTcpParams(acceptor_params_);
    server_params_.max_reply_buffer_size_ = 1 << 20;
    http_server_ = new http::Server(name_.c_str(), selector_, *net_factory_,
                                    server_params_);
    http_server_->AddAcceptor(net::PROTOCOL_TCP, net::HostPort(0, port_));
    selector_->RunInSelectLoop(NewCallback(http_server_, &http::Server::StartServing));

    return 0;
}

void ServerBase::Stop() {
    if (!selector_) return;
    CHECK(!selector_->IsInSelectThread());
    // delegate to selector (because we cannot stop the networking from outside)
    selector_->RunInSelectLoop(NewCallback(this, &ServerBase::TriggerStop));
}


void ServerBase::StopInSelectThread() {
    for (int i = 0; i < working_threads_.size(); ++i) {
        working_threads_[i]->CleanAndCloseAll();
    }
    if (http_server_ != NULL) {
        selector_->RunInSelectLoop(NewCallback(http_server_,
                                               &http::Server::StopServing));
    }
    selector_->RunInSelectLoop(NewCallback(selector_,
                                           &net::Selector::MakeLoopExit));
    selector_ = NULL;
}

void ServerBase::Shutdown() {
    delete http_server_;
    http_server_ = NULL;
    for (int i = 0; i < working_threads_.size(); ++i) {
        delete  working_threads_[i];
    }
    working_threads_.clear();
    delete selector_;
    selector_ = NULL;
    delete net_factory_;
    net_factory_ = NULL;
}

int64 ServerBase::GetDebugIdParam(http::ServerRequest* http_req) {
    if (http_req->request()->url() == NULL) {
        return 0;
    }
    const string id_str = strutil::Basename(http_req->request()->url()->path());
    return strtoll(id_str.c_str(), NULL, 10);
}
void ServerBase::ToDebugHtmlTable(int64 id_val,
                                  const vector< pair<string, google::protobuf::Message*> >& data,
                                  http::ServerRequest* http_req) {
    http_req->request()->server_data()->Write(
        strutil::StringPrintf("<html><body><h1> Id lookup: %ld</h1>"
                              "<table border=1>", id_val));
    for (int i = 0; i < data.size(); ++i) {
        http_req->request()->server_data()->Write(
            strutil::StringPrintf("<tr><td>%s</td><td>%s</td></tr>\n",
                                  data[i].first.c_str(),
                                  data[i].second->Utf8DebugString().c_str()));
        delete data[i].second;
    }
        http_req->request()->server_data()->Write("</table></html>");
}

}
