/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * C++ adaptation & additions
 *
 * Copyright (c) 2013, Urban Engines inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Urban Engines inc nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Catalin Popescu
 */

/**
 * Simple LRU cache class.
 * Adapted from Android's LruCache.java w/ some small changes.
 * Usage may require overriding the protected methods in this class:
 * <ul>
 *   <li>Create - to create a new value for a missing key.</li>
 *   <li>SizeOf - to return the size of a value.</li>
 *   <li>EntryRemoved - to properly dispose (delete) an evicted value.</li>
 * </ul>
 */

#ifndef __UTIL_LRU_CACHE_H__
#define __UTIL_LRU_CACHE_H__

#include "whisperlib/base/types.h"
#include "whisperlib/base/hash.h"
#include "whisperlib/base/hash.h"
#include WHISPER_HASH_MAP_HEADER
#include <list>
#include "whisperlib/sync/mutex.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/ref_counted.h"

#ifdef __has_include
 #if __has_include(<have_includes.h>)
  #include <have_includes.h>
 #elif !defined(HAVE_PTHREAD) && __has_include(<pthread.h>)
  #define HAVE_PTHREAD 1
  #define HAVE_PTHREAD_H
  #define HAVE_PTHREAD_RWLOCK 1
 #endif
#endif

#if defined(CPP14_SHARED_MUTEX) || (defined(HAVE_PTHREAD_RWLOCK) && HAVE_PTHREAD_RWLOCK == 1)
#define LRU_USE_RW_MUTEX
#endif

#ifdef LRU_USE_RW_MUTEX
#include "ext/base/mutex.h"
#define LRU_R_LOCKER(m) base::ReaderLock l(m)
#define LRU_W_LOCKER(m) base::WriterLock l(m)
#define LRU_R_LOCK(m) (m).LockR()
#define LRU_W_LOCK(m) (m).LockW()
#define LRU_UNLOCK(m) (m).Unlock()
#define LRU_LIST_LOCK(m) (m).Lock()
#define LRU_LIST_UNLOCK(m) (m).Unlock()
typedef base::RwMutex LruLockType;
#else
#define LRU_R_LOCKER(m) whisper::synch::SpinLocker l(&m)
#define LRU_W_LOCKER(m) whisper::synch::SpinLocker l(&m)
#define LRU_R_LOCK(m) (m).Lock()
#define LRU_W_LOCK(m) (m).Lock()
#define LRU_UNLOCK(m) (m).Unlock()
#define LRU_LIST_LOCK(m)
#define LRU_LIST_UNLOCK(m)
typedef whisper::synch::Spin LruLockType;
#endif


template<class K, class V>
class LruCachePolicy {
public:
    LruCachePolicy() {
    }
    virtual ~LruCachePolicy() {
    }
    /**
     * Called for entries that have been evicted or removed. This method is
     * invoked when a value is evicted to make space, removed by a call to
     * {@link #remove}, or replaced by a call to {@link #put}. The default
     * implementation does nothing.
     *
     * <p>The method is called without synchronization: other threads may
     * access the cache while this method is executing.
     *
     * @param evicted true if the entry is being removed to make space, false
     *     if the removal was caused by a {@link #put} or {@link #remove}.
     */
    virtual void EntryRemoved(bool /*evicted*/, const K& /*key*/,
                              ref_counted<V>* oldValue) const {
        oldValue->DecRef();
    }

    /**
     * Called after a cache miss to compute a value for the corresponding key.
     * Returns the computed value or null if no value can be computed. The
     * default implementation returns null.
     *
     * <p>The method is called without synchronization: other threads may
     * access the cache while this method is executing.
     *
     * <p>If a value for {@code key} exists in the cache when this method
     * returns, the created value will be released with {@link #EntryRemoved}
     * and discarded. This can occur when multiple threads request the same key
     * at the same time (causing multiple values to be created), or when one
     * thread calls {@link #put} while another is creating a value for the same
     * key.
     */
    virtual bool Create(const K& key, V** value) const = 0;

