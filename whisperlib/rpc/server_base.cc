// Copyright: Urban Engines, Inc. 2012 onwards.
// All rights reserved.
// cp@urbanengines.com

#include "whisperlib/rpc/server_base.h"
#include "whisperlib/rpc/rpc_http_server.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#endif

using namespace std;

namespace whisper {
namespace rpc {
ServerBase::ServerBase(int& argc, char**& argv, const string& name)
    : app::App(argc, argv),
      name_(name),
      num_threads_(0),
      port_(0),
      max_concurrent_requests_(300),
      rpc_path_("/rpc"),
      selector_(NULL),
      net_factory_(NULL),
      http_server_(NULL),
      rpc_server_(NULL) {
}

ServerBase::~ServerBase() {
    CHECK_NULL(selector_);
    CHECK_NULL(net_factory_);
    CHECK_NULL(http_server_);
    CHECK_NULL(rpc_server_);
    CHECK(working_threads_.empty());
}

int ServerBase::Initialize() {
    if (selector_)  return 0;   // already initialized
#ifdef USE_OPENSSL
    SSL_library_init();
#endif
    selector_ = new net::Selector();

    for (int i = 0; i < num_threads_; ++i) {
        working_threads_.push_back(new net::SelectorThread());
        working_threads_.back()->Start();
    }
    net_factory_ = new net::NetFactory(selector_);
    acceptor_params_.set_client_threads(&working_threads_);
    net_factory_->SetTcpParams(acceptor_params_);
    http_server_ = new http::Server(name_.c_str(), selector_, *net_factory_, http_params_);
    http_server_->AddAcceptor(net::PROTOCOL_TCP, net::HostPort(0, port_));
    selector_->RunInSelectLoop(NewCallback(http_server_, &http::Server::StartServing));

    rpc_server_ = new rpc::HttpServer(
        http_server_, NULL, rpc_path_, true, max_concurrent_requests_, "");

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
    StopServicesInSelectThread();
    selector_->RunInSelectLoop(NewCallback(selector_,
                                           &net::Selector::MakeLoopExit));
    selector_ = NULL;
}

void ServerBase::Shutdown() {
    delete rpc_server_;
    rpc_server_ = NULL;
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

}  // namespace rpc
}  // namespace whisper
