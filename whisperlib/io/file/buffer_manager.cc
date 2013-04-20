// Copyright (c) 2010, Whispersoft s.r.l.
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

#include <whisperlib/base/strutil.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/base/date.h>
#include <whisperlib/io/file/buffer_manager.h>

#define BM_LOG_DEBUG   VLOG(10)
#define BM_LOG_INFO    VLOG(8)
#define BM_LOG_WARNING LOG_WARNING
#define BM_LOG_ERROR   LOG_ERROR
#define BM_LOG_FATAL   LOG_FATAL

namespace io {

BufferManager::Buffer::State
BufferManager::Buffer::BeginUsage(Closure* data_done) {
  synch::MutexLocker l(&mutex_);
  switch ( state_ ) {
    case NEW:
      state_ = IN_LOOKUP;
      return NEW;
    case IN_LOOKUP:
      done_callbacks_.insert(data_done);
    case VALID_DATA:
      return state_;
  }
  BM_LOG_FATAL << "Illegal state_: " << state_;
  return NEW;  // keep gcc happy
}

void BufferManager::Buffer::MarkValidData(size_t size) {
  mutex_.Lock();
  CHECK_EQ(state_, IN_LOOKUP);
  DCHECK_LE(size, data_capacity_);
  data_size_ = size;
  state_ = VALID_DATA;
  if ( done_callbacks_.empty() ) {
    mutex_.Unlock();
    return;
  }
  vector<Closure*> to_call(done_callbacks_.size());
  copy(done_callbacks_.begin(), done_callbacks_.end(), to_call.begin());
  mutex_.Unlock();
  for (int i = 0; i < to_call.size(); ++i ) {
    to_call[i]->Run();
  }
}

void BufferManager::Buffer::EndUsage(Closure* data_done) {
  {
    synch::MutexLocker l(&mutex_);
    done_callbacks_.erase(data_done);
  }
  manager_->DoneBuffer(this);
}

//////////////////////////////////////////////////////////////////////

BufferManager::BufferManager(util::MemAlignedFreeArrayList* freelist)
    : freelist_(freelist) {
}

BufferManager::~BufferManager() {
  CHECK(active_buffers_.empty()) << active_buffers_.size()
                                 << " active buffers on destructor.";
  for ( BufferMap::const_iterator it = free_buffers_.begin();
        it != free_buffers_.end(); ++it ) {
  }
  free_buffers_.clear();
  usage_set_.clear();
}

BufferManager::Buffer* BufferManager::GetBuffer(const string& data_key) {
  synch::MutexLocker l(&mutex_);
  // Check if we have the data in active buffers:
  const BufferMap::const_iterator it = active_buffers_.find(data_key);
  if ( it != active_buffers_.end() ) {
    // We have it !
    ++it->second->use_count_;
    BM_LOG_INFO << "Reusing active buffer for: [" << data_key << "]";
    return it->second;
  }
  // Check to see if some free buffers have this data
  Buffer* buf = TryReuseFreeBufferLocked(data_key);
  if ( buf ) {
    BM_LOG_INFO << "Reusing buffer from freelist for: [" << data_key << "]";
    return buf;
  }

  // Need to create a new one or reuse an old one
  // First try to create a new one
  buf = CreateNewBufferLocked(data_key);
  if ( buf ) {
    BM_LOG_INFO << "Created new data block for: [" << data_key << "]";
    return buf;  // OK..
  }

  // We need to free some old buffer
  if ( free_buffers_.empty() ) {
    // Trouble - no more buffers:
    BM_LOG_WARNING << "No more buffers to alloc for: [" << data_key << "]";
    return NULL;
  }
  DCHECK(!usage_set_.empty());
  return ReDesignateOldestBufferLocked(data_key);
}

void BufferManager::DoneBuffer(BufferManager::Buffer* buf) {
  synch::MutexLocker l(&mutex_);
  DCHECK_GT(buf->use_count_, 0);
  --buf->use_count_;
  if ( buf->use_count_ > 0 ) {
    return;
  }
  BM_LOG_INFO << "Done w/ buffer: [" << buf->data_key_ << "], use count: "
              << buf->use_count_;
  const BufferMap::iterator it = active_buffers_.find(buf->data_key_);
  DCHECK(it != active_buffers_.end());
  active_buffers_.erase(it);
  if ( buf->data_size_ == 0 ) {
    // No valid data - we just release it.
    freelist_->Dispose(buf->data_);
    delete buf;
    return;
  }
  int64 now = timer::Date::Now();
  buf->last_release_time_ = now;
  usage_set_.insert(make_pair(now, buf));
  free_buffers_.insert(make_pair(buf->data_key_, buf));
}

//////////////////////////////////////////////////////////////////////

BufferManager::Buffer*
BufferManager::TryReuseFreeBufferLocked(const string& data_key) {
  BufferMap::iterator it_free = free_buffers_.find(data_key);
  if ( it_free == free_buffers_.end() ) {
    return NULL;
  }
  DCHECK(it_free->second->use_count_ == 0);

  // Move the buffer to active
  Buffer* const buf = it_free->second;
  free_buffers_.erase(it_free);

  UsageSet::iterator usage_it = usage_set_.find(
      make_pair(buf->last_release_time_, buf));
  DCHECK(usage_it != usage_set_.end());
  usage_set_.erase(usage_it);

  active_buffers_.insert(make_pair(data_key, buf));
  ++buf->use_count_;
  ++buf->reuse_count_;
  return buf;
}

BufferManager::Buffer*
BufferManager::CreateNewBufferLocked(const string& data_key) {
  char* data = freelist_->New();
  if ( NULL == data ) {
    BM_LOG_INFO << "Freelist exhausted, cannot allocate a new buffer";
    return NULL;
  }
  Buffer* const buf = new Buffer(this, data_key, data,
                                 freelist_->size() * freelist_->alignment(),
                                 freelist_->alignment());
  buf->use_count_ = 1;
  active_buffers_.insert(make_pair(data_key, buf));
  return buf;
}

BufferManager::Buffer*
BufferManager::ReDesignateOldestBufferLocked(const string& data_key) {
  if ( usage_set_.empty() ) {
    return NULL;
  }
  // Get the first buffer from the front of the set
  Buffer* buf = usage_set_.begin()->second;
  usage_set_.erase(usage_set_.begin());

  BM_LOG_INFO << "Redesignating buffer [" << buf->data_key_ << "] as ["
           << data_key << "]";

  // Take it away from free_buffers_
  DCHECK_EQ(buf->use_count_, 0);
  free_buffers_.erase(buf->data_key());
  // Re-assign it to the new data key
  buf->data_key_ = data_key;
  buf->state_ = Buffer::NEW;
  buf->use_count_ = 1;
  buf->reuse_count_ = 0;
  active_buffers_.insert(make_pair(data_key, buf));
  return buf;
}

string BufferManager::GetHtmlStats() const {

  synch::MutexLocker l(&mutex_);
  string out = strutil::StringPrintf("<h3>Total active buffers: %zu</h3>",
                                     active_buffers_.size());
  out += strutil::StringPrintf("<h3>Total free buffers: %zu</h3>",
                               free_buffers_.size());

  out += "<h2>Active Buffers</h2>\n<table border=1>\n";
  out += "<tr bgcolor=\"#fff0ff\"><td>Key</td><td>Use Count</td>"
         "<td>State</td><td>Reuse Count</td></tr>\n";
  for ( BufferMap::const_iterator it = active_buffers_.begin();
        it != active_buffers_.end(); ++it ) {
    out += strutil::StringPrintf(
        "<tr><td>%s</td><td>%d</td><td>%d</td><td>%" PRId64 "</td></tr>",
        it->first.c_str(),
        it->second->use_count_,
        it->second->state_,
        it->second->reuse_count_);
  }
  out += "</table>\n<h2>Free Buffers</h2>\n<table border=1>\n";
  out += "<tr bgcolor=\"#fff0ff\"><td>Key</td><td>Reuse Count</td>"
         "<td>Age(sec)</td></tr>\n";
  for ( UsageSet::const_reverse_iterator it = usage_set_.rbegin();
        it != usage_set_.rend(); ++it ) {
    int64 now = timer::Date::Now();
    out += strutil::StringPrintf(
        "<tr><td>%s</td><td>%" PRId64 "</td><td>%" PRId64 "</td></tr>",
        it->second->data_key_.c_str(),
        it->second->reuse_count_,
        (it->first - now) / 1000);
  }
  out += "</table>";
  return out;
}


}