    /**
     * This is used to destroy a value that was created w/ create
     * and never user. Normally we just delete it.
     */
    virtual void Destroy(V* value) const {
        delete value;
    }

    /**
     * Returns the size of the entry for {@code key} and {@code value} in
     * user-defined units.  The default implementation returns 1 so that size
     * is the number of entries and max size is the maximum number of entries.
     *
     * <p>An entry's size must not change while it is in the cache.
     */
    virtual int SizeOf(const K& key, const V* value) const = 0;

    /**
     * Creates a ref counted for the provided key / value, w/ value incremented.
     */
    virtual ref_counted<V>* CreateRef(const K& /*key*/, V* value) const {
        ref_counted<V>* const ref = new ref_counted<V>(value);
        ref->IncRef();
        return ref;
    }
};

template<class K, class V>
class SimpleLruCachePolicy : public LruCachePolicy<K, V> {
public:
    SimpleLruCachePolicy() {
    }
    virtual bool Create(const K& /*key*/, V** /*value*/) const {
        return false;
    }
    virtual int SizeOf(const K& /*key*/, const V* /*value*/) const {
        return 1;
    }
};

template<class K, class V>
class LruCache {
  public:
    typedef hash_map< K, std::pair<ref_counted<V>*, typename std::list<K>::iterator> > Map;

    /**
     * @param max_size for caches that do not override {@link #sizeOf}, this is
     *     the maximum number of entries in the cache. For all other caches,
     *     this is the maximum sum of the sizes of the entries in this cache.
     */
    LruCache(int64 max_size, const LruCachePolicy<K, V>& policy,
             bool allow_overshoot = false):
        policy_(policy),
        size_(0),
        max_size_(max_size),
        allow_overshoot_(allow_overshoot),
        put_count_(0),
        create_count_(0),
        eviction_count_(0),
        hit_count_(0),
        miss_count_(0) {
        CHECK_GT(max_size, 0);
    }
    ~LruCache() {
        EvictAll();
    }

    /**
     * Returns true if the cache contains a value for key at this time (w/o
     * affecting the use count.
     */
    bool HasKey(const K& key) const {
        LRU_R_LOCKER(mutex_);
        typename Map::const_iterator it = map_.find(key);
        return it != map_.end();
    }

    /**
     * Adjust the size of a key - if it got modified while in cache
     */
    bool AdjustSize(const K& key, int delta) {
        LRU_W_LOCKER(mutex_);
        typename Map::const_iterator it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        DCHECK((delta > 0) || (-delta < size_));
        size_ += delta;
        return true;
    }

    /**
     * Returns the value for {@code key} if it exists in the cache or can be
     * created by {@code #create}. If a value was returned, it is moved to the
     * head of the queue. This returns null if a value is not cached and cannot
     * be created.
     */
    bool Get(const K& key, ref_counted<V>** ret_val) {
        LRU_R_LOCK(mutex_);
        typename Map::iterator it = map_.find(key);
        if (it != map_.end()) {
            *ret_val = it->second.first;
            LRU_LIST_LOCK(list_mutex_);
            list_.erase(it->second.second);
            it->second.second = list_.insert(list_.begin(), key);
            LRU_LIST_UNLOCK(list_mutex_);
            ++hit_count_;
            (*ret_val)->IncRef();
            LRU_UNLOCK(mutex_);
            return true;
        }
        ++miss_count_;
        LRU_UNLOCK(mutex_);

        if (allow_overshoot_) {
            TrimToSize(max_size_);
        }

        /*
         * Attempt to create a value. This may take a long time, and the map
         * may be different when Create() returns. If a conflicting value was
         * added to the map while Create() was working, we leave that value in
         * the map and release the created value.
         */
        V* createdValue;
        if (!policy_.Create(key, &createdValue)) {
            return false;
        }

        bool createdDumped = false;;
        LRU_W_LOCK(mutex_);
        ++create_count_;
        it = map_.find(key);
        if (it != map_.end()) {
            *ret_val = it->second.first;
            list_.erase(it->second.second);
            it->second.second = list_.insert(list_.begin(), key);
            createdDumped = true;
        } else {
            typename std::list<K>::iterator it_key;
            it_key = list_.insert(list_.begin(), key);
            ref_counted<V>* const ref = policy_.CreateRef(key, createdValue);
            *ret_val = ref;
            map_.insert(std::make_pair(key, make_pair(ref, it_key)));
            size_ += SafeSizeOf(key, createdValue);
        }
        (*ret_val)->IncRef();
        LRU_UNLOCK(mutex_);

        if (createdDumped) {
            policy_.Destroy(createdValue);
        } else {
            if (!allow_overshoot_) {
                TrimToSize(max_size_);
            }
        }
        return true;
    }

