// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
//
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

#include <whisperlib/rpc/rpc_http_server.h>

#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <whisperlib/base/log.h>
#include <whisperlib/base/errno.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/rpc/rpc_consts.h>
#include <whisperlib/rpc/rpc_controller.h>
#include <whisperlib/io/buffer/protobuf_stream.h>
#include <whisperlib/http/http_server_protocol.h>
#include <whisperlib/net/ipclassifier.h>

//////////////////////////////////////////////////////////////////////

// TODO(cpopescu): make this a parameter rather than a flag ...
/*
DEFINE_string(rpc_js_form_path,
              "",
              "If specified, we export RPC accesing via auto-generated forms "
              "and we read the extra js sources from here (we need files "
              "from //depot/.../libs/net/rpc/js/");
*/

DEFINE_bool(rpc_enable_http_get,
            false,
            "If enabled we process HTTP GET requests. By default only"
            " HTTP POST requests are enabled.");
DEFINE_string(rpc_remote_address_header, "",
              "If non empty, in situations in which the server is "
              "behind a proxy, we use this header to extract the remote address");

//////////////////////////////////////////////////////////////////////

namespace rpc {
HttpServer::HttpServer(http::Server* server,
                       net::UserAuthenticator* authenticator,
                       const string& path,
                       bool is_public,
                       int max_concurrent_requests,
                       const string& ip_class_restriction)
    : authenticator_(authenticator),
      accepted_clients_(ip_class_restriction.empty() ? NULL
                        : net::IpClassifier::CreateClassifier(ip_class_restriction)),
      path_(path),
      max_concurrent_requests_(max_concurrent_requests),
      current_requests_(0),
      http_server_(server) {
  // TODO(cpopescu): enable javascript forms for debugging at some point
  /*
  if ( auto_js_forms && !FLAGS_rpc_js_form_path.empty() ) {
    const string rpc_base_js = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "rpc_base.js"));
    const string rpc_standard_js = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "rpc_standard.js"));
    js_prolog_ = ("<script language=\"JavaScript1.1\">\n" +
                  rpc_base_js + "\n" +
                  rpc_standard_js + "\n" +
                  "</script>\n");
    auto_forms_log_ = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "auto_forms_log.html"));
  }
  */
  http_server_->RegisterProcessor(path_,
      NewPermanentCallback(this, &HttpServer::ProcessRequest),
      is_public, true);
}

HttpServer::~HttpServer() {
  http_server_->UnregisterProcessor(path_);
  delete accepted_clients_;
  LOG_INFO_IF(current_requests_ > 0)
      << " Exiting the RPC server with current_requests_ "
      << "requests in processing !!";
}

bool HttpServer::RegisterService(const string& sub_path,
                                 google::protobuf::Service* service) {
  synch::MutexLocker l(&mutex_);
  const string& full_name(service->GetDescriptor()->full_name());
  const string full_path(strutil::JoinPaths(sub_path, full_name));
  ServicesMap::const_iterator it = services_.find(full_path);
  if ( it != services_.end() ) {
    LOG_WARNING << " Tried to double register " << full_path;
    return false;
  }
  LOG_INFO << " Registering: " << full_name << " on path: " << full_path;
  services_.insert(make_pair(full_path, service));
  return true;
}

bool HttpServer::UnregisterService(const string& sub_path,
                                   google::protobuf::Service* service) {
  synch::MutexLocker l(&mutex_);
  const string& full_name(service->GetDescriptor()->full_name());
  const string full_path(strutil::JoinPaths(sub_path, full_name));
  ServicesMap::iterator it = services_.find(full_path);
  if ( it == services_.end() ) {
    LOG_WARNING << " Tried to unregister " << full_path
                << " not found.";
    return false;
  }
  if ( service != it->second ) {
    LOG_WARNING << " Different service registered under " << full_path
                << full_name << " vs. " << it->second->GetDescriptor()->full_name()
                << " --> " << service << " vs. " << it->second;
    return false;
  }
  services_.erase(full_path);
  return true;
}

bool HttpServer::RegisterService(google::protobuf::Service* service) {
  return RegisterService("", service);
}

bool HttpServer::UnregisterService(google::protobuf::Service* service) {
  return UnregisterService("", service);
}

//////////////////////////////////////////////////////////////////////

void HttpServer::ProcessRequest(http::ServerRequest* req) {
  const string req_id =
      req->request()->client_header()->FindField(http::kHeaderXRequestId);
  if ( !req_id.empty() ) {
    req->request()->server_header()->AddField(
        http::kHeaderXRequestId, req_id, true);
  }
  mutex_.Lock();
  ++current_requests_;
  if ( current_requests_ >= max_concurrent_requests_ ) {
    mutex_.Unlock();
    LOG_ERROR << " Too many concurrent requests: " << current_requests_;
    // TODO(cpopescu): stats
    ReplyToRequest(req, http::INTERNAL_SERVER_ERROR, NULL, kRpcErrorServerOverloaded);
    return;
  }
  mutex_.Unlock();

  net::HostPort peer_address(GetRemoteAddress(req));
  if (peer_address.IsInvalid()) {
    LOG_ERROR << " Peer address cannot be determined: " << req->ToString();
    ReplyToRequest(req, http::BAD_REQUEST, NULL, kRpcErrorBadProxyHeader);
    return;
  }

  if ( accepted_clients_ != NULL &&
       !accepted_clients_->IsInClass(peer_address.ip_object()) ) {
    // TODO(cpopescu): stats
    LOG_ERROR << " Client not accepted: " << req->ToString()
              << " / peer: " << peer_address.ToString();
    ReplyToRequest(req, http::UNAUTHORIZED, NULL, kRpcErrorUnauthorizedIP);
    return;
  }


  ///////////////////////////////////////////////////////////////
  //
  // Split url into: service_path / service_name / method_name
  //
  const string url_path = URL::UrlUnescape(req->request()->url()->path());

  std::string sub_path = url_path.substr(path_.size());
  sub_path = strutil::StrTrimChars(sub_path, "/");
  LOG_INFO << "peer: " << peer_address.ToString()
           << " url: [" << url_path << "] , path_: [" << path_
           << "], sub_path: [" << sub_path << "]"
           << " client url: " << req->request()->client_header()->uri();

  // sub_path should be "a/b/c/service_name/method_name"
  //
  // We extract:
  //   service_full_path = "a/b/c/service_name"
  //   service_path = "a/b/c";
  //   service_name = "service_name";
  //   method_name = "method_name";
  //
  string service_full_path;
  string method_name;
  int last_slash_index = sub_path.rfind('/');
  if ( last_slash_index == std::string::npos ) {
    service_full_path = sub_path;
  } else {
    service_full_path = sub_path.substr(0, last_slash_index);
    method_name = sub_path.substr(last_slash_index + 1);
  }

  mutex_.Lock();
  ServicesMap::const_iterator it = services_.find(service_full_path);
  google::protobuf::Service* service = (it == services_.end() ? NULL : it->second);
  mutex_.Unlock();

  if (service == NULL) {
    LOG_ERROR << "Unknown service: " << service_full_path;
    // TODO(cpopescu): stats
    ReplyToRequest(req, http::NOT_FOUND, NULL, kRpcErrorServiceNotFound);
    return;
  }

  // maybe process auto-forms request -- TODO(cpopescu)
  const google::protobuf::MethodDescriptor* const method =
      service->GetDescriptor()->FindMethodByName(method_name);
  if (method == NULL) {
    LOG_ERROR << "Unknown method: " << method_name
              << " for service: " << service_full_path;
    // TODO(cpopescu): stats
    ReplyToRequest(req, http::NOT_FOUND, NULL, kRpcErrorMethodNotFound);
    return;
  }
  if (authenticator_ != NULL) {
    req->AuthenticateRequest(
      authenticator_,
      ::NewCallback(this, &HttpServer::ProcessAuthenticatedRequest,
                    req, service, method));
  } else {
    ProcessAuthenticatedRequest(req, service, method,
                                net::UserAuthenticator::Authenticated);
  }
}

////////////////////////////////////////////////////////////////////

void HttpServer::ProcessAuthenticatedRequest(
    http::ServerRequest* req,
    google::protobuf::Service* service,
    const google::protobuf::MethodDescriptor* method,
    net::UserAuthenticator::Answer auth_answer) {
  if ( !req->net_selector()->IsInSelectThread() ) {
    req->net_selector()->RunInSelectLoop(
      ::NewCallback(this, &HttpServer::ProcessAuthenticatedRequest,
                    req, service, method, auth_answer));
  }
  if ( auth_answer != net::UserAuthenticator::Authenticated ) {
    // TODO(cpopescu): stats
    LOG_INFO << "Unauthenticated request: " << req->ToString()
             << " answer: " << auth_answer;
    req->AnswerUnauthorizedRequest(authenticator_);
    return;
  }

  // Authenticated - OK !
  // read RPC packet from HTTP GET or POST
  google::protobuf::Message* request =
    service->GetRequestPrototype(method).New();

  const char* error_reason = NULL;
  if ( req->request()->client_header()->method() == http::METHOD_GET ) {
    // is HTTP GET enabled for rpc ?
    if ( !FLAGS_rpc_enable_http_get ) {
      error_reason = kRpcErrorMethodNotSupported;
    } else if (!request->ParseFromString(
                 URL::UrlUnescape(req->request()->url()->query()))) {
      error_reason = kRpcErrorBadEncoded;
    }
  } else if ( req->request()->client_header()->method() == http::METHOD_POST ) {
    if (!io::ParseProto(request, req->request()->client_data())) {
      error_reason = kRpcErrorBadEncoded;
    }
  } else {
    error_reason = kRpcErrorMethodNotSupported;
  }
  if (error_reason != NULL) {
    LOG_ERROR << "Bad request: http method: " << req->request()->client_header()->method()
              << " service:  " << service->GetDescriptor()->full_name()
              << " method: " << method->full_name()
              << " reason: " << error_reason;
    delete request;
    ReplyToRequest(req, http::BAD_REQUEST, NULL, error_reason);
    return;
  }
  StartProcessing(req, service, method, request);
}

////////////////////////////////////////////////////////////////////

net::HostPort HttpServer::GetRemoteAddress(http::ServerRequest* req) {
  if (FLAGS_rpc_remote_address_header.empty()) {
    return req->remote_address();
  }
  int32 len;
  const char* field = req->request()->client_header()->FindField(
    FLAGS_rpc_remote_address_header, &len);
  if (field == NULL) {
    return req->remote_address();
  }
  return net::HostPort(string(field, len), 80);  // pull a valid port
}

void HttpServer::StartProcessing(http::ServerRequest* req,
                                 google::protobuf::Service* service,
                                 const google::protobuf::MethodDescriptor* method,
                                 google::protobuf::Message* request) {
  net::HostPort peer_address(GetRemoteAddress(req));
  if (peer_address.IsInvalid()) {
    delete request;
    ReplyToRequest(req, http::BAD_REQUEST, NULL, kRpcErrorBadProxyHeader);
    return;
  }

  rpc::Transport* transport = new rpc::Transport(req->net_selector(),
                                                 rpc::Transport::HTTP,
                                                 net::HostPort(),
                                                 peer_address);
  if ( authenticator_ != NULL ) {
    req->request()->client_header()->GetAuthorizationField(
      transport->mutable_user(), transport->mutable_passwd());
  }

  rpc::Controller* const controller = new rpc::Controller(transport);

  // Any timeouts ?
  string timeout_str;
  if ( req->request()->client_header()->FindField("Timeout", &timeout_str) ) {
    errno = 0;   // essential as strtol would not set a 0 errno
    const int64 timeout_ms = strtoll(timeout_str.c_str(), NULL, 10);
    if ( timeout_ms > 0 ) {
      controller->set_timeout_ms(timeout_ms);
    }
  }

  // TODO(cpopescu): parametrize this one ?
  req->request()->server_header()->AddField(http::kHeaderContentType,
                                            "application/octet-stream",
                                            true);
  req->request()->set_server_use_gzip_encoding(true);

  google::protobuf::Message* response = service->GetResponsePrototype(method).New();
  google::protobuf::Closure* done_callback = google::protobuf::NewCallback(
    this, &HttpServer::RpcCallback, new RpcData(req, controller, request, response));

  service->CallMethod(method, controller, request, response, done_callback);
}

////////////////////////////////////////////////////////////////////

void HttpServer::RpcCallback(RpcData* data) {
  if (data->controller_->Failed()) {
    ReplyToRequest(data->req_, http::INTERNAL_SERVER_ERROR, data->controller_, NULL);
  } else if (!io::SerializeProto(data->response_, data->req_->request()->server_data())) {
    ReplyToRequest(data->req_, http::INTERNAL_SERVER_ERROR, NULL,
                   kRpcErrorSerializingResponse);
  } else {
    ReplyToRequest(data->req_, http::OK, NULL, NULL);
  }
  delete data;
}

////////////////////////////////////////////////////////////////////

void HttpServer::ReplyToRequest(http::ServerRequest* req,
                                int status,
                                rpc::Controller* controller,
                                const char* error_reason) {
  mutex_.Lock();
  --current_requests_;

  mutex_.Unlock();
  // TODO(cpopescu): fix this !!
  req->request()->set_server_use_gzip_encoding(false);

  if (controller != NULL) {
    controller->CallCancelCallback(false);
    if (controller->Failed()) {
      req->request()->server_header()->AddField(
        rpc::kRpcErrorCode, sizeof(rpc::kRpcErrorCode) - 1,
        strutil::StringPrintf("%d", controller->GetErrorCode()), true);
      req->request()->server_header()->AddField(
        http::kHeaderContentType, sizeof(http::kHeaderContentType) - 1,
        kRpcErrorContentType, sizeof(kRpcErrorContentType) - 1, true, true);
      req->request()->server_data()->Write(controller->ErrorText());
    }
  } else if (error_reason) {
    req->request()->server_header()->AddField(
      http::kHeaderContentType, sizeof(http::kHeaderContentType) - 1,
      kRpcErrorContentType, sizeof(kRpcErrorContentType) - 1, true, true);
    req->request()->server_data()->Write(error_reason);
  } else {
    req->request()->server_header()->AddField(
      http::kHeaderContentType,
      sizeof(http::kHeaderContentType) - 1,
      kRpcContentType, sizeof(kRpcContentType) - 1, true, true);
    /*
    req->request()->server_header()->AddField(http::kHeaderContentEncoding,
                                              sizeof(http::kHeaderContentEncoding) - 1,
                                              kRpcGzipEncoding, sizeof(kRpcGzipEncoding) - 1,
                                            true, true);
    */
  }

  if (req->net_selector()->IsInSelectThread()) {
    req->ReplyWithStatus(http::HttpReturnCode(status));
  } else {
    req->net_selector()->RunInSelectLoop(
      ::NewCallback(req, &http::ServerRequest::ReplyWithStatus,
                    http::HttpReturnCode(status)));
  }
}

HttpServer::RpcData::~RpcData() {
  delete controller_;
  delete request_;
  delete response_;
}

}
