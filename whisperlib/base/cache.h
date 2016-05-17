// -*- Mode:c++; c-basic-offset:2; indent-tabs-mode:nil; coding:utf-8 -*-
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
// Author: Cosmin Tudorache

#ifndef __WHISPERLIB_BASE_CACHE_H__
#define __WHISPERLIB_BASE_CACHE_H__

#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "whisperlib/base/types.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/sync/mutex.h"
// #include "whisperlib/io/ioutil.h"
#include "whisperlib/base/hash.h"
#include WHISPER_HASH_MAP_HEADER

namespace whisper {
namespace util {

template <typename V>
void DefaultValueDestructor(V v) {
  delete v;
};

struct CacheBase {
  // replacement algorithm
  enum Algorithm {
    LRU,
    MRU,
    RANDOM,
  };
  static const char* AlgorithmName(Algorithm alg) {
    switch(alg) {
      CONSIDER(LRU);
      CONSIDER(MRU);
      CONSIDER(RANDOM);
    }
    LOG_FATAL << "Illegal Algorithm: " << alg;
    return "Unknown";
  }
};

template <typename K, typename V>
class Cache : public CacheBase {
public:
  typedef void (*ValueDestructor)(V);

public:
  class Item;
  typedef hash_map<K, Item*> ItemMap;
  typedef std::map<uint64, Item*> ItemByUseMap;
  typedef std::list<Item*> ItemList;
  typedef typename ItemMap::iterator ItemMapIterator;
  typedef typename ItemList::iterator ItemListIterator;

public:
  class Item {
  public:
    Item(K key, V value, ValueDestructor destructor, uint64 use, int64 expiration_ts)
      : key_(key), value_(value), destructor_(destructor), use_(use),
        expiration_ts_(expiration_ts) {
    }
    ~Item() {
      if ( destructor_ != NULL ) {
        (*destructor_)(value_);
      }
    }
    K key() const { return key_; }
    V value() const { return value_; }
    uint64 use() const { return use_; }
    int64 expiration_ts() const { return expiration_ts_; }
    ItemListIterator items_by_exp_it() { return items_by_exp_it_; }
    void set_value(V value) { value_ = value; }
    void set_use(uint64 use) { use_ = use; }
    void set_items_by_exp_it(ItemListIterator it) { items_by_exp_it_ = it; }
    std::string ToString() const {
      std::ostringstream oss;
      oss << "(use_: " << use_ << ", value_: " << value_ << ")";
      return oss.str();
    }
  private:
    const K key_;
    const V value_;
    // this function is used to delete the 'value_' in destructor
    const ValueDestructor destructor_;
    // a value which increases every time this Item is used. Useful for LRU/MRU.
    uint64 use_;
    // expiration time stamp (ms by CLOCK_MONOTONIC)
    const int64 expiration_ts_;
    // points to the place of this item inside 'Cache::items_by_exp_' list
    ItemListIterator items_by_exp_it_;
  };


public:
  // null_value: what the cache returns on Get() if the key is not found.
  // destructor: a function which knows how to delete V type
  //             Use &DefaultValueDestructor() for default "delete" operator.
  //             Use NULL is you don't wish to destroy the values.
  Cache(Algorithm algorithm, size_t max_size, int64 expiration_ms,
        ValueDestructor destructor, V null_value)
    : CacheBase(),
      mutex_(true),  // reentrant
      algorithm_(algorithm),
      max_size_(max_size),
      default_expiration_ms_(expiration_ms),
      destructor_(destructor),
      null_value_(null_value),
      items_(),
      items_by_use_(),
      items_by_exp_(),
      next_use_(0) {
  }
  virtual ~Cache() {
    Clear();
  }

  Algorithm algorithm() const {
    return algorithm_;
  }
  const char* algorithm_name() const {
    return AlgorithmName(algorithm());
  }
  size_t Size() const {
    synch::MutexLocker l(&mutex_);
    return items_.size();
  }
  size_t Capacity() const {
    return max_size_;
  }

