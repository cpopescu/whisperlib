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

#include "whisperlib/rpc/rpc_http_server.h"

#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include "whisperlib/base/log.h"
#include "whisperlib/base/core_errno.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/rpc/rpc_consts.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/io/buffer/protobuf_stream.h"
#include "whisperlib/io/ioutil.h"
#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/net/ipclassifier.h"
#include "whisperlib/rpc/RpcStats.pb.h"
#include "whisperlib/base/strutil.h"
#include <fstream>

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_SYSINFO_H
#include <sys/sysinfo.h>
#endif

using namespace std;

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

namespace whisper {
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
    stream_proto_error_close_(false),
    stats_msg_text_size_(2048),
    stats_msg_history_size_(200),
    num_current_requests_(0),
    http_server_(server) {
  http_server_->RegisterProcessor(path_,
                                  NewPermanentCallback(this, &HttpServer::ProcessRequest),
                                  is_public, true);
  http_server_->RegisterProcessor(path_ + "/rpcz",
                                  NewPermanentCallback(this, &HttpServer::ProcessRpcStatusRequest),
                                  is_public, true);
}

HttpServer::~HttpServer() {
  http_server_->UnregisterProcessor(path_);
  delete accepted_clients_;
  LOG_INFO_IF(num_current_requests_ > 0)
    << " Exiting the RPC server with current_requests_ "
    << "requests in processing !!";
  while (!completed_requests_.empty()) {
    delete completed_requests_.front();
    completed_requests_.pop_front();
  }
}

bool HttpServer::RegisterService(const string& sub_path,
                                 google::protobuf::Service* service) {
  synch::MutexLocker l(&mutex_);
  const string& full_name(service->GetDescriptor()->full_name());
  const string full_path(strutil::JoinPaths(sub_path, full_name));
  ServicesMap::const_iterator it = services_.find(full_path);
  if ( it != services_.end() ) {
    LOG_WARN << " Tried to double register " << full_path;
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
    LOG_WARN << " Tried to unregister " << full_path
                << " not found.";
    return false;
  }
  if ( service != it->second ) {
    LOG_WARN << " Different service registered under " << full_path
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

void HttpServer::ProcessRpcStatusRequest(http::ServerRequest* req) {
  pb::ServerStats stats;
  GetServerStats(&stats);
  io::MemoryStream* out =  req->request()->server_data();
  out->Write("<html><body><center>");
  out->Write(ServerStatsToHtml(stats));
  out->Write("\n</center></body></html>");
  req->ReplyWithStatus(http::OK);
}
void HttpServer::ProcessRequest(http::ServerRequest* req) {
  const string req_id =
    req->request()->client_header()->FindField(http::kHeaderXRequestId);
  if ( !req_id.empty() ) {
    req->request()->server_header()->AddField(
      http::kHeaderXRequestId, req_id, true);
  }
  net::HostPort peer_address(GetRemoteAddress(req));
  if (peer_address.IsInvalid()) {
    RegisterErrorRequest("Peer address cannot be determined", req, peer_address);
    ReplyToRequest(req, http::BAD_REQUEST, NULL, kRpcErrorBadProxyHeader);
    return;
  }
  if ( accepted_clients_ != NULL &&
       !accepted_clients_->IsInClass(peer_address.ip_object()) ) {
    RegisterErrorRequest("Client not accepted for ip address", req, peer_address);
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
  LOG_INFO_IF(http_server_->protocol_params().dlog_level_)
    << "peer: " << peer_address.ToString()
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
  size_t last_slash_index = sub_path.rfind('/');
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
    RegisterErrorRequest(service_full_path + " - unknown service.", req, peer_address);
    // LOG_ERROR << "Unknown service: " << service_full_path;
    // TODO(cpopescu): stats
    ReplyToRequest(req, http::NOT_FOUND, NULL, kRpcErrorServiceNotFound);
    return;
  }

  // maybe process auto-forms request -- TODO(cpopescu)
  const google::protobuf::MethodDescriptor* const method =
    service->GetDescriptor()->FindMethodByName(method_name);
  if (method == NULL) {
    RegisterErrorRequest(method_name + " - unknown method for service: " + service_full_path,
                         req, peer_address);
    // TODO(cpopescu): stats
    ReplyToRequest(req, http::NOT_FOUND, NULL, kRpcErrorMethodNotFound);
    return;
  }

  mutex_.Lock();
  ++num_current_requests_;
  if ( num_current_requests_ >= max_concurrent_requests_ ) {
    mutex_.Unlock();
    RegisterErrorRequest(strutil::StringPrintf(
                           "Too many concurrent requests: %d",
                           num_current_requests_), req, peer_address);
    // TODO(cpopescu): stats
    ReplyToRequest(req, http::INTERNAL_SERVER_ERROR, NULL, kRpcErrorServerOverloaded);
    return;
  }
  mutex_.Unlock();

  if (authenticator_ != NULL) {
    req->AuthenticateRequest(
      authenticator_,
      whisper::NewCallback(this, &HttpServer::ProcessAuthenticatedRequest,
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
      whisper::NewCallback(this, &HttpServer::ProcessAuthenticatedRequest,
                           req, service, method, auth_answer));
    return;
  }
  net::HostPort peer_address(GetRemoteAddress(req));
  if ( auth_answer != net::UserAuthenticator::Authenticated ) {
    RegisterErrorRequest(std::string("Unauthenticated request: ") +
                         net::UserAuthenticator::AnswerName(auth_answer), req, peer_address);
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
  } else if ( req->request()->client_header()->method() ==
              http::METHOD_OPTIONS ) {
    delete request;
    ReplyToRequest(req, http::OK, NULL, NULL);
    return;
  } else {
    error_reason = kRpcErrorMethodNotSupported;
  }
  if (error_reason != NULL) {
    std::string error_str(std::string("Bad request: http method: ")
                          + http::GetHttpMethodName(req->request()->client_header()->method())
                          + " service:  " + service->GetDescriptor()->full_name()
                          + " method: " + method->full_name()
                          + " reason: " + error_reason);
    RegisterErrorRequest(error_str, req, peer_address);
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
  size_t len;
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
    RegisterErrorRequest("Invalid peer address", req, peer_address);
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
  string is_streaming;
  int heart_beat_ms = kRpcDefaultHeartBeat * 1000;
  if ( req->request()->client_header()->FindField(kRpcHttpIsStreaming, &is_streaming) ) {
    controller->set_is_streaming(is_streaming == "1" || is_streaming == "true");
    string heart_beat;
    if (req->request()->client_header()->FindField(kRpcHttpHeartBeat,
                                                   &heart_beat) ) {
      errno = 0;
      int heart_beat_sec = strtol(heart_beat.c_str(), NULL, 10);
      if ( heart_beat_sec < kRpcMinHeartBeat ) {
        heart_beat_sec = kRpcMinHeartBeat;
      }
      heart_beat_ms = heart_beat_sec * 1000;
    }
  }

  // Any timeouts ?
  string timeout_str;
  if ( req->request()->client_header()->FindField("Timeout", &timeout_str) ) {
    errno = 0;   // essential as strtol would not set a 0 errno
    const int64 timeout_ms = strtoll(timeout_str.c_str(), NULL, 10);
    if ( timeout_ms > 0 ) {
      controller->set_timeout_ms(timeout_ms);
    }
  }

  if (!controller->custom_content_type().empty()) {
    req->request()->server_header()->AddField(http::kHeaderContentType,
                                              controller->custom_content_type(),
                                              true);
  } else {
    req->request()->server_header()->AddField(http::kHeaderContentType,
                                              "application/octet-stream",
                                              true);
  }
  req->request()->set_server_use_gzip_encoding(true, true);

  google::protobuf::Message* response = service->GetResponsePrototype(method).New();
  google::protobuf::Closure* done_callback = NULL;
  RpcData* const rpc_data = new RpcData(req, controller, request, response, peer_address,
                                        stats_msg_text_size_);
  mutex_.Lock();
  current_requests_.insert(rpc_data);
  mutex_.Unlock();

  if (controller->is_streaming()) {
    done_callback = ::google::protobuf::internal::NewPermanentCallback(
      this, &HttpServer::RpcStreamCallback, rpc_data);
    rpc_data->streaming_heartbeat_ms_ = heart_beat_ms;
  } else {
    done_callback = ::google::protobuf::internal::NewCallback(
      this, &HttpServer::RpcCallback, rpc_data);
  }
  service->CallMethod(method, controller, request, response, done_callback);
  if (controller->is_streaming() && !controller->IsFinalized()) {
    RpcStreamCallback(rpc_data);  // maybe start the header and so..
  }
}

////////////////////////////////////////////////////////////////////

static const size_t kRpcMinBufferSpace = 32000;

// Rpc callback for streams point to this -> called by the implementation whenever
// it wants to stream some data / report an error.
void HttpServer::RpcStreamCallback(RpcData* data) {
  if (!data->req_->net_selector()->IsInSelectThread()) {
    data->req_->net_selector()->RunInSelectLoop(
      whisper::NewCallback(this, &HttpServer::RpcStreamCallback, data));
    return;
  }

  data->streaming_mutex_->Lock();
  if (!data->streaming_sent_response_) {
    if (data->controller_->Failed()) {
      data->streaming_mutex_->Unlock();
      ReplyToRequest(data->req_, http::INTERNAL_SERVER_ERROR,
                     data->controller_, NULL);
      RpcStreamClosed(data);
      return;
    }
    PrepareForResponse(data->req_, data->controller_, NULL);
    data->streaming_req_close_callback_ = whisper::NewCallback(
      this, &HttpServer::RpcStreamClosed, data);
    data->streaming_heartbeat_callback_ = whisper::NewPermanentCallback(
      this, &HttpServer::RpcStreamHeartbeat, data);
    data->streaming_sent_response_ = true;
    data->streaming_mutex_->Unlock();
    data->req_->BeginStreamingData(
      http::OK, data->streaming_req_close_callback_, true);
    data->req_->net_selector()->RegisterAlarm(
      data->streaming_heartbeat_callback_, data->streaming_heartbeat_ms_);
  } else {
    data->streaming_mutex_->Unlock();
  }
  RpcStreamSomeData(data);
}

void HttpServer::RpcStreamHeartbeat(RpcData* data) {
  if (data->req_->is_orphaned() ||
      data->controller_->Failed() ||
      data->controller_->IsCanceled()) {
    return;  // they'll close it ..
  }
  data->req_->net_selector()->RegisterAlarm(
    data->streaming_heartbeat_callback_, data->streaming_heartbeat_ms_);
  size_t size = data->req_->free_output_bytes();
  if (size > sizeof(int32) && data->streaming_message_->IsEmpty()) {
    size -= io::BaseNumStreamer<io::MemoryStream, io::MemoryStream>::WriteInt32(
      data->req_->request()->server_data(), 0, common::BIGENDIAN);
    RpcStreamContinue(data);
  }
}

// Puts some data from the wire, detects the end of stream,
// flushes the protocols etc.
//
// NOTE: please observe the fine locking
//
void HttpServer::RpcStreamSomeData(RpcData* data) {
  data->streaming_mutex_->Lock();
  // There was some error encountered ?
  if (data->req_->is_orphaned() ||
      data->controller_->Failed() ||
      data->controller_->IsCanceled()) {
    data->streaming_mutex_->Unlock();
    data->controller_->set_is_finalized();
    data->req_->EndStreamingData();   // Streaming Closed gets called..
    return;
  }

  // Try to pull some data to be sent
  size_t size = data->req_->free_output_bytes();   // less than this on wire
  const size_t start_size = size;
  bool popped = false;
  do {
    popped = RpcStreamPopMessageDataLocked(data);
    if (!data->streaming_message_->IsEmpty()) {
      // Write out the size (then the
      if (!data->streaming_message_sent_size_) {
        if (size > sizeof(int32)) {
          size -= io::BaseNumStreamer<io::MemoryStream, io::MemoryStream>
            ::WriteInt32(data->req_->request()->server_data(),
                         data->streaming_message_->Size(),
                         common::BIGENDIAN);
          data->streaming_message_sent_size_ = true;
        } else {
          break;  // need to wait for some space in the buffer
        }
      }
      // Write some data if size allows.
      if (size > 0) {
        const size_t cb = std::min(size, data->streaming_message_->Size());
        data->req_->request()->server_data()->AppendStream(
          data->streaming_message_, cb);
        size -=cb;
      }
    }
  } while (size > 0 && popped);
  {
    data->streaming_mutex_->Lock();
    data->stats_->set_streamed_size(data->stats_->streamed_size() + start_size - size);
    data->streaming_mutex_->Unlock();
  }

  // Implicit end of stream -
  // no streaming_callback, no message, nothing to be sent.
  if (data->controller_->server_streaming_callback() == NULL &&
      data->streaming_message_->IsEmpty() &&
      !data->controller_->HasStreamedMessage()) {
    data->stream_ended_ = true;
  }
  if (data->streaming_message_->IsEmpty()) {
    if (data->stream_ended_) {
      data->streaming_mutex_->Unlock();
      data->controller_->set_is_finalized();
      data->req_->EndStreamingData();
      // Streaming Closed gets called - we are done
      return;   // end of stream
    }
  }
  if (size != start_size || !data->req_->request()->server_data()->IsEmpty()) {
    data->streaming_mutex_->Unlock();
    RpcStreamContinue(data);
    data->streaming_mutex_->Lock();
  }
  if (!data->stream_ended_ && !data->streaming_scheduled_
      && !data->controller_->HasStreamedMessage()
      && data->controller_->server_streaming_callback() != NULL) {
    data->streaming_scheduled_ = true;
    data->streaming_mutex_->Unlock();
    data->req_->net_selector()->RunInSelectLoop(
      whisper::NewCallback(data, &RpcData::RunStreamingCallback));
  } else {
    data->streaming_mutex_->Unlock();
  }
}

void HttpServer::RpcData::RunStreamingCallback() {
  streaming_scheduled_ = false;
  if (controller_->server_streaming_callback()) {
    controller_->server_streaming_callback()->Run();
  }
}

void HttpServer::RpcStreamContinue(RpcData* data) {
  data->req_->set_ready_callback(
    whisper::NewCallback(this, &HttpServer::RpcStreamSomeData, data),
    kRpcMinBufferSpace);
  data->req_->ContinueStreamingData();
}

void HttpServer::RpcStreamClosed(RpcData* data) {
  net::Selector* net_selector = data->req_->net_selector();
  if (data->streaming_heartbeat_callback_) {
    net_selector->UnregisterAlarm(
      data->streaming_heartbeat_callback_);
  }
  data->streaming_req_close_callback_ = NULL;
  data->controller_->FinalizeStreamingOnNetworkError();

  RpcCompleteData(data, net_selector);
}

void HttpServer::RpcCompleteData(RpcData* data,
                                 net::Selector* net_selector) {
  synch::MutexLocker l(&mutex_);
  --num_current_requests_;
  current_requests_.erase(data);
  completed_requests_.push_front(data->GrabStats(stats_msg_text_size_));
  while (completed_requests_.size() > stats_msg_history_size_) {
    delete completed_requests_.back();
    completed_requests_.pop_back();
  }

  net_selector->DeleteInSelectLoop(data);
}


bool HttpServer::RpcStreamPopMessageDataLocked(RpcData* data) {
  // DCHECK(data->streaming_mutex_->IsHeld());
  bool popped = false;
  while (data->streaming_message_->IsEmpty() &&
         data->controller_->HasStreamedMessage() &&
         !data->stream_ended_) {
    data->streaming_message_sent_size_ = false;
    pair<google::protobuf::Message*, bool> msg =
      data->controller_->PopStreamedMessage();
    DCHECK(msg.second);  // have to have something in there
    if (!msg.first) {
      data->stream_ended_ = true;
    } else {
      data->streaming_message_->MarkerSet();
      if (!io::SerializeProto(msg.first, data->streaming_message_)) {
        data->streaming_message_->MarkerRestore();
        LOG_ERROR << " Error serializing rpc streamed message. "
                  << (stream_proto_error_close_ ? "closing" : "skipping")
                  << " uninitialized: " << msg.first->InitializationErrorString();
        if (stream_proto_error_close_) {
          data->stream_ended_ = true;
        } // else - will just continue
      } else {
        data->streaming_message_->MarkerClear();
        popped = true;
      }
      delete msg.first;
    }
  }
  if (!popped && !data->stream_ended_ && !data->streaming_message_->IsEmpty()) {
    popped = true;
  }
  return popped;
}


void HttpServer::RpcCallback(RpcData* data) {
  data->controller_->set_is_finalized();
  // Keep a pointer on the selector, as the 'req_'
  // gets automatically deleted by ReplyToRequest()
  net::Selector* net_selector = data->req_->net_selector();
  if (data->controller_->Failed()) {
    if (data->controller_->GetErrorCode() == ERROR_USER) {
      ReplyToRequest(data->req_, http::BAD_REQUEST,
                     data->controller_, data->controller_->ErrorText().c_str());
    } else {
      ReplyToRequest(data->req_, http::INTERNAL_SERVER_ERROR,
                     data->controller_, data->controller_->ErrorText().c_str());
    }
  } else if (!io::SerializeProto(data->response_,
                                 data->req_->request()->server_data())) {
    ReplyToRequest(data->req_, http::INTERNAL_SERVER_ERROR, NULL,
                   kRpcErrorSerializingResponse);
  } else {
    ReplyToRequest(data->req_, http::OK, NULL, NULL);
  }
  RpcCompleteData(data, net_selector);
}

void HttpServer::PrepareForResponse(http::ServerRequest* req,
                                    rpc::Controller* controller,
                                    const char* error_reason) {
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
    } else if (!controller->custom_content_type().empty()) {
      req->request()->server_header()->AddField(
        http::kHeaderContentType, sizeof(http::kHeaderContentType) - 1,
        controller->custom_content_type(), true, true);
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
  }
  req->request()->server_header()->AddField(
    "Access-Control-Allow-Origin", "*", true, true);
}

void HttpServer::SendResponse(http::ServerRequest* req,
                              int status) {
  if (req->net_selector()->IsInSelectThread()) {
    req->ReplyWithStatus(http::HttpReturnCode(status));
  } else {
    req->net_selector()->RunInSelectLoop(
      whisper::NewCallback(req, &http::ServerRequest::ReplyWithStatus,
                           http::HttpReturnCode(status)));
  }
}

void HttpServer::GetServerStats(pb::ServerStats* stats) const {
  stats->set_now_ts(timer::TicksNsec());
  GetMachineStats(stats->mutable_machine_stats());

  synch::MutexLocker l(&mutex_);
  for (std::set<RpcData*>::const_iterator it = current_requests_.begin();
       it != current_requests_.end(); ++it) {
    if ((*it)->stats_) stats->add_live_req()->CopyFrom(*(*it)->stats_);
  }
  for (size_t i = 0; i < completed_requests_.size(); ++i) {
    stats->add_completed_req()->CopyFrom(*completed_requests_[i]);
  }
}

void HttpServer::GetMachineStats(pb::MachineStats* stats) const {
  stats->set_server_name(http_server()->name());
  stats->set_path(path_);
  if (authenticator_) stats->set_auth_realm(authenticator_->realm());
#ifdef HAVE_SYS_SYSINFO_H
  struct sysinfo info;
  if (!sysinfo(&info)) {
    pb::SystemStatus* status = stats->mutable_system_status();
    status->set_uptime(info.uptime);
    status->add_loads(info.loads[0]);
    status->add_loads(info.loads[1]);
    status->add_loads(info.loads[2]);
    status->set_freeram(info.freeram);
    status->set_sharedram(info.sharedram);
    status->set_bufferram(info.bufferram);
    status->set_totalswap(info.totalswap);
    status->set_freeswap(info.freeswap);
    status->set_procs(info.procs);
    status->set_totalhigh(info.totalhigh);
    status->set_freehigh(info.freehigh);
    status->set_mem_unit(info.mem_unit);
  }
#endif
#ifdef HAVE_SYS_STATVFS_H
  if (io::IsReadableFile("/proc/mounts")) {
  std::ifstream mount_info("/proc/mounts");
  std::set<std::string> dests;
  while( !mount_info.eof() ) {
    std::string device, destination;
    mount_info >> device >> destination;
    if (device.empty()) {
      break;
    }
    if(!strutil::StrStartsWith(device, "/") ||
       !strutil::StrStartsWith(destination, "/") ||
       dests.count(destination)) {
      continue;
    }
    dests.insert(destination);
    struct statvfs buf;
    const int err = statvfs(destination.c_str(), &buf);
    pb::DiskStatus* status = stats->add_disk_status();
    status->set_path(destination);
    if (err == 0) {
      status->set_free_bytes(buf.f_bsize * buf.f_bavail);
      status->set_free_bytes_root(buf.f_bsize * buf.f_bfree);
      status->set_free_inodes(buf.f_favail);
      status->set_free_inodes_root(buf.f_ffree);
      status->set_total_disk_bytes(buf.f_bsize * buf.f_blocks);
    } else {
      stats->mutable_disk_status()->RemoveLast();
    }
  }
  }
#endif
}


void HttpServer::RegisterErrorRequest(const std::string& reason,
                                      http::ServerRequest* req,
                                      const net::HostPort& peer_address) {
  LOG_WARN << " Error with RPC request: " << reason;
  pb::RequestStats* stats = BuildErrorStats(req, peer_address, reason);

  synch::MutexLocker l(&mutex_);
  --num_current_requests_;
  completed_requests_.push_front(stats);
  while (completed_requests_.size() > stats_msg_history_size_) {
    delete completed_requests_.back();
    completed_requests_.pop_back();
  }
}

////////////////////////////////////////////////////////////////////

void HttpServer::ReplyToRequest(http::ServerRequest* req,
                                int status,
                                rpc::Controller* controller,
                                const char* error_reason) {
  PrepareForResponse(req, controller, error_reason);
  SendResponse(req, status);
}

pb::RequestStats* HttpServer::BuildErrorStats(http::ServerRequest* req,
                                              const net::HostPort& remote_address,
                                              const std::string& error) {
  pb::RequestStats* stats = new pb::RequestStats();
  stats->set_peer_address(remote_address.ToString());
  stats->set_start_time_ts(timer::TicksNsec());
  stats->set_response_time_ts(stats->start_time_ts());
  stats->set_method_txt(req->request()->url()->path());
  stats->set_request_txt(req->ToString());
  stats->set_error_txt(error);
  stats->set_server_size(req->request()->stats().server_size_);
  stats->set_server_raw_size(req->request()->stats().server_raw_size_);
  stats->set_client_size(req->request()->stats().client_size_);
  stats->set_client_raw_size(req->request()->stats().client_raw_size_);
  return stats;
}

HttpServer::RpcData::RpcData(http::ServerRequest* req,
                             rpc::Controller* controller,
                             const google::protobuf::Message* request,
                             google::protobuf::Message* response,
                             const net::HostPort& remote_address,
                             size_t limit_print)
  : req_(req), controller_(controller), request_(request), response_(response),
    xid_(req->request()->client_header()->FindField(kRpcHttpXid)),
    streaming_mutex_(controller_->is_streaming() ? new synch::Spin() : NULL),
    streaming_scheduled_(false),
  streaming_sent_response_(false),
  streaming_message_sent_size_(false),
  stream_ended_(false),
  streaming_heartbeat_ms_(0),
  streaming_message_(controller_->is_streaming() ? new io::MemoryStream() : NULL),
  streaming_req_close_callback_(NULL),
  streaming_heartbeat_callback_(NULL),
  stats_(new pb::RequestStats()) {
  stats_->set_peer_address(remote_address.ToString());
  stats_->set_start_time_ts(timer::TicksNsec());
  stats_->set_method_txt(req->request()->url()->path() + " xid:" + xid_);
  if (controller->is_streaming()) {
    stats_->set_is_streaming(true);
  }
  stats_->set_request_type_name(request->GetDescriptor()->full_name());
  stats_->set_request_size(request->ByteSize());
  if (size_t(stats_->request_size()) < limit_print) {
    stats_->set_request_txt(request->ShortDebugString());
  }
  stats_->set_response_type_name(response->GetDescriptor()->full_name());
}

pb::RequestStats* HttpServer::RpcData::GrabStats(size_t limit_print) {
  stats_->set_server_size(req_->request()->stats().server_size_ +
                          req_->request()->server_data()->Size());
  stats_->set_server_raw_size(req_->request()->stats().server_raw_size_);
  stats_->set_client_size(req_->request()->stats().client_size_);
  stats_->set_client_raw_size(req_->request()->stats().client_raw_size_);

  stats_->set_response_time_ts(timer::TicksNsec());
  stats_->set_response_size(response_->ByteSize());

  if (controller_->Failed()) {
    stats_->set_error_txt(controller_->ErrorText());
  } else if (size_t(stats_->response_size()) < limit_print) {
    stats_->set_response_txt(response_->ShortDebugString());
  }
  pb::RequestStats* stats = stats_; stats_ = NULL;
  return stats;
}

HttpServer::RpcData::~RpcData() {
  delete streaming_heartbeat_callback_;
  delete controller_;
  delete request_;
  delete response_;
  delete streaming_message_;
  delete streaming_mutex_;
  delete stats_;
}

}  // namespace rpc
}  // namespace whisper
