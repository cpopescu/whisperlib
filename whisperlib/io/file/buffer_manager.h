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
// This class can be used in conjunction with some AioManagers to
// manage fixed sized buffers that are used to read pieces of file.
//
// Usage pattern:
//
// ...
//
// BufferManager::Buffer* buf = manager->GetBuffer();
// Callback<size_t>* full_callback = NewCallback(&BufferFull, buf);
// BufferManager::Buffer::State state = buf->BeginUsage(full_callback);
// if ( state == BufferManager::Buffer::IN_LOOKUP ) {
//   return;   // wait for full_callback to be performed
// }
// delete full_callback;  // not used anymore
// if ( state == BufferManager::Buffer::VALID_DATA ) {
//   UseData(buf);
// }
// // else state == BufferManager::Buffer::NEW;
// const size_t size = ProduceSomeDataInBuffer(buf->data_, buf->data_capacity_);
// buf->MarkValidData(size);
// UseData(buf);
// ...
//
// void BufferFull(BufferManager::Buffer* buf, size_t size) {
//   UseBuffer(buf);
// }
//
// void UseBuffer(BufferManager::Buffer* buf) {
//  * Use data in buf->data_ with size buf->data_size()
//  buf->EndUsage(NULL);
// }
//
// Now, if you want to end the usage while waiting for BufferFull to complete,
// you need to call:
// buf->EndUsage(full_callback);
// delete full_callback_;
//
//

#ifndef __COMMON_IO_FILE_BUFFER_MANAGER_H__
#define __COMMON_IO_FILE_BUFFER_MANAGER_H__

#include <sys/types.h>
#include <map>
#include <set>
#include "whisperlib/base/types.h"
#include "whisperlib/base/hash.h"
#include WHISPER_HASH_SET_HEADER

#include "whisperlib/base/callback.h"
#include "whisperlib/sync/mutex.h"
#include "whisperlib/base/free_list.h"

namespace whisper {
namespace io {

class BufferManager {
 public:
  BufferManager(util::MemAlignedFreeArrayList* freelist);
  ~BufferManager();

  class Buffer {
   public:
    enum State {
      NEW,
      IN_LOOKUP,
      VALID_DATA,
    };
    Buffer(BufferManager* manager,
           const std::string data_key, char* data,
           const int data_capacity, const size_t data_alignment)
        : mutex_(),
          state_(NEW),
          data_(data),
          data_capacity_(data_capacity),
          data_alignment_(data_alignment),
          data_size_(0),
          data_key_(data_key),
          use_count_(0),
          reuse_count_(0),
          last_release_time_(0),
          manager_(manager) {
    }
    ~Buffer() {
    }

    const std::string& data_key() const {
      return data_key_;
    }
    size_t data_size() const {
      CHECK_EQ(state_, VALID_DATA);
      return data_size_;
    }

    //
    // When somebody starts to use a buffer after it whas returned,
    // he has the obligation to call this function.
    // It needs to pass a callback that will be invoked when data is available.
    // If this function returns NEW:
    //    you own the operation of filling up the buffer. You need to call
    //    MarkValidData upon completion. Your data_done will not be called.
    // If this function returns IN_LOOKUP:
    //    you just wait for data_done to be called with the validity of
    //    data.
    // If this function returns VALID_DATA:
    //    you should use the data right away.
    //
    // Call EndUsage when you finished with the data.
    //
    State BeginUsage(Closure* data_done);
    void MarkValidData(size_t size);
    void EndUsage(Closure* data_done);

   public:
    mutable synch::Mutex mutex_;
    State state_;

    char* const data_;              // the data_ buffer
    const int data_capacity_;
    const size_t data_alignment_;   // memory alignment of data_ buffer.

    size_t data_size_;  // the size of the data_ buffer when state == VALID_DATA
    hash_set<Closure*> done_callbacks_;

    // Only manager touches the next members:

    std::string data_key_;   // what data is / should be placed in the buffer.
    int use_count_;     // how many asked for this buffer to use.
    int64 reuse_count_;   // how many times we reused this buffer
    int64 last_release_time_;   // last time when the buffer was released
    BufferManager* const manager_;
    friend class BufferManager;
  };

  Buffer* GetBuffer(const std::string& data_key);
  size_t alignment() const {
    return freelist_->alignment();
  }
  size_t size() const {
    return freelist_->size() * freelist_->alignment();
  }
  std::string GetHtmlStats() const;

 private:
  // Utility functions for GetBuffer - should be called w/ mutex_ held
  BufferManager::Buffer*
  TryReuseFreeBufferLocked(const std::string& data_key);
  BufferManager::Buffer*
  CreateNewBufferLocked(const std::string& data_key);
  BufferManager::Buffer*
  ReDesignateOldestBufferLocked(const std::string& data_key);


  void DoneBuffer(Buffer* buffer);

  // Allocates buffers for us
  util::MemAlignedFreeArrayList* const freelist_;

  typedef std::map<std::string, Buffer*> BufferMap;
  mutable synch::Mutex mutex_;
  // Buffers that were get
  BufferMap active_buffers_;
  // Buffers that are free and may have data
  BufferMap free_buffers_;
  // Buffers to be reassigned - sorted by release time.
  // They are in free_buffers_ too.
  typedef std::set< std::pair<int64, Buffer*> > UsageSet;
  UsageSet usage_set_;

  friend class Buffer;

  DISALLOW_EVIL_CONSTRUCTORS(BufferManager);
};

}  // namespace io
}  // namespace whisper


#endif //  __COMMON_IO_FILE_BUFFER_MANAGER_H__
