// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
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
// Protobuf adaptation:
//
// (c) Copyright 2011, 1618labs
// All rights reserved.
// Author: Catalin Popescu (cp@1618labs.com)
//
#include <whisperlib/rpc/rpc_http_client.h>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/callback.h>
#include <whisperlib/base/strutil.h>
#include <whisperlib/http/http_client_protocol.h>
#include <whisperlib/http/failsafe_http_client.h>
#include <whisperlib/io/buffer/protobuf_stream.h>
#include <whisperlib/rpc/rpc_controller.h>
#include <whisperlib/rpc/rpc_consts.h>

namespace rpc {

HttpClient::HttpClient(http::FailSafeClient* failsafe_client,
                       const string& http_request_path)
  : RpcChannel(),
    selector_(failsafe_client->selector()),
    failsafe_client_(failsafe_client),
    http_request_path_(http_request_path),
    xid_(0),
    closing_(false) {
}

HttpClient::HttpClient(http::FailSafeClient* failsafe_client,
                       const string& http_request_path,
                       const vector< pair<string, string> >& request_headers,
                       const string& auth_user,
                       const string& auth_pass)
  : RpcChannel(),
    selector_(failsafe_client->selector()),
    failsafe_client_(failsafe_client),
    http_request_path_(http_request_path),
    request_headers_(request_headers),
    auth_user_(auth_user),
    auth_pass_(auth_pass),
    xid_(0),
    closing_(false) {
}

HttpClient::~HttpClient() {
  LOG_INFO << " Deleting rpc http client " << this;
  // CHECK(queries_.empty());
}

void HttpClient::StartClose() {
  if (!selector_->IsInSelectThread()) {
    selector_->RunInSelectLoop(::NewCallback(this, &HttpClient::StartClose));
    return;
  }

  LOG_INFO << "Starting to close http client: " << this << " / " << ToString();

  vector<int64> to_cancel;
  mutex_.Lock();
  closing_ = true;
  for (QueryMap::const_iterator it = queries_.begin(); it != queries_.end(); ++it) {
    to_cancel.push_back(it->first);
  }
  to_wait_cancel_.clear();
  mutex_.Unlock();

  for (int i = 0; i < to_cancel.size(); ++i) {
    if (!CancelRequestVerified(to_cancel[i])) {
      to_wait_cancel_.insert(to_cancel[i]);
    }
  }
  failsafe_client_->ForceCloseAll();

  if (to_wait_cancel_.empty()) {
    LOG_INFO << " Done Http Rpc client - deleting self: " << ToString()
             << " canceled: " << to_cancel.size() << " self: " << this;
    selector_->DeleteInSelectLoop(this);
  } else {
    LOG_INFO << " Done start close http client (deleting later): " << ToString()
             << " canceled: " << to_cancel.size() << " self: " << this;
  }
}

string HttpClient::ToString() const {
  return strutil::StringPrintf(
    "HttpClient{"
    "failsafe_client_: 0x%p, "
    "http_request_path_: %s, "
    "auth_user_: %s, "
    "queries_: #%zu queries}",
    failsafe_client_,
    http_request_path_.c_str(),
    auth_user_.c_str(),
    queries_.size());
}

void HttpClient::CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done) {
  // Create a http request containing:
  //  - the required method
  //  - the encoded RPC message
  //
  rpc::Controller* rpc_controller = reinterpret_cast<rpc::Controller*>(controller);

  VLOG(5) << "Sending request on http path: [" << http_request_path_ << "]";
  // req will be deleted on CallbackRequestDone
  const string path(strutil::JoinPaths(http_request_path_, method->name()));
  http::ClientRequest* req = new http::ClientRequest(http::METHOD_POST, path);

  if (req == NULL) {
    rpc_controller->SetErrorCode(rpc::ERROR_CLIENT);
    rpc_controller->SetFailed("Error in request allocation");
    done->Run();
    return;
  }
  // write necessary parameters - headers
  for (int i = 0; i != request_headers_.size(); ++i) {
    req->request()->client_header()->AddField(request_headers_[i].first,
                                              request_headers_[i].second, true);
  }
  if ( !auth_user_.empty() ) {
    req->request()->client_header()->SetAuthorizationField(auth_user_, auth_pass_);
  }
  const int64 xid = GetNextXid();

  req->request()->client_header()->AddField(
    kRpcHttpXid, sizeof(kRpcHttpXid) - 1,
    strutil::StringPrintf("%" PRId64, xid), true, true);
  req->request()->client_header()->AddField(
    http::kHeaderContentType,
    sizeof(http::kHeaderContentType) - 1,
    kRpcContentType, sizeof(kRpcContentType) - 1, true, true);
  //req->request()->client_header()->AddField(http::kHeaderContentEncoding,
  //                                          sizeof(http::kHeaderContentEncoding) - 1,
  //                                          kRpcGzipEncoding, sizeof(kRpcGzipEncoding) - 1,
  //                                          true, true);


  // write RPC message
  io::SerializeProto(request, req->request()->client_data());

  google::protobuf::Closure* const cancel_callback =
    google::protobuf::NewCallback(this, &rpc::HttpClient::CallbackCancelRequested, xid);
  QueryStruct* qs = new QueryStruct(req, xid, method, rpc_controller, response, done, cancel_callback);

  mutex_.Lock();
  if (closing_) {
    delete qs;
    rpc_controller->SetErrorCode(rpc::ERROR_CLIENT);
    rpc_controller->SetFailed("We are closing the client");
    mutex_.Unlock();

    done->Run();
  } else {
    rpc_controller->NotifyOnCancel(cancel_callback);
    queries_.insert(make_pair(xid, qs));
    selector_->RunInSelectLoop(::NewCallback(this, &rpc::HttpClient::StartRequest, qs));
    mutex_.Unlock();
  }
}

