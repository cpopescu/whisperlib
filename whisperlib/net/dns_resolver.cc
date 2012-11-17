// Copyright (c) 2011, Whispersoft s.r.l.
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
// Author: Cosmin Tudorache
//

#include <set>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <whisperlib/base/gflags.h>
#include <whisperlib/base/strutil.h>
#include <whisperlib/base/scoped_ptr.h>
#include <whisperlib/base/cache.h>
#include <whisperlib/sync/thread.h>
#include <whisperlib/sync/producer_consumer_queue.h>
#include <whisperlib/net/dns_resolver.h>

DEFINE_int32(dns_timeout_ms, 10000,
             "Timeout for DNS queries.");


namespace {
class DnsResolver {
 private:
  struct Query {
    net::Selector* selector_;
    string hostname_;
    net::DnsResultHandler* result_handler_;
    Query(net::Selector* selector, const string& hostname,
        net::DnsResultHandler* result_handler)
      : selector_(selector),
        hostname_(hostname),
        result_handler_(result_handler) {
    }
  };
  typedef map<net::DnsResultHandler*, Query*> QueryMap;

  static const uint32 kQueryQueueMaxSize = 1000;
  static const uint32 kCacheSize = 100;
  static const int64 kCacheTimeout = 24*3600*1000LL; // 1 day

 public:
  DnsResolver()
    : thread_(NULL),
      query_map_(),
      query_queue_(kQueryQueueMaxSize),
      cache_(util::CacheBase::LRU, kCacheSize, kCacheTimeout, NULL, NULL) {
  }
  virtual ~DnsResolver() {
    Stop();
  }

  void Start() {
    CHECK_NULL(thread_) << "Already started";
    thread_ = new thread::Thread(NewCallback(this, &DnsResolver::Run));
    bool success = thread_->Start();
    CHECK(success) << "Failed to start DNS";
  }
  void Stop() {
    if ( !IsRunning() ) {
      return;
    }
    LOG_WARNING << "DnsResolver is stopping...";
    // a NULL Query is the exit signal
    query_queue_.Put(NULL);
    thread_->Join();
    delete thread_;
    thread_ = NULL;
  }

  bool IsRunning() const {
    return thread_ != NULL;
  }

  void Resolve(net::Selector* selector, const string& hostname,
      net::DnsResultHandler* result_handler) {
    CHECK_NOT_NULL(selector);
    CHECK_NOT_NULL(result_handler);
    CHECK(result_handler->is_permanent());
    DCHECK(IsRunning());
    if ( query_queue_.IsFull() ) {
      LOG_ERROR << "Too many queries, fail for hostname: ["
                << hostname << "]";
      Return(selector, result_handler, NULL);
      return;
    }

    synch::MutexLocker lock(&mutex_);
    scoped_ref<net::DnsHostInfo> info = cache_.Get(hostname);
    if ( info.get() != NULL ) {
      VLOG(5) << "Cache hit for hostname: [" << hostname << "]";
      Return(selector, result_handler, info);
      return;
    }
    VLOG(4) << "Cache miss for hostname: [" << hostname
            << "]" << ". Resolving";
    Query* query = new Query(selector, hostname, result_handler);
    query_queue_.Put(query);
    query_map_[result_handler] = query;
  }
  void Cancel(net::DnsResultHandler* result_handler) {
    synch::MutexLocker lock(&mutex_);
    QueryMap::iterator it = query_map_.find(result_handler);
    if ( it == query_map_.end() ) {
      return;
    }
    it->second->result_handler_ = NULL;
  }

 private:
  void Run() {
    LOG_INFO << "DnsResolver running..";
    while ( true ) {
      Query* query = query_queue_.Get();
      scoped_ptr<Query> auto_del_query(query);

      // a NULL Query is the exit signal
      if ( query == NULL ) {
        break;
      }

      // a NULL result_handler means a Canceled query
      if ( query->result_handler_ == NULL ) {
        continue;
      }

      // blocking resolve
      scoped_ref<net::DnsHostInfo> info =
          net::DnsBlockingResolve(query->hostname_);

      // deliver result
      {
        synch::MutexLocker lock(&mutex_);
        cache_.Add(query->hostname_, info);
        // a NULL result_handler means a Canceled query
        if ( query->result_handler_ != NULL ) {
          query_map_.erase(query->result_handler_);
          Return(query->selector_, query->result_handler_, info);
        }
      }
    }
    LOG_INFO << "DnsResolver stopped.";
  }
  void Return(net::Selector* selector, net::DnsResultHandler* handler,
              scoped_ref<net::DnsHostInfo> info) {
    selector->RunInSelectLoop(NewCallback(this, &DnsResolver::Return,
        handler, info));
  }
  void Return(net::DnsResultHandler* handler, scoped_ref<net::DnsHostInfo> info) {
    handler->Run(info);
  }

