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
// (c) Copyright 2011, Urban Engines
// All rights reserved.
// Author: Catalin Popescu (cp@urbanengines.com)
//
#include "whisperlib/rpc/rpc_http_client.h"

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "whisperlib/base/log.h"
#include "whisperlib/base/callback.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/http/http_client_protocol.h"
#include "whisperlib/http/failsafe_http_client.h"
#include "whisperlib/io/buffer/protobuf_stream.h"
#include "whisperlib/io/num_streaming.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/rpc/rpc_consts.h"
#include "whisperlib/sync/event.h"
#include "whisperlib/rpc/RpcStats.pb.h"

using namespace std;

namespace whisper {
namespace rpc {

HttpClient::HttpClient(http::FailSafeClient* failsafe_client,
                       const string& http_request_path)
  : RpcChannel(),
    selector_(failsafe_client->selector()),
    failsafe_client_(failsafe_client),
    server_name_(failsafe_client_->server_names()),
    http_request_path_(http_request_path),
    xid_(2 + (timer::TicksNsec() % 256)),  // small number over 1 - for differentiation in statusz
    closing_(false),
    stats_msg_text_size_(2048),
    stats_msg_history_size_(200) {
}

HttpClient::HttpClient(http::FailSafeClient* failsafe_client,
                       const string& http_request_path,
                       const vector< pair<string, string> >& request_headers,
                       const string& auth_user,
                       const string& auth_pass)
  : RpcChannel(),
    selector_(failsafe_client->selector()),
    failsafe_client_(failsafe_client),
    server_name_(failsafe_client_->server_names()),
    http_request_path_(http_request_path),
    request_headers_(request_headers),
    auth_user_(auth_user),
    auth_pass_(auth_pass),
    xid_(2 + (timer::TicksNsec() % 256)),  // small number over 1 - for differentiation in statusz
    closing_(false),
    stats_msg_text_size_(2048),
    stats_msg_history_size_(200) {
}

HttpClient::~HttpClient() {
  LOG_INFO << " Deleting rpc http client " << this;
  while (completed_queries_.size() > stats_msg_history_size_) {
    delete completed_queries_.back();
    completed_queries_.pop_back();
  }
  // CHECK(queries_.empty());
}

void HttpClient::StartClose() {
  if (!selector_->IsInSelectThread()) {
    selector_->RunInSelectLoop(whisper::NewCallback(this, &HttpClient::StartClose));
    return;
  }

  LOG_INFO << "Starting to close http client: " << this << " / " << ToString();

  vector<int64> to_cancel;
  vector<http::ClientStreamReceiverProtocol*> streams;
  mutex_.Lock();
  closing_ = true;
  for (QueryMap::const_iterator it = queries_.begin(); it != queries_.end(); ++it) {
    to_cancel.push_back(it->first);
    streams.push_back(it->second->protocol_);
  }
  to_wait_cancel_.clear();
  mutex_.Unlock();

  vector<http::ClientStreamReceiverProtocol*> streams_to_stop;
  for (int i = 0; i < to_cancel.size(); ++i) {
    if (!CancelRequestVerified(to_cancel[i])) {
      to_wait_cancel_.insert(to_cancel[i]);
      if (streams[i]) {
        streams_to_stop.push_back(streams[i]);
      }
    }
  }
  for (int i = 0; i < streams_to_stop.size(); ++i) {
    if (streams_to_stop[i]->connection()) {
      streams_to_stop[i]->connection()->ForceClose();
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
    "failsafe_client_: 0x%p [%s], "
    "http_request_path_: %s, "
    "auth_user_: %s}",
    failsafe_client_, server_name_.c_str(),
    http_request_path_.c_str(),
    auth_user_.c_str());
}

void HttpClient::GetClientStats(pb::ClientStats* stats) const {
  stats->set_now_ts(timer::TicksNsec());
  synch::SpinLocker l(&mutex_);
  for (QueryMap::const_iterator it = queries_.begin();
       it != queries_.end(); ++it) {
    stats->add_live_req()->CopyFrom(*(it->second->stats_));
  }
  for (size_t i = 0; i < completed_queries_.size(); ++i) {
    stats->add_completed_req()->CopyFrom(*completed_queries_[i]);
  }
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
  const int64 xid = GetNextXid();
  http::ClientRequest* req = new http::ClientRequest(http::METHOD_POST, path);
  if (req == NULL) {
    rpc_controller->SetErrorCode(rpc::ERROR_CLIENT);
    rpc_controller->SetFailed("Error in request allocation");
    if (done) done->Run();
    return;
  }
  req->set_request_id(xid);

  // write necessary parameters - headers
  for (int i = 0; i != request_headers_.size(); ++i) {
    req->request()->client_header()->AddField(request_headers_[i].first,
                                              request_headers_[i].second, true);
  }
  if ( !auth_user_.empty() ) {
    req->request()->client_header()->SetAuthorizationField(auth_user_, auth_pass_);
  }

  req->request()->set_server_use_gzip_encoding(rpc_controller->compress_transfer(), true);

  req->request()->client_header()->AddField(
    kRpcHttpXid, sizeof(kRpcHttpXid) - 1,
    strutil::StringPrintf("%" PRId64, xid), true, true);
  req->request()->client_header()->AddField(
    http::kHeaderContentType,
    sizeof(http::kHeaderContentType) - 1,
    kRpcContentType, sizeof(kRpcContentType) - 1, true, true);
  if (rpc_controller->is_streaming()) {
    req->request()->client_header()->AddField(
      kRpcHttpIsStreaming, sizeof(kRpcHttpIsStreaming) - 1,
      strutil::StringPrintf("%d", int(rpc_controller->is_streaming())),
      true, true);
    req->request()->client_header()->AddField(
      kRpcHttpHeartBeat, sizeof(kRpcHttpHeartBeat) - 1,
      strutil::StringPrintf(
        "%d", int(failsafe_client_->client_params()->read_timeout_ms_) / 2 / 1000),
        true, true);
  }

  //req->request()->client_header()->AddField(http::kHeaderContentEncoding,
  //                                          sizeof(http::kHeaderContentEncoding) - 1,
  //                                          kRpcGzipEncoding, sizeof(kRpcGzipEncoding) - 1,
  //                                          true, true);


  // write RPC message
  io::SerializeProto(request, req->request()->client_data());

  google::protobuf::Closure* const cancel_callback =
    google::protobuf::internal::NewCallback(
      this, &rpc::HttpClient::CallbackCancelRequested, xid);
  synch::Event* done_ev = NULL;
  if (done == NULL) {
    done_ev = new synch::Event(false, true);
  }
  QueryStruct* qs = new QueryStruct(
    req, xid, method, rpc_controller, request, response,
    done == NULL ?
    ::google::protobuf::internal::NewCallback(done_ev, &synch::Event::Signal)
    : done, cancel_callback, stats_msg_text_size_);

  mutex_.Lock();
  if (closing_) {
    rpc_controller->SetErrorCode(rpc::ERROR_CLIENT);
    rpc_controller->SetFailed("We are closing the client");
    mutex_.Unlock();
    delete qs;
    if (done) done->Run();
  } else {
    rpc_controller->NotifyOnCancel(cancel_callback);
    queries_.insert(make_pair(xid, qs));
    mutex_.Unlock();

    selector_->RunInSelectLoop(
      whisper::NewCallback(this, &rpc::HttpClient::StartRequest, qs));
    if (done_ev) done_ev->Wait();
  }
  delete done_ev;
}

////////////////////////////////////////////////////////////////////////////////

void HttpClient::StartRequest(HttpClient::QueryStruct* qs) {
  if (qs->cancelled_) {
    CallbackRequestDone(qs);
  } else {
    mutex_.Lock();
    string error;
    if (closing_) {
      error = "We are closing the client";
    } else {
      if (qs->controller_->is_streaming()) {
        qs->protocol_ = failsafe_client_->CreateStreamReceiveClient(-1);
        if (qs->protocol_ == NULL) {
          error = "Cannot allocate client protocol for streaming";
        } else {
          qs->started_ = true;
          qs->stream_callback_ = whisper::NewPermanentCallback(
            this, &rpc::HttpClient::CallbackRequestStream, qs);
          selector_->RunInSelectLoop(
            whisper::NewCallback(
              qs->protocol_,
              &http::ClientStreamReceiverProtocol::BeginStreamReceiving,
              qs->req_, qs->stream_callback_));
        }
      } else {
        qs->started_ = true;
        Closure* const req_done_callback = whisper::NewCallback(
          this, &rpc::HttpClient::CallbackRequestDone, qs);
        selector_->RunInSelectLoop(
          whisper::NewCallback(failsafe_client_,
                        &http::FailSafeClient::StartRequestWithUrgency,
                        qs->req_, req_done_callback,
                        qs->controller_->is_urgent()));
      }
    }
    if (!error.empty()) {
      google::protobuf::Closure* done = qs->done_;
      qs->controller_->SetErrorCode(rpc::ERROR_CLIENT);
      qs->controller_->SetFailed(error);
      mutex_.Unlock();
      delete qs;
      done->Run();
    } else {
      mutex_.Unlock();
    }
  }
}

void HttpClient::CallbackCancelRequested(int64 xid) {
  mutex_.Lock();
  if (closing_) return;
  mutex_.Unlock();
  Closure* cancel_callback = whisper::NewCallback(
    this, &rpc::HttpClient::CancelRequest, xid);
  selector_->RunInSelectLoop(cancel_callback);
}

HttpClient::QueryStruct::QueryStruct(http::ClientRequest* req,
                                     int64 xid,
                                     const google::protobuf::MethodDescriptor* method,
                                     rpc::Controller* controller,
                                     const google::protobuf::Message* request,
                                     google::protobuf::Message* response,
                                     google::protobuf::Closure* done,
                                     google::protobuf::Closure* cancel_callback,
                                     size_t limit_print)
  : req_(req),
    xid_(xid),
    method_(method),
    controller_(controller),
    response_(response),
    done_(done),
    cancel_callback_(cancel_callback),
    cancelled_(false),
    started_(false),
    protocol_(NULL),
    stream_callback_(NULL),
    next_message_size_(-1),
    stats_(new pb::RequestStats()) {
  stats_->set_peer_address(method->name());
  stats_->set_start_time_ts(timer::TicksNsec());
  stats_->set_method_txt(req->name() + strutil::StringPrintf(" / xid: %" PRId64, xid));
  if (controller->is_streaming()) {
    stats_->set_is_streaming(true);
  }
  stats_->set_request_type_name(request->GetDescriptor()->full_name());
  stats_->set_request_size(request->ByteSize());
  if (stats_->request_size() < limit_print) {
    stats_->set_request_txt(request->ShortDebugString());
  }
  stats_->set_response_type_name(response->GetDescriptor()->full_name());
}
HttpClient::QueryStruct::~QueryStruct() {
  delete req_;
  delete cancel_callback_;
  delete protocol_;
  delete stream_callback_;
  delete stats_;
}

pb::RequestStats* HttpClient::QueryStruct::GrabStats(size_t limit_print) {
  stats_->set_server_size(req_->request()->stats().server_size_ +
                          req_->request()->server_data()->Size());
  stats_->set_server_raw_size(req_->request()->stats().server_raw_size_);
  stats_->set_client_size(req_->request()->stats().client_size_);
  stats_->set_client_raw_size(req_->request()->stats().client_raw_size_);

  stats_->set_response_time_ts(timer::TicksNsec());
  stats_->set_response_size(response_->ByteSize());

  if (controller_->Failed()) {
    stats_->set_error_txt(controller_->ErrorText());
  } else if (stats_->response_size() < limit_print) {
    stats_->set_response_txt(response_->ShortDebugString());
  }
  pb::RequestStats* stats = stats_; stats_ = NULL;
  return stats;
}

void HttpClient::CancelRequest(int64 xid) {
  CancelRequestVerified(xid);   // call bool returning, discard the success
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
    if (qs->protocol_) {
      delete qs->protocol_;   // clears the request
      qs->protocol_ = NULL;
    } else if (failsafe_client_->CancelRequest(qs->req_)) {
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

void HttpClient::MaybeReadNextMessages(HttpClient::QueryStruct* qs) {
  io::MemoryStream* const in = qs->req_->request()->server_data();
  bool read_next = true;
  while (read_next) {
    if (qs->next_message_size_ <= 0) {
      if (in->Size() >= sizeof(int32)) {
        qs->next_message_size_ =
          io::BaseNumStreamer<io::MemoryStream, io::MemoryStream>::ReadInt32(
            in, common::BIGENDIAN);
      } else {
        read_next = false;
      }
    } else if (in->Size() >= qs->next_message_size_) {
      google::protobuf::Message* crt_message = qs->response_->New();
      if (crt_message == NULL) {
        LOG_WARN << "Error allocating new message of type: "
                  << qs->response_->GetTypeName();
      } else {
        const int32 init_size = in->Size();
        in->MarkerSet();
        if (!io::ParseProto(crt_message, in, qs->next_message_size_)) {
          delete crt_message; crt_message = NULL;
          LOG_WARN << "Parsing Response Error: req["
                    << ToString(qs) << "] size:"
                    << qs->next_message_size_;
        } else if (init_size - in->Size() != qs->next_message_size_) {
          LOG_WARN << "Size parsed does not match declared size: "
                    << init_size - in->Size()
                    << " vs. " << qs->next_message_size_;
          delete crt_message; crt_message = NULL;
        }
        in->MarkerRestore();
      }
      in->Skip(qs->next_message_size_);
      qs->controller_->PushStreamedMessage(crt_message);
      qs->next_message_size_ = 0;
    } else {
      read_next = false;
    }
  }
}

bool HttpClient::CallbackRequestStream(HttpClient::QueryStruct* qs) {
  DCHECK(selector_->IsInSelectThread());
  MaybeReadNextMessages(qs);
  if (qs->controller_->HasStreamedMessage()) {
    qs->done_->Run();
  }
  if (qs->req_->is_finalized()) {
    CallbackRequestDone(qs);   // this will be our end.
    return false;
  }
  return true;
}

void HttpClient::CallbackRequestDone(HttpClient::QueryStruct* qs) {
  DCHECK(selector_->IsInSelectThread());
  mutex_.Lock();
  // LOG_INFO
  //   << "Callback done request: " << qs->xid_ << " / " << ToString() << " / "
  //   << qs->cancelled_ << " // " << qs->req_->name() << " / queue size: " << queries_.size();
  CHECK(queries_.erase(qs->xid_));

  // From this moment the request cannot be canceled
  qs->controller_->NotifyOnCancel(NULL);
  delete qs->cancel_callback_;
  qs->cancel_callback_ = NULL;
  qs->controller_->set_is_finalized();

  mutex_.Unlock();
  bool delete_self = false;
  if ( !qs->cancelled_ ) {
    const http::ClientError cli_error = qs->req_->error();
    const http::HttpReturnCode ret_code =
        qs->req_->request()->server_header()->status_code();
    if ( cli_error != http::CONN_OK) {
      LOG_WARN << "Network Error: req[" << ToString(qs) << "] : "
                << http::ClientErrorName(cli_error);
      qs->controller_->SetErrorCode(rpc::ERROR_NETWORK);
      qs->controller_->SetFailed(std::string(http::ClientErrorName(cli_error)) +
                                 " / Http Client Error");
      // In this case the connection is recycled anyway.
    } else if (ret_code != http::OK ) {
      ErrorCode error_code = rpc::ERROR_SERVER;
      if (ret_code == http::UNKNOWN) {
        // This means a timeout mainly - go w/ proper response code
        error_code = rpc::ERROR_NETWORK;
      } else if (ret_code >= 400 && ret_code < 500) {
        error_code = rpc::ERROR_USER;
      }
      LOG_WARN << "Server Http Error: req[" << ToString(qs) << "]: "
                << http::GetHttpReturnCodeName(ret_code);
      qs->controller_->SetErrorCode(error_code);
      qs->controller_->SetFailed(qs->req_->request()->server_data()->ToString() +
                                 " / Http Server Error: " + http::GetHttpReturnCodeName(ret_code));
      // Ask for a connection reset - as things may be hung in a proxy on the way.
      if ( error_code != rpc::ERROR_NETWORK ) {
        failsafe_client_->Reset();
      }
    } else if (!qs->controller_->is_streaming()) {
      // wrap request buffer
      io::MemoryStream* const in = qs->req_->request()->server_data();
      in->MarkerSet(); // to be able to restore data on error & print
                       // it for debug
      if (!io::ParseProto(qs->response_, in)) {
        qs->controller_->SetErrorCode(rpc::ERROR_PARSE);
        in->MarkerRestore();
        LOG_WARN << "Parsing Response Error: req[" << ToString(qs) << "] size:"
                  << in->Size();
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
  mutex_.Lock();
  completed_queries_.push_front(qs->GrabStats(stats_msg_text_size_));
  while (completed_queries_.size() > stats_msg_history_size_) {
    delete completed_queries_.back();
    completed_queries_.pop_back();
  }
  mutex_.Unlock();

  qs->done_->Run();
  delete qs;

  if (delete_self) {
    DLOG_INFO << " Done Http Rpc client - deleting self: " << ToString();
    selector_->DeleteInSelectLoop(this);
  }
}
}  // namespace rpc
}  // namespace whisper