    /**
     * Caches {@code value} for {@code key}. The value is moved to the head of
     * the queue.
     *
     * @return if a value was replaced.
     */
    bool Put(const K& key, V* value) {
        ref_counted<V>* previous;
        if (Put(key, value, &previous, NULL)) {
            previous->DecRef();
            return true;
        } else {
            return false;
        }
    }

    /**
     * Caches {@code value} for {@code key}. The value is moved to the head of
     * the queue. Returns the previous value for key in {@code previous}.
     *
     * @return if a value was replaced.
     */
    bool Put(const K& key, V* value, ref_counted<V>** previous, ref_counted<V>** added) {
        bool entry_removed = false;
        if (added) {
            *added = NULL;
        }

        if (allow_overshoot_) {
            TrimToSize(max_size_);
        }

        LRU_W_LOCK(mutex_);
        ++put_count_;
        typename Map::iterator it = map_.find(key);
        *previous = NULL;
        if (it != map_.end()) {
            *previous = it->second.first;
            size_ -= SafeSizeOf(key, (*previous)->get());
            list_.erase(it->second.second);
            it->second.second = list_.insert(list_.begin(), key);
            ref_counted<V>* const ref = policy_.CreateRef(key, value);
            if (added) {
                ref->IncRef();
                *added = ref;
            }
            it->second.first = ref;
            entry_removed = true;
        } else {
            typename std::list<K>::iterator it_key;
            it_key = list_.insert(list_.begin(), key);
            ref_counted<V>* const ref = policy_.CreateRef(key, value);
            map_.insert(std::make_pair(key, std::make_pair(ref, it_key)));
            if (added) {
                ref->IncRef();
                *added = ref;
            }
        }
        size_ += SafeSizeOf(key, value);

        LRU_UNLOCK(mutex_);

        if (*previous) {
            (*previous)->IncRef();
        }
        if (entry_removed) {
            policy_.EntryRemoved(false, key, *previous);
        }
        if (!allow_overshoot_) {
            TrimToSize(max_size_);
        }
        return entry_removed;
    }

    /**
     * Removes the entry for {@code key} if it exists.
     *
     * @return if the value was removed.
     */
    bool Remove(const K& key) {
        ref_counted<V>* previous;
        if (Remove(key, &previous)) {
            previous->DecRef();
            return true;
        }
        return false;
    }

    /**
     * Removes the entry for {@code key} if it exists. Returns the previous value
     * for key in {@code previous}.
     *
     * @return if the value was removed.
     */
    bool Remove(const K& key, ref_counted<V>** previous) {
        bool entry_removed = false;
        *previous = NULL;
        LRU_W_LOCK(mutex_);
        typename Map::iterator it = map_.find(key);
        if (it != map_.end()) {
            *previous = it->second.first;
            size_ -= SafeSizeOf(key, (*previous)->get());
            list_.erase(it->second.second);
            map_.erase(it);
            entry_removed = true;
        }
        LRU_UNLOCK(mutex_);

        if (*previous) {
            (*previous)->IncRef();
        }
        if (entry_removed) {
            policy_.EntryRemoved(false, key, *previous);
        }

        return entry_removed;
    }

    /**
     * Clear the cache, calling {@link #EntryRemoved} on each removed entry.
     */
    void EvictAll() {
        TrimToSize(-1); // -1 will evict 0-sized elements
    }

  protected:


