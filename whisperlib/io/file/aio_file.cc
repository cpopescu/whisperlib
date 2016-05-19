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

#include "whisperlib/io/file/aio_file.h"
#include "whisperlib/base/free_list.h"
#include "whisperlib/base/core_errno.h"
#include "whisperlib/base/gflags.h"

#ifdef TIZEN
#include <bits/local_lim.h>
#endif

//////////////////////////////////////////////////////////////////////

DEFINE_int32(max_concurrent_aio_ops,
             64,
             "We start at most these many aio operations per thread");

//////////////////////////////////////////////////////////////////////

// If this is defined we want to log periodically aio operations
#undef __AIO_DEEP_LOG__

// Helper Namespace
using namespace whisper;
namespace {

static const size_t kMaxConcurrentRequests = 65536;

// The guy which perfoms the actual reads - run this in a thread -
void MainAioProcessThread(
  size_t block_type,
  int lio_opcode,
  io::AioManager::ReqQueue* reqs,
  io::AioManager::ReqQueue* resps) {
  util::SimpleFreeList<struct aiocb> aio_freelist(kMaxConcurrentRequests);
  struct aiocb* ops[kMaxConcurrentRequests];
  io::AioManager::Request* crt_reqs[kMaxConcurrentRequests];

  LOG_INFO << "Starting AIO processing thread: " << pthread_self()
           << " Block Type: " << block_type << " - opcode: " << lio_opcode;

  io::AioManager::Request* crt = NULL;
  size_t crt_ndx = 0;
#ifdef __AIO_DEEP_LOG__
  int64 total_time = 0;
  int64 total_ops = 0;
  int64 total_bytes = 0;
#endif
  do {
    while ( crt_ndx < size_t(FLAGS_max_concurrent_aio_ops) ) {
      if ( (crt = reqs->Get(0)) != NULL ) {
        crt_reqs[crt_ndx] = crt;
        struct aiocb* p = aio_freelist.New();
        ops[crt_ndx] = crt->PrepareAioCb(p, lio_opcode);
        ++crt_ndx;
      } else {
        break;
      }
    }
    if ( crt_ndx > 0 ) {
      LOG_INFO_IF(crt_ndx >= size_t(FLAGS_max_concurrent_aio_ops))
        << " AIO operations Maxed out !!";
#ifdef __AIO_DEEP_LOG__
      const int64 now = timer::TicksMsec();
#endif
      int64 cb = 0;
      const int err = lio_listio(LIO_WAIT, ops, crt_ndx, NULL);
#ifdef __AIO_DEEP_LOG__
      const int64 delta = (timer::TicksMsec() - now);
#endif
      if ( err && errno != EIO ) {
        LOG_ERROR << " ERROR in lio_listio: "
                  << GetSystemErrorDescription(errno);
        for ( size_t i = 0; i < crt_ndx; ++i ) {
          crt_reqs[i]->errno_ = errno;
          crt_reqs[i]->result_ = -1;
          resps->Put(crt_reqs[i]);
          aio_freelist.Dispose(ops[i]);
        }
      } else {
        for ( size_t i = 0; i < crt_ndx; ++i ) {
          struct aiocb* p = ops[i];
          crt_reqs[i]->errno_ = aio_error(p);
          if ( crt_reqs[i]->errno_ < 0 ) {
            LOG_ERROR << " I/O error on file: " << crt_reqs[i]->errno_
                      << " - " << GetSystemErrorDescription(errno);
          }
          crt_reqs[i]->result_ = crt_reqs[i]->errno_ == 0 ? aio_return(p) : -1;
          if ( crt_reqs[i]->errno_ == 0 ) {
            cb += crt_reqs[i]->result_;
          }
          resps->Put(crt_reqs[i]);
          aio_freelist.Dispose(p);
        }
#ifdef __AIO_DEEP_LOG__
        total_time += delta;
        total_ops += crt_ndx;
        total_bytes += cb;
        LOG_INFO << " AIO Ops: " << crt_ndx
                 << " t=" << delta << " ms"
                 << " sz=" << cb  << " B  " << cb / (delta + 1) << " KBps"
                 << " AVG : #ops " << (static_cast<double>(total_ops) * 1000 /
                                       total_time)
                 << " op/sec " << " speed: " << total_bytes  / (total_time + 1)
                 << " KBps.";
#endif
      }
    }
    crt_ndx = 0;
    crt = reqs->Get();     // waiting part :)
    if ( crt != NULL ) {
      crt_reqs[crt_ndx] = crt;
      struct aiocb* p = aio_freelist.New();
      ops[crt_ndx] = crt->PrepareAioCb(p, lio_opcode);
      ++crt_ndx;
    }
  } while ( crt != NULL);
  LOG_INFO << "Exiting AIO processing thread: " << pthread_self();
}
}