 private:
  // internal resolver thread
  thread::Thread* thread_;

  // synchronize access to query_map_, cache_
  synch::Mutex mutex_;

  // map by DnsResultHandler, useful when we want to Cancel a query
  QueryMap query_map_;

  // communicates with the internal resolver thread
  synch::ProducerConsumerQueue<Query*> query_queue_;

  // cache of solved queries
  util::Cache<string, scoped_ref<net::DnsHostInfo> > cache_;
};

DnsResolver* g_dns_resolver = NULL;
static synch::Mutex g_dns_mutex;

static const int kDnsMutexPoolSize = 11;
static synch::MutexPool g_mutex_pool(kDnsMutexPoolSize);

}

namespace net {

void DnsInit() {
  synch::MutexLocker l(&g_dns_mutex);
  if (g_dns_resolver) {
    LOG_ERROR << "DNS Resolver already initialized!";
    return;
  }
  g_dns_resolver = new DnsResolver();
  g_dns_resolver->Start();
}

void DnsResolve(net::Selector* selector, const string& hostname,
                DnsResultHandler* result_handler) {
  CHECK_NOT_NULL(g_dns_resolver) << "DNS Resolver not initialized!";
  g_dns_resolver->Resolve(selector, hostname, result_handler);
}
void DnsCancel(DnsResultHandler* result_handler) {
  CHECK_NOT_NULL(g_dns_resolver) << "DNS Resolver not initialized!";
  g_dns_resolver->Cancel(result_handler);
}

synch::Mutex glb_dns_exit_mutex;
void DnsExit() {
  DnsResolver* r = NULL;
  {
    synch::MutexLocker lock(&glb_dns_exit_mutex);
    if (g_dns_resolver == NULL ) {
      return;
    }
    r = g_dns_resolver;
    g_dns_resolver = NULL;
  }
  r->Stop();
  delete r;
}

////////////////////////////////////////////////////////////////////////////

scoped_ref<DnsHostInfo> DnsBlockingResolve(const string& hostname) {
  struct addrinfo* result = NULL;
  const int error = ::getaddrinfo(hostname.c_str(), NULL, NULL, &result);
  if ( error != 0 ) {
    LOG_ERROR << "Error resolving hostname: [" << hostname << "]"
        ", error: " << ::gai_strerror(error);
    return NULL;
  }
  set<IpAddress> ipv4, ipv6;
  for (struct addrinfo* res = result; res != NULL; res = res->ai_next) {
    const struct sockaddr_storage* ss =
        reinterpret_cast<const struct sockaddr_storage*>(res->ai_addr);
    if ( ss->ss_family == AF_INET ) {
      // IPv4 address
      const sockaddr_in* p = reinterpret_cast<const sockaddr_in*>(ss);
      ipv4.insert(IpAddress(ntohl(p->sin_addr.s_addr)));
      continue;
    }
    if ( ss->ss_family == AF_INET6 ) {
      // IPv6 address
      const sockaddr_in6* p = reinterpret_cast<const sockaddr_in6*>(ss);
      const uint8* b = reinterpret_cast<const uint8*>(p->sin6_addr.s6_addr);
      ipv6.insert(IpAddress(b[ 0], b[ 1], b[ 2], b[ 3],
                            b[ 4], b[ 5], b[ 6], b[ 7],
                            b[ 8], b[ 9], b[10], b[11],
                            b[12], b[13], b[14], b[15]));
      continue;
    }
    LOG_WARNING << "Don't know how to handle ss_family: " << ss->ss_family;
  }
  ::freeaddrinfo(result);

  DnsHostInfo* info = new DnsHostInfo(hostname, ipv4, ipv6, &g_mutex_pool);
  LOG_WARNING << "Resolved: [" << hostname << "] to: " << info->ToString();
  return info;
}

}
