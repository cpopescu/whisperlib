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

#include <whisperlib/http/failsafe_http_client.h>

#define LOG_HTTP                                                      \
    LOG_INFO_IF(client_params_->dlog_level_) << " - HTTP Failsafe: "

namespace http {

FailSafeClient::FailSafeClient(
    net::Selector* selector,
    const ClientParams* client_params,
    const vector<net::HostPort>& servers,
    ResultClosure<BaseClientConnection*>* connection_factory,
    int num_retries,
    int32 request_timeout_ms,
    int32 reopen_connection_interval_ms,
    const string& force_host_header)
    : selector_(selector),
      client_params_(client_params),
      servers_(servers),
      connection_factory_(connection_factory),
      num_retries_(num_retries),
      request_timeout_ms_(request_timeout_ms),
      reopen_connection_interval_ms_(reopen_connection_interval_ms),
      force_host_header_(force_host_header),
      pending_requests_(new PendingQueue),
      pending_map_(new PendingMap()),
      closing_(false) {
  CHECK(connection_factory_->is_permanent());
  CHECK(!servers_.empty());
  for ( int i = 0; i < servers_.size(); ++i ) {
    clients_.push_back(new ClientProtocol(
                           client_params,
                           connection_factory->Run(),
                           servers_[i]));
    death_time_.push_back(0);
  }
  requeue_pending_callback_ = NewPermanentCallback(
      this, &FailSafeClient::RequeuePendingAlarm);
  selector_->RunInSelectLoop(
      NewCallback(selector, &net::Selector::RegisterAlarm,
                  requeue_pending_callback_, kRequeuePendingAlarmMs));
}

FailSafeClient::~FailSafeClient() {
  closing_ = true;
  for ( int i = 0; i < clients_.size(); ++i ) {
    if ( clients_[i] != NULL ) {
      clients_[i]->ResolveAllRequestsWithError();
      delete clients_[i];
    }
  }
  while ( !pending_requests_->empty() ) {
    PendingStruct* const ps = pending_requests_->front();
    if (!ps->canceled_) {
      CompleteWithError(ps, CONN_CLIENT_CLOSE);
    } else {
      DeleteCanceledPendingStruct(ps);
    }
    pending_requests_->pop_front();
  }
  delete pending_requests_;
  delete pending_map_;
  selector_->UnregisterAlarm(requeue_pending_callback_);
  delete requeue_pending_callback_;
}

void FailSafeClient::StartRequestWithUrgency(ClientRequest* request,
                                             Closure* completion_callback,
                                             bool urgent) {
  DCHECK(pending_map_->find(request) == pending_map_->end());
  LOG_HTTP << " Starting request: " << request->name();
  PendingStruct* const ps = new PendingStruct(
      selector_->now(), num_retries_,
      request, completion_callback, urgent);
  if ( InternalStartRequest(ps) ) {
    RequeuePending();
  }
}

bool FailSafeClient::CancelRequest(ClientRequest* request) {
  CHECK(selector_->IsInSelectThread());
  const PendingMap::iterator it = pending_map_->find(request);
  if ( it == pending_map_->end() ) {
      return false;
  }
  it->second->canceled_ = true;
  pending_map_->erase(request);
  return true;
}

////////////////////

void FailSafeClient::RequeuePendingAlarm() {
  RequeuePending();
  selector_->RegisterAlarm(requeue_pending_callback_, kRequeuePendingAlarmMs);
}

void FailSafeClient::RequeuePending() {
  if ( pending_requests_->empty() ) {
      CHECK(pending_map_->empty());
      return;
  }
  PendingQueue* const old_queue = pending_requests_;
  pending_requests_ = new PendingQueue();

  while ( !old_queue->empty() ) {
      PendingStruct* const first = old_queue->front();
      old_queue->pop_front();
      if (pending_map_->find(first->req_) == pending_map_->end()) {
          continue;  // leftover
      }
      LOG_HTTP << " Requeueing: " << first->req_;
      if ( first->canceled_ ) {
        DeleteCanceledPendingStruct(first);
        continue;
      }
      pending_map_->erase(first->req_);
      if ( !InternalStartRequest(first) ) {
        break;
      }
  }

  // Whatever is left in old_queue did not fit - need to push to the
  // new pending_requests_
  while ( !old_queue->empty() ) {
      PendingStruct* const ps = old_queue->front();
      old_queue->pop_front();
      if (pending_map_->find(ps->req_) == pending_map_->end()) {
          continue;  // leftover
      }
      if ( ps->canceled_ ) {
          DeleteCanceledPendingStruct(ps);
      } else if ( selector_->now() - ps->start_time_ > request_timeout_ms_ ) {
          pending_map_->erase(ps->req_);
          CompleteWithError(ps, CONN_REQUEST_TIMEOUT);
      } else {
          // In this case we keep the ordering
          pending_requests_->push_back(ps);
      }
  }
  delete old_queue;
}

void FailSafeClient::CompleteWithError(PendingStruct* ps,
                                       http::ClientError error) {
  ps->req_->set_error(error);
  Closure* completion_closure = ps->completion_closure_;
  delete ps;
  selector_->RunInSelectLoop(completion_closure);
}


bool FailSafeClient::InternalStartRequest(PendingStruct* ps) {
  if ( selector_->now() - ps->start_time_ > request_timeout_ms_ ) {
    CompleteWithError(ps, CONN_REQUEST_TIMEOUT);
    return true;
  }
  if ( ps->retries_left_ <= 0 ) {
    CompleteWithError(ps, CONN_TOO_MANY_RETRIES);
    return true;
  }
  int min_id = -1;
  int min_load = 0;
  for ( int i = 0; i < clients_.size(); ++i ) {
    if ( clients_[i] != NULL && clients_[i]->IsAlive() ) {
      const int32 load = (clients_[i]->num_active_requests() +
                          clients_[i]->num_waiting_requests());
      if ( load < min_load || min_id < 0 ) {
        min_load = load;
        min_id = i;
      }
    } else {
      if ( clients_[i] != NULL ) {
          ClientProtocol* clients = clients_[i];
          clients_[i] = NULL;
          clients->ResolveAllRequestsWithError();
          delete clients;
      }
      if ( selector_->now() - death_time_[i] >
           reopen_connection_interval_ms_ && death_time_[i] > 0) {
          LOG_INFO << " Reopening client for: " << servers_[i].ToString()
                   << " // " << selector_->now() << " // " << death_time_[i]
                   << " @" << reopen_connection_interval_ms_;
          clients_[i] = new ClientProtocol(client_params_,
                                           connection_factory_->Run(),
                                           servers_[i]);
          death_time_[i] = selector_->now();
      } else if (death_time_[i] == 0) {
          death_time_[i] = selector_->now();
      }
    }
  }
  if ( min_id < 0 ) {
    pending_map_->insert(make_pair(ps->req_, ps));
    if (!ps->urgent_) {
      pending_requests_->push_back(ps);
    } else {
      pending_requests_->push_front(ps);
    }
    return false;
  }
  ps->retries_left_--;
  LOG_HTTP << " Sending request: " << ps->req_
           << " to " << servers_[min_id].ToString();

  bool host_added = false;
  if ( !force_host_header_.empty() ) {
    host_added = true;
    ps->req_->request()->client_header()->AddField(
        "Host", force_host_header_.c_str(), true, true);
  }

  ps->req_->request()->client_data()->MarkerSet();
  clients_[min_id]->SendRequest(
      ps->req_,
      NewCallback(this, &FailSafeClient::CompletionCallback, ps));
  return true;
}

void FailSafeClient::CompletionCallback(PendingStruct* ps) {
  CHECK(ps->req_->is_finalized());
  if ( ps->canceled_ ) {
    ps->req_->request()->client_data()->MarkerRestore();
    RequeuePending();
  } else if ( closing_ || ps->req_->error() == CONN_OK ) {
    pending_map_->erase(ps->req_);
    RequeuePending();
    ps->req_->request()->client_data()->MarkerClear();

    Closure* completion_closure = ps->completion_closure_;
    delete ps;
    selector_->RunInSelectLoop(completion_closure);
  } else {
    ps->req_->request()->client_data()->MarkerRestore();
    ps->req_->request()->server_data()->Clear();
    ps->req_->request()->server_header()->Clear();
    LOG_INFO << servers_[0] << " / w/" << servers_.size()
             << " Failsafe retrying request: " << ps->req_->name();
    InternalStartRequest(ps);
  }
}

void FailSafeClient::DeleteCanceledPendingStruct(PendingStruct* ps) {
  if (ps->completion_closure_ != NULL &&
      !ps->completion_closure_->is_permanent()) {
    delete ps->completion_closure_;
  }
  pending_map_->erase(ps->req_);
  delete ps->req_;
  delete ps;
}

}
