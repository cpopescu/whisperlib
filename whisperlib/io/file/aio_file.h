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
// Engine for AIO operations. Here is how things go:
//  - the AIO thread runs in a different thread(s) and performs commands
//  - submit commands to us from another thread which contain an operation code,
//    parameters and a completion callback.
//  - we split commands into read / write / and based on size..
//  - you provide us w/ a selector where we run your completed callbacks.
//  - upon error we automatically delete the guilty file descriptor.
//
// IMPORTANT - use one AioManager per phisical disk !!
//
#ifndef __COMMON_IO_FILE_AIO_FILE__
#define __COMMON_IO_FILE_AIO_FILE__

#include <aio.h>
#include <string>

#include "whisperlib/net/selector.h"
#include "whisperlib/base/types.h"
#include "whisperlib/base/callback.h"
#include "whisperlib/sync/thread.h"
#include "whisperlib/sync/producer_consumer_queue.h"

namespace whisper {
namespace io {

class AioManager {
 public:
  AioManager(const char* name, net::Selector* selector);
  ~AioManager();

  //////////////////////////////////////////////////////////////////////

  // The interface is done through this structure.
  struct Request {
    const int fd_;       // a request regarding this file
    int64 offset_;       // offset where to perform operations
                         // *** IMPORTANT:  MUST be disk block alligned ***
    void* buffer_;       // memory buffer involved in the operation
                         // for write we write here, for read we put data
                         // here
                         // *** IMPORTANT:  MUST be allocated at disk block
                         //     size alligned memory ***
    size_t size_;        // size of buffer_
                         // *** IMPORTANT:  MUST be a multiple of disk block
                         //     size alligned memory ***
    net::Selector* selector_;
    Closure* closure_;   // we run this closure in the 'selector_' thread
                         // upon request completion
    int errno_;          // error status of the request 0 meand OK
    int result_;         // result associated w/ the operation
                         // for read: the number of bytes read from the file
                         // for write: the number of bytes written to the file
    Request(int fd, int64 offset, void* buffer, size_t size,
            net::Selector* selector, Closure* closure)
      :  fd_(fd),
         offset_(offset),
         buffer_(buffer),
         size_(size),
         selector_(selector),
         closure_(closure),
         errno_(0),
         result_(0) {
    }
    struct aiocb* PrepareAioCb(struct aiocb* p, int lio_opcode) const;
  };

  //////////////////////////////////////////////////////////////////////

  // Initiate a read request, as instructed by 'req'
  void Read(Request* req);

  // Initiate a write request, as instructed by 'req'
  void Write(Request* req);

  const std::string& name() const { return name_; }

  // For passing messages between different threads ..
  typedef synch::ProducerConsumerQueue<Request*> ReqQueue;

 private:
  enum Op {
    OP_READ = 0,
    OP_WRITE,
    NUM_OPS,
  };
  // We process results from response_queue_ here
  void ProcessResponses();

  // We have threads for :
  //  OPERATIONS: OP_READ     OP_WRITE
  //  SIZE:
  //       size < 8 block
  //       8 blocks <= size < 32 blocks
  //       32 blocks <= size
  //
  // TODO(cpopescu): !!! Parametrize !!!
  static const size_t kBlockSize = 4096;     // most standard..
  static const size_t kBlockSizeShift = 12;  // i.e. 2^12 = 4096
  static const size_t kNumBlockTypes = 3;

  size_t SizePool(size_t size) const {
    const size_t nblocks = size >> kBlockSizeShift;
    return ((nblocks >> 3) != 0 ) + ((nblocks >> 6) != 0);
  }
  size_t OpPool(Op op) const {
    return static_cast<size_t>(op);
  }

  const std::string name_;   // you can give a name to this guy..
  net::Selector* const selector_;   // all requests should come through
                                    // this selector thread

  // We run one thread per operation x request size bucket
  whisper::thread::Thread* aio_threads_[NUM_OPS * kNumBlockTypes];


  // These threads read their requests through a reques queue..
  ReqQueue* request_queues_[NUM_OPS * kNumBlockTypes];
  // .. and write back the responses to the response thread through this
  ReqQueue response_queue_;

  // this guy reads responses from 'response_queue_' and calls the
  // corresponding closures in the selector
  whisper::thread::Thread response_thread_;

  // TODO(cpopescu):  More statistics then that ugly log

  DISALLOW_EVIL_CONSTRUCTORS(AioManager);
};
}  // namespace io
}  // namespace whisper
#endif  // __COMMON_IO_FILE_AIO_FILE__
