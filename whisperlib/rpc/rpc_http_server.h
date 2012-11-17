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
//
// Protobuf adaptation:
//
// (c) Copyright 2011, 1618labs
// All rights reserved.
// Author: Catalin Popescu (cp@1618labs.com)
//
// TODO(cpopescu): implement cancel request - client -> server
//
#ifndef __NET_RPC_LIB_SERVER_RPC_HTTP_SERVER_H__
#define __NET_RPC_LIB_SERVER_RPC_HTTP_SERVER_H__

#include <map>
#include <string>
#include <whisperlib/base/types.h>
#include <whisperlib/sync/mutex.h>

#include WHISPER_HASH_SET_HEADER

#include <whisperlib/net/user_authenticator.h>

namespace google { namespace protobuf {
class Service;
class Message;
class MethodDescriptor;
} }
namespace http {
class Server;
class ServerRequest;
}
namespace net {
class IpClassifier;
}
namespace rpc {
class Controller;

//////////////////////////////////////////////////////////////////////

class HttpServer {
 public:
  // Starts serving RPC requests under the provided path
  // Makes the processing calls to be issued in the given select server
  // IMPORTANT: this rpc::HttpServer *must* live longer then the
  //            underlining http::Server
  //
  // We don't own any of the provided objects..
  HttpServer(http::Server* server,
             net::UserAuthenticator* authenticator,
             const string& path,
             bool is_public,
             int max_concurrent_requests,
             const string& ip_class_restriction);
  ~HttpServer();

  // Registers a service with this server (will be provided
  // under /..path../
  bool RegisterService(google::protobuf::Service* service);
  bool UnregisterService(google::protobuf::Service* service);

  // Registers a service with this name (will be provided
  // under /..path../..sub_path../
  // NOTE: sub_path should contain a final slash (if you wish !)
  bool RegisterService(const string& sub_path, google::protobuf::Service* service);
  bool UnregisterService(const string& sub_path, google::protobuf::Service* service);

  const string& path() const {
    return path_;
  }

protected:
  struct RpcData {
    RpcData(http::ServerRequest* req,
            rpc::Controller* controller,
            const google::protobuf::Message* request,
            google::protobuf::Message* response)
      : req_(req), controller_(controller), request_(request), response_(response) {
    }
    ~RpcData();
    http::ServerRequest* req_;
    rpc::Controller* controller_;
    const google::protobuf::Message* request_;
    google::protobuf::Message* response_;
  };


  // Actual request processing - callback on http calls
  void ProcessRequest(http::ServerRequest* req);

  // After the authentication completes, the processing is continued in
  // this function (that we force in req->net_selector();
  void ProcessAuthenticatedRequest(http::ServerRequest* req,
                                   google::protobuf::Service* service,
                                   const google::protobuf::MethodDescriptor* method,
                                   net::UserAuthenticator::Answer auth_answer);

  // In this function we state the actual processing (i.e. service->CallMethod)
  void StartProcessing(http::ServerRequest* req,
                       google::protobuf::Service* service,
                       const google::protobuf::MethodDescriptor* method,
                       google::protobuf::Message* request);

  // Callback from service->CallMethod
  void RpcCallback(RpcData* data);

  // Closes a request - schedules the reply in the request select server
  void ReplyToRequest(http::ServerRequest* req,
                      int status,
                      rpc::Controller* controller,   // can be null
                      const char* error_reason);     // can be null


  //////////////////////////////////////////////////////////////////////

  synch::Mutex mutex_;

  // If this is set empty we require for our users to be authenticated
  net::UserAuthenticator* const authenticator_;

  // If not null accept clients only from guys identified under this class
  net::IpClassifier* const accepted_clients_;

  // On which path we serve ?
  const string path_;

  // We do not accept more then these concurrent requests
  int max_concurrent_requests_;

  // Current statistics
  int current_requests_;

  // What services we provide ..
  typedef map<string, google::protobuf::Service*> ServicesMap;
  ServicesMap services_;

  http::Server* http_server_;

  // TODO(cpopescu): stats !
  //
  // - per each service / method -
  //    - num received (min etc)
  //    - processing time
  //
  // - stats per error case
  // - recent request browsing
  //
 private:
  DISALLOW_EVIL_CONSTRUCTORS(HttpServer);
};
}
#endif  // __NET_RPC_LIBS_SERVER_RPC_HTTP_SERVER_H__
