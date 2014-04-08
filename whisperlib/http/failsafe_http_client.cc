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
#include <whisperlib/base/gflags.h>

#define LOG_HTTP                                                        \
    LOG_INFO_IF(client_params_->dlog_level_) << " - HTTP Failsafe: "

DEFINE_int32(http_failsafe_max_log_size, 100,
             "Keep a history of at most this size");

namespace http {

FailSafeClient::FailSafeClient(
    net::Selector* selector,
    const ClientParams* client_params,
    const vector<net::HostPort>& servers,
    ResultClosure<BaseClientConnection*>* connection_factory,
    bool auto_delete_connection_factory,
    int num_retries,
    int32 request_timeout_ms,
    int32 reopen_connection_interval_ms,
    const string& force_host_header)
    : selector_(selector),
      client_params_(client_params),
      servers_(servers),
      connection_factory_(connection_factory),
      auto_delete_connection_factory_(auto_delete_connection_factory),
      num_retries_(num_retries),
      request_timeout_ms_(request_timeout_ms),
      reopen_connection_interval_ms_(reopen_connection_interval_ms),
      force_host_header_(force_host_header),
      pending_requests_(new PendingQueue),
      pending_map_(new PendingMap()),
      closing_(false) {
  CHECK(connection_factory_->is_permanent());
  CHECK(!servers_.empty());

  requeue_pending_callback_ = NewPermanentCallback(
      this, &FailSafeClient::RequeuePendingAlarm);
  selector_->RunInSelectLoop(
      NewCallback(selector, &net::Selector::RegisterAlarm,
                  requeue_pending_callback_, kRequeuePendingAlarmMs));
}

FailSafeClient::~FailSafeClient() {
  LOG_INFO << " Deleting failsafe connection: " << this;
  closing_ = true;
  ForceCloseAll();
  LOG_INFO << " Deleting pending requests " << pending_requests_->size();
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
  if ( auto_delete_connection_factory_ ) {
      delete connection_factory_;
  }
  LOG_INFO << " Failsafe connection deleted:" << this;
}

void FailSafeClient::ForceCloseAll() {
  bool was_closing = closing_;
  closing_ = true;
  for ( int i = 0; i < clients_.size(); ++i ) {
    if ( clients_[i] != NULL ) {
      clients_[i]->ResolveAllRequestsWithError();
      delete clients_[i];
    }
  }
  death_time_.clear();
  clients_.clear();
  closing_ = was_closing;
}

void FailSafeClient::StartRequestWithUrgency(ClientRequest* request,
                                             Closure* completion_callback,
                                             bool urgent) {
  DCHECK(pending_map_->find(request) == pending_map_->end());
  LOG_HTTP << " Queuing request: " << request->name();
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
  LOG_INFO << this << " Canceled request: " << it->first;
  it->second->canceled_ = true;
  it->second->req_ = NULL;  // This is got at this point, should not
                            //  be used anymore (caller takes care)
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
      LOG_HTTP << " Requeueing: " << first->req_->name();
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
  const int64 now = selector_->now();
  completion_events_.push_back(make_pair(now, ps->ToString(now) + " => ERROR"));
  while (completion_events_.size() > FLAGS_http_failsafe_max_log_size) {
      completion_events_.pop_front();
  }
  Closure* completion_closure = ps->completion_closure_;
  delete ps;
  selector_->RunInSelectLoop(completion_closure);
}


bool FailSafeClient::InternalStartRequest(PendingStruct* ps) {
  if ( closing_ ) {
    return false;
  }
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
  while (min_id < 0) {
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
          min_load = 0; min_id = i;
        } else if (death_time_[i] == 0) {
            death_time_[i] = selector_->now();
        }
      }
    }
    if ( min_id < 0 || min_load > 0) {
      if (clients_.size() < servers_.size() ) {
        min_id = clients_.size();
        min_load = 0;
        clients_.push_back(new ClientProtocol(
                               client_params_, connection_factory_->Run(),
                           servers_[clients_.size()]));
        death_time_.push_back(0);
      } else {
          break;
      }
    }
  }

  if ( min_id < 0 || min_load > 0 ) {
//       (min_load > 0 && client_params_->keep_alive_sec_ == 0) ) {
    pending_map_->insert(make_pair(ps->req_, ps));
    if (!ps->urgent_) {
      pending_requests_->push_back(ps);
    } else {
      pending_requests_->push_front(ps);
    }
    return false;
  }
  ps->retries_left_--;
  LOG_HTTP << " Sending request: " << ps->req_->name()
           << " to " << servers_[min_id].ToString() << " id: " << min_id
           << " load: " << min_load;
  const int64 now = selector_->now();
  completion_events_.push_back(
      make_pair(now, ps->ToString(now) +
                strutil::StringPrintf(" => START [%d]", min_id)));

  bool host_added = false;
  if ( !force_host_header_.empty() ) {
    host_added = true;
    ps->req_->request()->client_header()->AddField(
        "Host", force_host_header_.c_str(), true, true);
  }

  ps->req_->request()->client_data()->MarkerSet();
  clients_[min_id]->SendRequest(
      ps->req_,
      NewCallback(this, &FailSafeClient::CompletionCallback, ps, min_id));
  return true;
}

