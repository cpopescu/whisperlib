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

#include <whisperlib/base/types.h>
#include WHISPER_HASH_MAP_HEADER
#include <list>
#include <whisperlib/sync/mutex.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/ref_counted.h>

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
    virtual void EntryRemoved(bool evicted, const K& key,
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
     * Returns the size of the entry for {@code key} and {@code value} in
     * user-defined units.  The default implementation returns 1 so that size
     * is the number of entries and max size is the maximum number of entries.
     *
     * <p>An entry's size must not change while it is in the cache.
     */
    virtual int SizeOf(const K& key, const V* value) const = 0;
};

template<class K, class V>
class SimpleLruCachePolicy : public LruCachePolicy<K, V> {
public:
    SimpleLruCachePolicy() {
    }
    virtual bool Create(const K& key, V** value) const {
        return false;
    }
    virtual int SizeOf(const K& key, const V* value) const {
        return 1;
    }
};

template<class K, class V>
class LruCache {
    typedef hash_map< K, pair<ref_counted<V>*, typename list<K>::iterator> > Map;

  public:
    /**
     * @param max_size for caches that do not override {@link #sizeOf}, this is
     *     the maximum number of entries in the cache. For all other caches,
     *     this is the maximum sum of the sizes of the entries in this cache.
     */
    LruCache(int64 max_size, const LruCachePolicy<K, V>& policy, synch::MutexPool* pool):
        policy_(policy),
        pool_(pool),
        size_(0),
        max_size_(max_size),
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
        synch::MutexLocker l(&mutex_);
        typename Map::const_iterator it = map_.find(key);
        return it != map_.end();
    }

    /**
     * Returns the value for {@code key} if it exists in the cache or can be
     * created by {@code #create}. If a value was returned, it is moved to the
     * head of the queue. This returns null if a value is not cached and cannot
     * be created.
     */
    bool Get(const K& key, ref_counted<V>** ret_val) {
        mutex_.Lock();
        typename Map::iterator it = map_.find(key);
        if (it != map_.end()) {
            *ret_val = it->second.first;
            list_.erase(it->second.second);
            it->second.second = list_.insert(list_.begin(), key);

            ++hit_count_;
            (*ret_val)->IncRef();
            mutex_.Unlock();
            return true;
        }
        ++miss_count_;
        mutex_.Unlock();

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
        mutex_.Lock();
        ++create_count_;
        it = map_.find(key);
        if (it != map_.end()) {
            *ret_val = it->second.first;
            list_.erase(it->second.second);
            it->second.second = list_.insert(list_.begin(), key);
            createdDumped = true;
        } else {
            typename list<K>::iterator it_key = list_.insert(list_.begin(), key);
            ref_counted<V>* ref = new ref_counted<V>(createdValue, pool_->GetMutex(createdValue));
            ref->IncRef();
            *ret_val = ref;
            map_.insert(make_pair(key, make_pair(ref, it_key)));
            size_ += SafeSizeOf(key, createdValue);
        }
        (*ret_val)->IncRef();
        mutex_.Unlock();

        if (createdDumped) {
            delete createdValue;
        } else {
            TrimToSize(max_size_);
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

        mutex_.Lock();
        ++put_count_;
        typename Map::iterator it = map_.find(key);
        *previous = NULL;
        if (it != map_.end()) {
            *previous = it->second.first;
            size_ -= SafeSizeOf(key, (*previous)->get());
            list_.erase(it->second.second);
            it->second.second = list_.insert(list_.begin(), key);
            ref_counted<V>* ref = new ref_counted<V>(value, pool_->GetMutex(value));
            ref->IncRef();
            if (added) {
                ref->IncRef();
                *added = ref;
            }
            it->second.first = ref;
            entry_removed = true;
        } else {
            typename list<K>::iterator it_key = list_.insert(list_.begin(), key);
            ref_counted<V>* ref = new ref_counted<V>(value, pool_->GetMutex(value));
            ref->IncRef();
            map_.insert(make_pair(key, make_pair(ref, it_key)));
            if (added) {
                ref->IncRef();
                *added = ref;
            }
        }
        size_ += SafeSizeOf(key, value);

        mutex_.Unlock();

        if (*previous) {
            (*previous)->IncRef();
        }
        if (entry_removed) {
            policy_.EntryRemoved(false, key, *previous);
        }
        TrimToSize(max_size_);
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
        mutex_.Lock();
        typename Map::iterator it = map_.find(key);
        if (it != map_.end()) {
            *previous = it->second.first;
            size_ -= SafeSizeOf(key, (*previous)->get());
            list_.erase(it->second.second);
            map_.erase(it);
            entry_removed = true;
        }
        mutex_.Unlock();

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
    int size() const {
        synch::MutexLocker l(&mutex_);
        return size_;
    }

    /**
     * For caches that do not override {@link #sizeOf}, this returns the maximum
     * number of entries in the cache. For all other caches, this returns the
     * maximum sum of the sizes of the entries in this cache.
     */
    int64 max_size() const {
        synch::MutexLocker l(&mutex_);
        return max_size_;
    }

    /**
     * Returns the number of times {@link #get} returned a value.
     */
    int hit_count() const {
        synch::MutexLocker l(&mutex_);
        return hit_count_;
    }

    /**
     * Returns the number of times {@link #get} returned null or required a new
     * value to be created.
     */
    int miss_count() const {
        synch::MutexLocker l(&mutex_);
        return miss_count_;
    }

    /**
     * Returns the number of times {@link #create(Object)} returned a value.
     */
    int create_count() const {
        synch::MutexLocker l(&mutex_);
        return create_count_;
    }

    /**
     * Returns the number of times {@link #put} was called.
     */
    int put_count() const {
        synch::MutexLocker l(&mutex_);
        return put_count_;
    }

    /**
     * Returns the number of values that have been evicted.
     */
    int eviction_count() const {
        synch::MutexLocker l(&mutex_);
        return eviction_count_;
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

            mutex_.Lock();
            CHECK(size_ >= 0 && !(map_.empty() && size_ != 0))
                <<  ".SizeOf() is reporting inconsistent results!";
            if (size_ <= max_size || map_.empty()) {
                mutex_.Unlock();
                break;
            }
            CHECK(!list_.empty());
            key = list_.back();
            list_.pop_back();
            typename Map::iterator it = map_.find(key);
            CHECK(it != map_.end());
            value = it->second.first;
            map_.erase(it);

            size_ -= SafeSizeOf(key, value->get());
            ++eviction_count_;
            mutex_.Unlock();

            policy_.EntryRemoved(true, key, value);
        }
    }

    const LruCachePolicy<K, V>& policy_;
    synch::MutexPool* pool_;
    list<K> list_;
    Map map_;

    /** Size of this cache in units. Not necessarily the number of elements. */
    int64 size_;
    int64 max_size_;

    int put_count_;
    int create_count_;
    int eviction_count_;
    int hit_count_;
    int miss_count_;

    mutable synch::Mutex mutex_;

    DISALLOW_EVIL_CONSTRUCTORS(LruCache);
};

#endif //  __UTIL_LRU_CACHE_H__