////////////////////////////////////////////////////////////////////////////////

void HttpClient::StartRequest(HttpClient::QueryStruct* qs) {
  if (qs->cancelled_) {
    CallbackRequestDone(qs);
  } else {
    mutex_.Lock();
    if (closing_) {
      google::protobuf::Closure* done = qs->done_;
      qs->controller_->SetErrorCode(rpc::ERROR_CLIENT);
      qs->controller_->SetFailed("We are closing the client");
      delete qs;
      mutex_.Unlock();
      done->Run();
    } else {
      qs->started_ = true;
      Closure* const req_done_callback = ::NewCallback(
        this, &rpc::HttpClient::CallbackRequestDone, qs);
      selector_->RunInSelectLoop(::NewCallback(failsafe_client_,
                                               &http::FailSafeClient::StartRequestWithUrgency,
                                               qs->req_, req_done_callback,
                                               qs->controller_->is_urgent()));
      mutex_.Unlock();
    }
  }
}

void HttpClient::CallbackCancelRequested(int64 xid) {
  mutex_.Lock();
  if (closing_) return;
  mutex_.Unlock();
  Closure* cancel_callback = ::NewCallback(this, &rpc::HttpClient::CancelRequest, xid);
  selector_->RunInSelectLoop(cancel_callback);
}

HttpClient::QueryStruct::~QueryStruct() {
  delete req_;
  delete cancel_callback_;
}

void HttpClient::CancelRequest(int64 xid) {
  CancelRequest(xid);
}
bool HttpClient::CancelRequestVerified(int64 xid) {
  HttpClient::QueryStruct* qs = NULL;
  mutex_.Lock();
  const QueryMap::const_iterator it = queries_.find(xid);
  if (it != queries_.end()) {
    qs = it->second;
  }
  mutex_.Unlock();
  if (qs == NULL) {
    return true;   // nothing left to cancel
  }
  DCHECK(selector_->IsInSelectThread());

  qs->cancel_callback_ = NULL;
  CHECK(!qs->cancelled_);
  qs->cancelled_ = true;

  if (qs->started_) {
    if (failsafe_client_->CancelRequest(qs->req_)) {
      // We'll never get the CallbackRequestDone
      CallbackRequestDone(qs);
      return true;
    }  // else we will receive a callback
    return false;
  } // else StartRequest takes care of it
  return true;
}

string HttpClient::ToString(const HttpClient::QueryStruct* qs) const {
  return strutil::StringPrintf("%s @%s xid:%" PRId64,
                               http_request_path_.c_str(),
                               qs->method_->DebugString().c_str(),
                               qs->xid_);
}


void HttpClient::CallbackRequestDone(HttpClient::QueryStruct* qs) {
  DCHECK(selector_->IsInSelectThread());
  LOG_INFO << "Callback done request: " << qs->xid_ << " / " << ToString() << " / "
          << qs->cancelled_ << " // " << qs->req_->name();
  mutex_.Lock();
  CHECK(queries_.erase(qs->xid_));
  // From this moment the request cannot be canceled
  qs->controller_->NotifyOnCancel(NULL);
  delete qs->cancel_callback_;
  qs->cancel_callback_ = NULL;
  mutex_.Unlock();
  bool delete_self = false;
  if ( !qs->cancelled_ ) {
    const http::ClientError cli_error = qs->req_->error();
    const http::HttpReturnCode ret_code =
        qs->req_->request()->server_header()->status_code();
    if ( cli_error != http::CONN_OK) {
      LOG_ERROR << "Network Error: req[" << ToString(qs) << "] : " << http::ClientErrorName(cli_error);
      qs->controller_->SetErrorCode(rpc::ERROR_NETWORK);
      qs->controller_->SetFailed(http::ClientErrorName(cli_error));
    } else if (ret_code != http::OK ) {
      LOG_ERROR << "Server Http Error: req[" << ToString(qs) << "]: " << http::GetHttpReturnCodeName(ret_code);
      qs->controller_->SetErrorCode(rpc::ERROR_SERVER);
      qs->controller_->SetFailed(qs->req_->request()->server_data()->ToString());
    } else {
      // wrap request buffer
      io::MemoryStream* const in = qs->req_->request()->server_data();
      in->MarkerSet(); // to be able to restore data on error & print
                       // it for debug
      if (!io::ParseProto(qs->response_, in)) {
        qs->controller_->SetErrorCode(rpc::ERROR_PARSE);
        in->MarkerRestore();
        LOG_ERROR << "Parsing Response Error: req[" << ToString(qs) << "] size:" << in->Size();
        DLOG_INFO << "Received message: " << in->DebugString();
        in->Clear();  // need to consume data.
      } else {
        in->MarkerClear();
      }
    }
  } else {
    qs->req_->request()->server_data()->Clear();
    if (!to_wait_cancel_.empty()) {
      to_wait_cancel_.erase(qs->xid_);
      delete_self = to_wait_cancel_.empty();
    }
  }
  qs->done_->Run();
  delete qs;

  if (delete_self) {
    LOG_INFO << " Done Http Rpc client - deleting self: " << ToString();
    selector_->DeleteInSelectLoop(this);
  }
}
}
