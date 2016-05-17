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

// Various utility classes

#ifndef __COMMON_BASE_UTIL_H__
#define __COMMON_BASE_UTIL_H__

#include <stdint.h>
#include <atomic>
#include <string>
#include <iostream>     // std::cout, std::fixed
#include <iomanip>      // std::setprecision
#include <sstream>
#include "whisperlib/base/types.h"

using std::string;

namespace whisper {
namespace util {

extern const string kEmptyString;

// Utility class to represent an integer interval. Can read from string
// (e.g. "2-7"), can generate random numbers in the interval.
// Generally used in tests.
class Interval {
 public:
  Interval(const Interval& other);
  Interval(int32 min, int32 max);

  int32 min() const { return min_; }
  int32 max() const { return max_; }

  // Serialize load from string. e.g. "3-17"
  bool Read(const string& s);

  // Generate a random number in the interval: [min_, max_)
  // seed: used in random number generator
  //       if null: default system seed is used
  int32 Rand(unsigned int* seed = NULL) const;

  // Generate a random string, with a random length in interval: [min_, max_)
  string RandomString(unsigned int* seed = NULL) const;

 private:
  // use 32 bit bounds because ::rand() works on "int" values.
  int32 min_;
  int32 max_;
};

///////////////////////////////////////////////////////////////////////

#if 0
// A synchronized counter & report tool. Useful on counting class instances, mainly for debug.
struct InstanceCounter {
    const string name_;
    const int64 kPrintIntervalMs;
    std::atomic_int count_;
    int64 print_ts_;

    InstanceCounter(const string& name, int64 print_interval_ms)
        : name_(name), kPrintIntervalMs(print_interval_ms), count_(0), print_ts_(0), lock_() {}

    // increment the number of instances
    void Inc();
    // decrement the number of instances
    void Dec();
    // prints the number of instances
    void PrintReport(bool force = true);
};
#endif

//////////////////////////////////////////////////////////////////////////

// scoped_ptr for std containers. Works for any iterable container(list,vector,set).
//
// e.g. list<A*>* x = new list<A*>;
//      x->push_back(new A());
//      x->push_back(new A());
//      ScopedContainer<list<A*> > auto_del(x); // also deletes 'x'
//
// e.g. set<A*> x;
//      x.insert(new A());
//      x.insert(new A());
//      ScopedContainer<set<A*> > auto_del(x); // deletes just the content of 'x'
//
template<typename T>
class ScopedContainer {
public:
    ScopedContainer(T* t) : t_(t), del_t_(true) {}
    ScopedContainer(T& t) : t_(&t), del_t_(false) {}
    ~ScopedContainer() {
        for ( typename T::const_iterator it = t_->begin(); it != t_->end(); ++it ) {
            delete *it;
        }
        t_->clear();
        if (del_t_) {
            delete t_;
        }
    }
private:
    T* t_;
    bool del_t_;
};

// A similar container for std map types (map, hash_map)
template<typename T>
class ScopedMap {
public:
    ScopedMap(T* t) : t_(t), del_t_(true) {}
    ScopedMap(T& t) : t_(&t), del_t_(false) {}
    ~ScopedMap() {
        for ( typename T::const_iterator it = t_->begin(); it != t_->end(); ++it ) {
            delete it->second;
        }
        t_->clear();
        if (del_t_) {
            delete t_;
        }
    }
private:
    T* t_;
    bool del_t_;
};

//////////////////////////////////////////////////////////////////////////////////
// Prints progress on a large iteration
// Usage:
//   // array = large container
//   ProgressPrinter p(array.size());
//   for (const auto& x: array) {
//     .. handle x ..
//     // prints sth like: "Processing array - Progress: 47%"
//     LOG_PROGRESS(INFO, p, "Processing array");
//   }
#define LOG_PROGRESS(severity, progress_printer, msg) \
    if ((progress_printer).Step()) \
        LOG(severity) << msg << " - Progress: " << (progress_printer).Progress() << "%"
class ProgressPrinter {
public:
    ProgressPrinter(size_t size, int progress_print_count = 30)
        : size_(size), print_size_(size / progress_print_count),
          index_(0), next_limit_(print_size_) {}
    // returns: true  -> you should print the progress now, sufficient steps accumulated
    //          false ->
    bool Step() {
        int index = ++index_;
        if (index >= next_limit_) {
            next_limit_ = index + print_size_;
            return true;
        }
        return false;
    }
    // returns the progress, as an integer in interval [0..100]
    uint32 Progress() const {
        return index_ * 100 / size_;
    }
    double ProgressF() const {
        return index_ * 100.0 / size_;
    }
private:
    const size_t size_;       // total iteration size
    const size_t print_size_; // print every this steps
    std::atomic_size_t index_;            // current step index
    size_t next_limit_;       // when the index_ reaches this limit => progress should be printed
};

// Statistics on a set of values
template<typename T>
class SimpleStats {
public:
    SimpleStats() : min_(0), max_(0), total_(0), count_(0) {}
    void Add(T value) {
        if (count_ == 0 || value < min_) {
            min_ = value;
        }
        if (count_ == 0 || value > max_) {
            max_ = value;
        }
        total_ += value;
        count_++;
    }
    string ToString(double modifier, const string& unit, bool show_total) const {
        std::ostringstream oss;
        const T avg = count_ > 0 ? (total_ / count_) : 0;
        oss << std::setprecision(avg * modifier < 1 ? 3 :
                                 avg * modifier < 10 ? 2 :
                                 avg * modifier < 100 ? 1: 0) << std::fixed;
        oss << "avg: " << (avg * modifier)
            << ", min: " << (min_ * modifier)
            << ", max: " << (max_ * modifier);
        if (show_total) {
            oss << ", total: " << (total_ * modifier);
        }
        if (unit.size()) {
            oss << " " << unit;
        }
        return oss.str();
    }
private:
    T min_;
    T max_;
    T total_;
    uint32 count_;
};

// Returns the current process size in memory: virtual memory + resident set size, in bytes.
// Implementation reads "/proc/self/stat" and parses useful data.
// WARNING: due to the implementation, the performance is quite poor for massive calls
// Returns success.
bool ProcessMemUsage(int64* out_vsz, int64* out_rss);
// Returns the current process virtual memory size in bytes.
// Returns 0 on failure.
int64 ProcessMemUsageVSZ();

// Helper, for hi frequency calls to ProcessMemUsage.
// It lowers the calls frequency by caching the obtained values for some time.
class ProcessMemUsageCache {
public:
    ProcessMemUsageCache(uint32 cache_ms) : cache_ms_(cache_ms), vsz_(0), rss_(0), ts_(0) {}

    int64 vsz() { Update(); return vsz_; }
    int64 rss() { Update(); return rss_; }

private:
    // update the cached values if enough time has passed
    void Update();

private:
    // keep values cached for this amount of time (milliseconds)
    int64 cache_ms_;

    // The cached values
    int64 vsz_;
    int64 rss_;

    // the timestamp when the values were obtained
    int64 ts_;
};

}  // namespace util
}  // namespace whisper

#endif