  public:
    /**
     * For caches that do not override {@link #sizeOf}, this returns the number
     * of entries in the cache. For all other caches, this returns the sum of
     * the sizes of the entries in this cache.
     */
    int64 size() const {
        LRU_R_LOCKER(mutex_);
        return size_;
    }

    /**
     * Returns the number of elements in cache
     */
    size_t count() const {
        LRU_R_LOCKER(mutex_);
        return map_.size();
    }

    /**
     * For caches that do not override {@link #sizeOf}, this returns the maximum
     * number of entries in the cache. For all other caches, this returns the
     * maximum sum of the sizes of the entries in this cache.
     */
    int64 max_size() const {
        LRU_R_LOCKER(mutex_);
        return max_size_;
    }

    /**
     * Returns the number of times {@link #get} returned a value.
     */
    int64 hit_count() const {
        LRU_R_LOCKER(mutex_);
        return hit_count_;
    }

    /**
     * Returns the number of times {@link #get} returned null or required a new
     * value to be created.
     */
    int64 miss_count() const {
        LRU_R_LOCKER(mutex_);
        return miss_count_;
    }

    /**
     * Returns the number of times {@link #create(Object)} returned a value.
     */
    int64 create_count() const {
        LRU_R_LOCKER(mutex_);
        return create_count_;
    }

    /**
     * Returns the number of times {@link #put} was called.
     */
    int64 put_count() const {
        LRU_R_LOCKER(mutex_);
        return put_count_;
    }

    /**
     * Returns the number of values that have been evicted.
     */
    int eviction_count() const {
        LRU_R_LOCKER(mutex_);
        return eviction_count_;
    }

    /**
     * Is the cache allows overshooting, this is true if the cache is full.
     */
    bool is_full() const {
        LRU_R_LOCKER(mutex_);
        return size_ >= max_size_;
    }

    void lock_r() const {
        LRU_R_LOCK(mutex_);
    }
    const Map& get_map() const { return map_; }
    void unlock() const {
        LRU_UNLOCK(mutex_);
    }

private:
    int SafeSizeOf(const K& key, const V* value) const {
        const int result = policy_.SizeOf(key, value);
        CHECK_GE(result, 0);
        return result;
    }


    /**
     * @param max_size the maximum size of the cache before returning. May be -1
     *     to evict even 0-sized elements.
     */
    void TrimToSize(int64 max_size) {
        while (true) {
            K key;
            ref_counted<V>* value;

            LRU_R_LOCK(mutex_);
            CHECK(size_ >= 0 && !(map_.empty() && size_ != 0))
                <<  ".SizeOf() is reporting inconsistent results!";
            if (size_ <= max_size || map_.empty()) {
                LRU_UNLOCK(mutex_);
                break;
            }
            LRU_UNLOCK(mutex_);
            LRU_W_LOCK(mutex_);
            if (list_.empty()) break;
            key = list_.back();
            list_.pop_back();
            typename Map::iterator it = map_.find(key);
            CHECK(it != map_.end());
            value = it->second.first;
            map_.erase(it);

            size_ -= SafeSizeOf(key, value->get());
            ++eviction_count_;
            LRU_UNLOCK(mutex_);

            policy_.EntryRemoved(true, key, value);
        }
    }

    const LruCachePolicy<K, V>& policy_;
    std::list<K> list_;
    Map map_;

    /** Size of this cache in units. Not necessarily the number of elements. */
    int64 size_;
    int64 max_size_;
    bool allow_overshoot_;

    int64 put_count_;
    int64 create_count_;
    int64 eviction_count_;
    int64 hit_count_;
    int64 miss_count_;

    mutable LruLockType mutex_;
    mutable whisper::synch::Spin list_mutex_;

    DISALLOW_EVIL_CONSTRUCTORS(LruCache);
};
#undef LRU_R_LOCKER
#undef LRU_W_LOCKER
#undef LRU_R_LOCK
#undef LRU_W_LOCK
#undef LRU_UNLOCK
#undef LRU_LIST_LOCK
#undef LRU_LIST_UNLOCK

#endif //  __UTIL_LRU_CACHE_H__