//////////////////////////////////////////////////////////////////////

namespace whisper {
namespace io {

struct aiocb* AioManager::Request::PrepareAioCb(struct aiocb* p,
                                                int lio_opcode) const {
  bzero(p, sizeof(*p));
  p->aio_fildes = fd_;
  p->aio_buf = buffer_;
  p->aio_nbytes = size_;
  p->aio_offset = offset_;

  p->aio_lio_opcode = lio_opcode;
  return p;
}

AioManager::AioManager(const char* name, net::Selector* selector)
  : name_(name),
    selector_(selector),
    response_queue_(kMaxConcurrentRequests),
    response_thread_(NewCallback(
                       this, &AioManager::ProcessResponses)) {
  CHECK(response_thread_.SetJoinable());
  CHECK(response_thread_.SetStackSize(PTHREAD_STACK_MIN + (1 << 20)));
  CHECK(response_thread_.Start());
  for ( size_t i = 0; i < NUM_OPS; ++i ) {
    const int lio_opcode = i < kNumBlockTypes ? LIO_READ : LIO_WRITE;
    for ( size_t j = 0; j < kNumBlockTypes; ++j ) {
      const size_t ndx = i * kNumBlockTypes + j;
      request_queues_[ndx] = new ReqQueue(kMaxConcurrentRequests);
      Closure* const c = NewCallback(&MainAioProcessThread,
                                     j,
                                     lio_opcode,
                                     request_queues_[ndx],
                                     &response_queue_);
      aio_threads_[ndx] = new ::thread::Thread(c);
      // 1MB - over the minimum ..
      CHECK(aio_threads_[ndx]->SetStackSize(PTHREAD_STACK_MIN + (1 << 20)));
      CHECK(aio_threads_[ndx]->SetJoinable());
      CHECK(aio_threads_[ndx]->Start());
    }
  }
}

AioManager::~AioManager() {
  LOG_INFO << " Deleting: " << this;
  for ( size_t i = 0; i < NUMBEROF(request_queues_); ++i ) {
    request_queues_[i]->Put(NULL);
    request_queues_[i]->Put(NULL);   // need to put two of them :)
  }
  for ( size_t i = 0; i < NUMBEROF(aio_threads_); ++i ) {
    LOG_INFO << " Waiting for aio_thread " << i << " to stop.";
    aio_threads_[i]->Join();
    delete aio_threads_[i];
  }
  for ( size_t i = 0; i < NUMBEROF(request_queues_); ++i ) {
    delete request_queues_[i];
  }
  response_queue_.Put(NULL);
  LOG_INFO << " Waiting for response thread..";
  response_thread_.Join();
  LOG_INFO << " AioManager ended !";
}

void AioManager::ProcessResponses() {
  while ( true ) {
    AioManager::Request* resp = response_queue_.Get();
    if ( resp == NULL ) {
      break;
    }
    if ( resp->closure_ == NULL ) {
      continue;
    }
    resp->selector_->RunInSelectLoop(resp->closure_);
  }
  LOG_INFO << "Response processor ended ..";
}

void AioManager::Read(Request* req) {
  const size_t ndx = OP_READ * kNumBlockTypes + SizePool(req->size_);
  request_queues_[ndx]->Put(req);
}
void AioManager::Write(Request* req) {
  const size_t ndx = OP_WRITE * kNumBlockTypes + SizePool(req->size_);
  request_queues_[ndx]->Put(req);
}
}  // namespace io
}  // namespace whisper
