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

#ifndef __WHISPERLIB_RPC_RPC_CLIENT_H__
#define __WHISPERLIB_RPC_RPC_CLIENT_H__

#include <map>
#include <string>
#include <vector>
#include <whisperlib/base/types.h>
#include <whisperlib/sync/mutex.h>
#include <google/protobuf/service.h>
#include WHISPER_HASH_SET_HEADER

namespace http {
class FailSafeClient;
class ClientRequest;
}
namespace net {
class Selector;
}

namespace rpc {
class Controller;

//
// This is a failsafe transport over HTTP connection from client side.
// You can use a set of rpc servers to call (i.e. uses the
// first that is available, and does load balancing) and it manages
// differently the reconnection and retries (is not on every send, but
// employs a timeout etc.)
// And you can cancel the lookup (your callback will never be called again
// on completion).
//
// Always create pointers to a rpc::HttpClient and call StartClose() to initiate
// a delete sequence (that completes in the associated selector, after all
// pending requests are cleared)
//
class HttpClient : public google::protobuf::RpcChannel {
 public:
  // Constructor w/ no extra parameters
  HttpClient(http::FailSafeClient* failsafe_client,
         const string& http_request_path);

  // Constructor that allows the user to set authorization cookies or user / pass
  // parameters.
  HttpClient(http::FailSafeClient* failsafe_client,
         const string& http_request_path,
         const vector< pair<string, string> >& request_headers,
         const string& auth_user,
         const string& auth_pass);
  // Don't call the destructor directly, instead call StartClose that will eventually delete the client
  virtual ~HttpClient();

  // Starte the close / delete of the client.
  void StartClose();

  // Returns a debug string for this client
  string ToString() const;

  // Main interface function - calls the proper method.
  // The controller *must* be of rpc::Controller type
  virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller,
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* done);


  //////////////////////////////////////////////////////////////////////
  //
  // Accessors:
  //
  const http::FailSafeClient* failsafe_client() const {
    return failsafe_client_;
  }
  const string& http_request_path() const {
    return http_request_path_;
  }
  const string& auth_user() const {
    return auth_user_;
  }
  const string& auth_pass() const {
    return auth_pass_;
  }

  net::Selector* selector() const {
    return selector_;
  }

 private:
  // We set this callback as cancel callback for the rpc controller this function.
  void CallbackCancelRequested(int64 xid);

  // Returns the next request id for this client - attached in the header - identifies
  // an rpc for debugging - not necessary unique among clients.
  int64 GetNextXid() {
    synch::MutexLocker l(&mutex_);
    return xid_++;
  }

  //////////////////////////////////////////////////////////////////////

  // Structure that wraps an RPC query request to the server
  struct QueryStruct {
    http::ClientRequest* const req_;
    const int64 xid_;

    const google::protobuf::MethodDescriptor* const method_;
    rpc::Controller* const controller_;
    google::protobuf::Message* const response_;
    google::protobuf::Closure* done_;
    google::protobuf::Closure* cancel_callback_;
    bool cancelled_;
    bool started_;

    QueryStruct(http::ClientRequest* req,
                int64 xid,
                const google::protobuf::MethodDescriptor* method,
                rpc::Controller* controller,
                google::protobuf::Message* response,
                google::protobuf::Closure* done,
                google::protobuf::Closure* cancel_callback)
      : req_(req),
        xid_(xid),
        method_(method),
        controller_(controller),
        response_(response),
        done_(done),
        cancel_callback_(cancel_callback),
        cancelled_(false),
        started_(false) {
    }
    ~QueryStruct();
  };

  // Debug string for a query
  string ToString(const QueryStruct* qs) const;

  // Actually starts the RPC from the select thread
  void StartRequest(QueryStruct* qs);

  // Cancels an RPC from the selector thread
  void CancelRequest(int64 xid);
  bool CancelRequestVerified(int64 xid);


  // Called by the selector after a http request receives full response
  // from the server. The request object is the same one you used
  // when launching the query (you probably want to delete it).
  void CallbackRequestDone(QueryStruct* qs);

  //////////////////////////////////////////////////////////////////////

  net::Selector* const selector_;
  http::FailSafeClient* const failsafe_client_;
  const string http_request_path_;
  const vector< pair<string, string> > request_headers_;
  const string auth_user_;
  const string auth_pass_;

  // Protects internal data:
  synch::Mutex mutex_;
  int64 xid_;      // next request id
  bool closing_;   // set to true after StartClose
  hash_set<int64> to_wait_cancel_;

  typedef map<int64, QueryStruct*> QueryMap;
  QueryMap queries_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(HttpClient);
};

}
#endif  // __WHISPERLIB_RPC_RPC_CLIENT_H__