  // expiration_ms: alternative expiration time. If -1, then use default_expiration_ms_.
  void Add(K key, V value, int64 expiration_ms = -1) {
    if (expiration_ms == -1) { expiration_ms = default_expiration_ms_; }
    ExpireSomeCache();

    synch::MutexLocker l(&mutex_);
    if ( FindLocked(key) != NULL ) {
      Del(key);
    }
    if ( items_.size() >= max_size_ ) {
      DelByAlgorithmLocked();
      if ( items_.size() >= max_size_ ) {
        return;
      }
    }
    const uint64 use = NextUse();
    Item* const item = new Item(key, value, destructor_, use,
                          timer::TicksMsec() + expiration_ms);
    items_.insert(std::make_pair(key, item));
    // item->set_items_it(result.first);
    items_by_exp_.push_back(item);
    item->set_items_by_exp_it(--items_by_exp_.end());
    items_by_use_[use] = item;
  }
  // Test if the given key is in cache.
  bool Has(K key) const {
    synch::MutexLocker l(&mutex_);
    return FindLocked(key) != NULL;
  }
  V Get(K key) {
    ExpireSomeCache();
    synch::MutexLocker l(&mutex_);
    DCHECK_EQ(items_by_exp_.size(), items_.size());
    DCHECK_EQ(items_by_use_.size(), items_.size());
    Item* item = FindLocked(key);
    if ( item == NULL ) {
      return null_value_;
    }
    items_by_use_.erase(item->use());
    item->set_use(NextUse());
    items_by_use_[item->use()] = item;
    return item->value();
  }
  void GetAll(std::map<K,V>* out) {
    ExpireSomeCache();
    synch::MutexLocker l(&mutex_);
    for ( typename ItemMap::const_iterator it = items_.begin(); it != items_.end(); ++it ) {
      const K& key = it->first;
      const Item* item = it->second;
      out->insert(std::make_pair(key, item->value()));
    }
  }
#if 0
  // This method exists only for K == string.
  V GetPathBased(K key) {
    synch::MutexLocker l(&mutex_);
    Item* item = io::FindPathBased(&items_, key);
    if ( item == NULL ) {
      return null_value_;
    }
    return item->value();
  }
#endif
  // removes a value from the cache, without destroying it.
  V Pop(K key) {
    ExpireSomeCache();
    synch::MutexLocker l(&mutex_);
    Item* const item = FindLocked(key);
    if ( item == NULL ) {
      return null_value_;
    }
    V v = item->value();
    item->set_value(null_value_);
    Del(item);
    return v;
  }
  void Del(K key) {
    synch::MutexLocker l(&mutex_);
    Item* item = FindLocked(key);
    if ( item == NULL ) {
      return;
    }
    DelItemLocked(item);
  }

  // Removes one value from the cache, freeing up some space, depending on the algorithm:
  // LRU - removes the least recently used value
  // MRU - removes the most recently used value
  // RANDOM - removes a random value
  void Del() {
    synch::MutexLocker l(&mutex_);
    DelByAlgorithmLocked();
  }

  void Clear() {
    synch::MutexLocker l(&mutex_);
    for ( typename ItemMap::const_iterator it = items_.begin();
          it != items_.end(); ++it ) {
      delete it->second;
    }
    items_.clear();
    items_by_use_.clear();
  }

  std::string ToString() const {
    synch::MutexLocker l(&mutex_);
    std::ostringstream oss;
    oss << "Cache{algorithm_: " << algorithm_name()
        << ", max_size_: " << max_size_
        << ", items: ";
    for ( typename ItemMap::const_iterator it = items_.begin();
          it != items_.end(); ++it ) {
      const K& key = it->first;
      const Item* item = it->second;
      oss << std::endl << key << " -> " << item->ToString();
    }
    oss << "}";
    return oss.str();
  }

private:
  uint64 NextUse() {
    synch::MutexLocker l(&mutex_);
    return next_use_++;
  }
  const Item* FindLocked(K key) const {
    typename ItemMap::const_iterator it = items_.find(key);
    return it == items_.end() ? NULL : it->second;
  }
  Item* FindLocked(K key) {
    synch::MutexLocker l(&mutex_);
    typename ItemMap::iterator it = items_.find(key);
    return it == items_.end() ? NULL : it->second;
  }
  void DelItemLocked(Item* item) {
    synch::MutexLocker l(&mutex_);
    items_.erase(item->key());
    items_by_use_.erase(item->use());
    items_by_exp_.erase(item->items_by_exp_it());
    delete item;
  }
  void DelByAlgorithmLocked() {
    Item* to_del = NULL;
    if ( items_by_use_.empty() ) {
      return;
    }
    switch ( algorithm_ ) {
      case LRU:
        to_del = items_by_use_.begin()->second;
        break;
      case MRU:
        to_del = (--items_by_use_.end())->second;
        break;
      case RANDOM:
        to_del = items_.begin()->second;
        break;
    };
    DelItemLocked(to_del);
  }
  void ExpireSomeCache() {
    int64 now = timer::TicksMsec();
    synch::MutexLocker l(&mutex_);
    std::vector<Item*> to_del;
    for ( typename ItemList::iterator it = items_by_exp_.begin();
          it != items_by_exp_.end(); ++it ) {
      Item* item = *it;
      if ( item->expiration_ts() > now ) {
        // first item which expires in the future
        break;
      }
      to_del.push_back(item);
    }
    for ( uint32 i = 0; i < to_del.size(); i++ ) {
      DelItemLocked(to_del[i]);
    }
  }

protected:
  mutable synch::Mutex mutex_;
  const Algorithm algorithm_;
  const size_t max_size_;
  const int64 default_expiration_ms_;
  ValueDestructor destructor_;
  V null_value_;

  // a map of Item* by key.
  ItemMap items_;

  // a map of Item* by use_ count. Useful in LRU/MRU.
  ItemByUseMap items_by_use_;

  // a list of Item* by expiration time.
  ItemList items_by_exp_;

  // used for generating unique USE numbers
  // If the cache is used by 1000000 ops/sec, there will still be
  // 599730 years until this counter resets.
  uint64 next_use_;
};

}  // namespace util
}  // namespace whisper

#endif   // __WHISPERLIB_BASE_CACHE_H__
