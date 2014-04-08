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
#ifndef __NET_HTTP_FAILSAFE_HTTP_CLIENT_H__
#define __NET_HTTP_FAILSAFE_HTTP_CLIENT_H__

#include <string>
#include <vector>
#include <deque>

#include <whisperlib/base/types.h>
#include <whisperlib/base/callback.h>
#include <whisperlib/http/http_client_protocol.h>
#include WHISPER_HASH_MAP_HEADER


namespace http {
//
// This is a class that accepts a set of servers to connect to and
// requests that can go to any of these servers.
// The requests will be distributed to the servers in a load-balancing
// fashion.
// In case of a failure the request is tried w/ another server until
// successful fetch is performed (ie. no network related trouble happend),
// or until the retry count goes to zero.
//
// This is not thread-safe - all call should be (and are) synchronized
// through the provided selector thread.
//
class FailSafeClient {
 public:
  FailSafeClient(
      net::Selector* selector,
      const ClientParams* client_params,
      const vector<net::HostPort>& servers,
      ResultClosure<BaseClientConnection*>* connection_factory,
      int num_retries,
      int32 request_timeout_ms,
      int32 reopen_connection_interval_ms,
      const string& force_host_header);
  ~FailSafeClient();

  const ClientParams* client_params() const {
    return client_params_;
  }
  const vector<net::HostPort>& servers() const {
    return servers_;
  }
  int num_retries() const {
    return num_retries_;
  }
  int64 request_timeout_ms() const {
    return request_timeout_ms_;
  }
  int64 reopen_connection_interval_ms() const {
    return reopen_connection_interval_ms_;
  }
  const string& force_host_header() const {
    return force_host_header_;
  }
  net::Selector* selector() const {
    return selector_;
  }
  void Reset() const;

  void StartRequest(ClientRequest* request,
                    Closure* completion_callback) {
      StartRequestWithUrgency(request, completion_callback, false);
  }

  void StartRequestWithUrgency(ClientRequest* request,
                               Closure* completion_callback,
                               bool is_urgent);

  // Cancels the provided request. We own the request from now on (as it may be
  // in download process), and we will delete it when done (together w/ the completion
  // callback if not permanent.
  bool CancelRequest(ClientRequest* request);

  void ScheduleStatusLog();

  // Force reopening of all connections
  void Reset();

  // Forces a close of all pending request - call this if you want
  // to cleanup your caller before this failsafe client gets deleted
  void ForceCloseAll();

 private:
  void WriteStatusLog() const;
  void StatusString(string* s) const;

  struct PendingStruct {
    int64 start_time_;
    int retries_left_;
    ClientRequest* req_;
    Closure* completion_closure_;
    bool canceled_;
    bool urgent_;
    PendingStruct(int64 start_time, int retries_left,
                  ClientRequest* req, Closure* completion_closure,
                  bool urgent)
        : start_time_(start_time),
          retries_left_(retries_left),
          req_(req),
          completion_closure_(completion_closure),
          canceled_(false),
          urgent_(urgent) {
    }
    string ToString(int64 crt_time) const;
  };

  void RequeuePendingAlarm();
  void RequeuePending();
  bool InternalStartRequest(PendingStruct* ps);
  void CompletionCallback(PendingStruct* ps);
  void DeleteCanceledPendingStruct(PendingStruct* ps);
  void CompleteWithError(PendingStruct* ps, http::ClientError error);

  static const int64 kRequeuePendingAlarmMs = 1000;

  net::Selector* const selector_;
  const ClientParams* client_params_;
  vector<net::HostPort> servers_;
  ResultClosure<BaseClientConnection*>* connection_factory_;
  const int num_retries_;
  const int64 request_timeout_ms_;
  const int64 reopen_connection_interval_ms_;
  const string force_host_header_;

  vector<ClientProtocol*> clients_;
  vector<int64> death_time_;

  typedef deque<PendingStruct*> PendingQueue;
  PendingQueue* pending_requests_;
  typedef hash_map<ClientRequest*, PendingStruct*> PendingMap;
  PendingMap* pending_map_;

  deque<pair<int64, string> > completion_events_;

  bool closing_;
  Closure* requeue_pending_callback_;
};
}

#endif