void FailSafeClient::CompletionCallback(PendingStruct* ps, int client_id) {
  // Is possible to be reset
  // CHECK(ps->req_->is_finalized());
  const int64 now = selector_->now();
  if ( ps->canceled_ ) {
    ps->req_->request()->client_data()->MarkerRestore();
    RequeuePending();
    completion_events_.push_back(make_pair(now, ps->ToString(now) + " => canceled"));
  } else if ( ps->req_->is_finalized() &&
              ps->req_->error() != CONN_INCOMPLETE &&
              (closing_ || ps->req_->error() == CONN_OK) ) {
    // W/o keep alive - stop it
    if (client_params_->keep_alive_sec_ == 0 && clients_[client_id] && !closing_) {
      selector_->RunInSelectLoop(NewCallback(this, &FailSafeClient::ClearClient,
                                            clients_[client_id]));
      clients_[client_id] = NULL;
      death_time_[client_id] = 0;
    }
    pending_map_->erase(ps->req_);
    RequeuePending();
    ps->req_->request()->client_data()->MarkerClear();
    completion_events_.push_back(make_pair(now, ps->ToString(now) + " => closed w/ " +
        http::GetHttpReturnCodeName(ps->req_->request()->server_header()->status_code())));

    Closure* completion_closure = ps->completion_closure_;
    delete ps;

    selector_->RunInSelectLoop(completion_closure);
  } else {
    ps->req_->request()->client_data()->MarkerRestore();
    ps->req_->request()->server_data()->Clear();
    ps->req_->request()->server_header()->Clear();
    completion_events_.push_back(make_pair(now, ps->ToString(now) + " => retry w/ " +
       http::GetHttpReturnCodeName(
           ps->req_->request()->server_header()->status_code())));
    InternalStartRequest(ps);
  }
  while (completion_events_.size() > FLAGS_http_failsafe_max_log_size) {
      completion_events_.pop_front();
  }
}

void FailSafeClient::ClearClient(ClientProtocol* client) {
  LOG_HTTP << " Clearing client:  w / active:"
           << client->num_active_requests()
           << " waiting: " << client->num_waiting_requests();
  client->ResolveAllRequestsWithError();
  delete client;
}

// Very, very important - ps->req_ is no longer valid at this point
// should not touch it in any way.
void FailSafeClient::DeleteCanceledPendingStruct(PendingStruct* ps) {
  LOG_INFO << " Deleting canceled pending struct: " << ps;
  // const int64 now = selector_->now();
  while (completion_events_.size() > FLAGS_http_failsafe_max_log_size) {
      completion_events_.pop_front();
  }

  if (ps->completion_closure_ != NULL &&
      !ps->completion_closure_->is_permanent()) {
    delete ps->completion_closure_;
  }
  pending_map_->erase(ps->req_);
  delete ps;
}

void FailSafeClient::StatusString(string* s) const {
    const int64 now = selector_->now();
    *s += "\n     Past events:";
    *s += "\n============================================================";
    for (int i = 0; i < completion_events_.size(); ++i) {
        *s += strutil::StringPrintf("\n%.3f ms ago: => ",
                                    (now - completion_events_[i].first) * 1e-3);
        *s += completion_events_[i].second;
    }
    *s += "\n";
    *s += "\n    Pending requests:";
    *s += "\n============================================================";
    for (int i = 0; i < pending_requests_->size(); ++i) {
        *s += (*pending_requests_)[i]->ToString(now);
    }
    *s += "\n";
    *s += "\n    Connection status:";
    *s += "\n============================================================";
    for (int i = 0; i < clients_.size(); ++i) {
        if (clients_[i] == NULL) {
            *s += strutil::StringPrintf("\n  >>> Connection %d --> CLOSED", i);
        } else {
            *s += strutil::StringPrintf("\n  >>> Connection %d Data: ", i);
            *s += clients_[i]->StatusString();
        }
    }
}

void FailSafeClient::ScheduleStatusLog() {
    selector_->RunInSelectLoop(NewCallback(this, &FailSafeClient::WriteStatusLog));
}
void FailSafeClient::WriteStatusLog() const {
    string s;
    StatusString(&s);
    LOG_INFO << "========== Status for: " << servers_[0].ToString() << " (+"
             << servers_.size() - 1 << ")\n" << s;
}


string FailSafeClient::PendingStruct::ToString(int64 crt_time) const {
    return strutil::StringPrintf(
        " [TIME: %" PRId64 " ms] size:%d retries_left:%d urgent:%d error:[%s] ==> ",
        crt_time - start_time_, int(req_->request()->server_data()->Size()),
        int(retries_left_), int(urgent_),
        (req_->error_name() == NULL ? " NONE " : req_->error_name()))
        + req_->name();
}

void FailSafeClient::Reset() {
   if (!selector_->IsInSelectThread()) {
       selector_->RunInSelectLoop(NewCallback(this, &FailSafeClient::Reset));
       return;
   }
   completion_events_.push_back(make_pair(selector_->now(),
                                          "=== Reset requested ==="));

   vector<ClientProtocol*> clients;
   for (int i = 0; i < clients_.size(); ++i) {
       if (clients_[i] != NULL) {
           clients.push_back(clients_[i]);
       }
   }
   clients_.clear();
   death_time_.clear();

   for (int i = 0; i < clients.size(); ++i) {
       clients[i]->ResolveAllRequestsWithProvidedError(CONN_CLIENT_CLOSE);
       delete clients[i];
   }
}

}
