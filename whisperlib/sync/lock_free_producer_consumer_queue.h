/**
 * Lock free producer - consumer queue w/ max num_reader / writers
 * for Linux.
 *
 * Adapted from:
 * http://www.linuxjournal.com/content/lock-free-multi-producer-multi-consumer-queue-ring-buffer
 *
 * This is a fast, lock free producer / consumer queue for Linux.
 */
// Copyright: Urban Engines, Inc. 2012 onwards.
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
// * Neither the name of Urban Engines inc nor the names of its
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

#ifndef __WHISPERLIB_LOCK_FREE_PRODUCER_CONSUMER_QUEUE_H__
#define __WHISPERLIB_LOCK_FREE_PRODUCER_CONSUMER_QUEUE_H__

#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"

#if defined(__x86_64__) && defined(LINUX)

#include <malloc.h>
#include <unistd.h>
#include <emmintrin.h>

#ifndef ____cacheline_aligned
#define ____cacheline_aligned	__attribute__((aligned(64)))
#endif

namespace whisper {
namespace synch {
template <class C>
class LockFreeProducerConsumerQueue {
private:
    size_t NextPow2(size_t sz) {
        size_t ret = 1;
        while (ret < sz) ret <<= 1;
        return ret;
    }

    const size_t num_producers_;
    const size_t num_consumers_;
    const size_t q_size_;
    const size_t q_mask_;
    const size_t sleep_usec_;

    // currently free position (next to insert)
    size_t head_ ____cacheline_aligned;
    // current tail, next to pop
    size_t tail_ ____cacheline_aligned;
    // last not-processed producer's pointer
    size_t last_head_ ____cacheline_aligned;
    // last not-processed consumer's pointer
    size_t last_tail_ ____cacheline_aligned;

    struct ClientPos {
        size_t head_;
        size_t tail_;
    };
    ClientPos* clients_;

    C** data_;

public:
    LockFreeProducerConsumerQueue(size_t q_size,
                                  size_t num_producers,
                                  size_t num_consumers,
                                  size_t sleep_usec = 250)
        : num_producers_(num_producers),
          num_consumers_(num_consumers),
          q_size_(NextPow2(q_size)), q_mask_(q_size_ - 1), sleep_usec_(sleep_usec),
          head_(0), tail_(0), last_head_(0), last_tail_(0) {
        const size_t n = std::max(num_consumers_, num_producers_);
        clients_ = (ClientPos *)::memalign(getpagesize(), sizeof(ClientPos) * n);
        ::memset((void *)clients_, 0xFF, sizeof(ClientPos) * n);
        data_ = (C **)::memalign(getpagesize(), q_size_ * sizeof(C*));
        VLOG(2) << " Lock free created w/ size: " << q_size
                 << " producers: " << num_producers
                 << " consumers: " << num_consumers
                 << " sleep_usec: " << sleep_usec;
    }

    ~LockFreeProducerConsumerQueue() {
        ::free(data_);
        ::free(clients_);
    }

    void Put(C* data, size_t producer_id) {
        DCHECK_LT(producer_id, num_producers_);

        clients_[producer_id].head_ = head_;
        clients_[producer_id].head_ = __sync_fetch_and_add(&head_, 1);

        while (__builtin_expect(clients_[producer_id].head_ >=
                                last_tail_ + q_size_, 0)) {
            size_t pos_min = tail_;
            for (size_t i = 0; i < num_consumers_; ++i) {
                size_t tmp_tail = clients_[i].tail_;
                asm volatile("" ::: "memory");
                if (tmp_tail < pos_min) pos_min = tmp_tail;
            }
            last_tail_ = pos_min;
            if (clients_[producer_id].head_ < last_tail_ + q_size_)
                break;
            if (__builtin_expect(sleep_usec_ > 0, 1)) {
                usleep(sleep_usec_);
            } else {
                _mm_pause();
            }
        }
        data_[clients_[producer_id].head_ & q_mask_] = data;

        clients_[producer_id].head_ = ULONG_MAX;
    }

    C* Get(size_t consumer_id) {
        DCHECK_LT(consumer_id, num_consumers_);

        clients_[consumer_id].tail_ = tail_;
        clients_[consumer_id].tail_ = __sync_fetch_and_add(&tail_, 1);

        while (__builtin_expect(clients_[consumer_id].tail_ >=
                                last_head_, 0)) {
            size_t pos_min = head_;
            for (size_t i = 0; i < num_producers_; ++i) {
                size_t tmp_head = clients_[i].head_;
                // GCC-specific compiler barrier—to make sure that
                // the compiler won't move order
                asm volatile("" ::: "memory");
                if (tmp_head < pos_min) pos_min = tmp_head;
            }
            last_head_ = pos_min;
            if (clients_[consumer_id].tail_ < last_head_)
                break;
            if (__builtin_expect(sleep_usec_ > 0, 1)) {
                usleep(sleep_usec_);
            } else {
                _mm_pause();
            }
        }
        C *ret = data_[clients_[consumer_id].tail_ & q_mask_];

        clients_[consumer_id].tail_ = ULONG_MAX;
        return ret;
    }
};
}       // namespace synch
}       // namespace whisper

#else   // !defined(__x86_64__) || !defined(LINUX)

#include "whisperlib/sync/producer_consumer_queue.h"

namespace whisper {
namespace synch {
//
// Fallthrough implementation for other systems.
//
template <class C>
class LockFreeProducerConsumerQueue {

private:
    ProducerConsumerQueue<C*> queue_;
    const size_t num_producers_;
    const size_t num_consumers_;
    const size_t unused_sleep_usec_;  // keep compiler happy

public:
    LockFreeProducerConsumerQueue(size_t q_size,
                                  size_t num_producers,
                                  size_t num_consumers,
                                  size_t unused_sleep_usec = 250)
        : queue_(q_size),
          num_producers_(num_producers),
          num_consumers_(num_consumers),
          unused_sleep_usec_(unused_sleep_usec) {
    }
    void Put(C* data, size_t producer_id) {
        DCHECK_LT(producer_id, num_producers_);
        queue_.Put(data);
    }
    C* Get(size_t consumer_id) {
        DCHECK_LT(consumer_id, num_consumers_);
        return queue_.Get();
    }
};
}       // namespace synch
}       // namespace whisper

#endif   // !defined(__x86_64__) || !defined(LINUX)

#endif   // __WHISPERLIB_LOCK_FREE_PRODUCER_CONSUMER_QUEUE_H__
