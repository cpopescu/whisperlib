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

#include <string>
#include <iostream>     // std::cout, std::fixed
#include <iomanip>      // std::setprecision
#include <sstream>
#include <whisperlib/base/types.h>
#include <whisperlib/sync/mutex.h>

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

// A synchronized counter & report tool. Useful on counting class instances, mainly for debug.
struct InstanceCounter {
    const string name_;
    const int64 kPrintIntervalMs;
    uint32 count_;
    int64 print_ts_;
    synch::Mutex lock_;

    InstanceCounter(const string& name, int64 print_interval_ms)
        : name_(name), kPrintIntervalMs(print_interval_ms), count_(0), print_ts_(0), lock_() {}

    // increment the number of instances
    void Inc();
    // decrement the number of instances
    void Dec();
    // prints the number of instances
    void PrintReport();
};

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
    ScopedContainer(const T* t) : t_(t), del_t_(true) {
    }
    ScopedContainer(const T& t) : t_(&t), del_t_(false) {
    }
    ~ScopedContainer() {
        for ( typename T::const_iterator it = t_->begin(); it != t_->end(); ++it ) {
            delete *it;
        }
        if (del_t_) {
            delete t_;
        }
    }
private:
    const T* t_;
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
    if (progress_printer.Step()) \
        LOG(severity) << msg << " - Progress: " << progress_printer.Progress() << "%";
class ProgressPrinter {
public:
    ProgressPrinter(uint64 size, uint32 progress_print_count = 30)
        : size_(size), print_size_(size/progress_print_count), index_(0), print_acc_(0) {}
    // returns: true  -> you should print the progress now, sufficient steps accumulated
    //          false ->
    bool Step() {
        index_++;
        print_acc_++;
        return print_acc_ > print_size_;
    }
    // returns the progress, as an integer in interval [0..100]
    uint32 Progress() const {
        print_acc_ = 0;
        return index_ * 100 / size_;
    }
private:
    const uint64 size_;        // total iteration size
    const uint64 print_size_;  // print every this steps
    uint64 index_;             // current step index
    mutable uint64 print_acc_; // steps accumulated since last Progress() read
};

// Statistics on a repetitive operation
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
    string ToString(double modifier, const string& unit) const {
        ostringstream oss;
        const T avg = total_ / count_;
        oss << setprecision(avg * modifier < 1 ? 3 :
                            avg * modifier < 10 ? 2 :
                            avg * modifier < 100 ? 1: 0) << fixed;
        oss << (avg * modifier) << " " << unit
            << " (" << (min_ * modifier) << " - " << (max_ * modifier) << ")."
            << " Total: " << (total_ * modifier) << " " << unit;
        return oss.str();
    }
private:
    T min_;
    T max_;
    T total_;
    uint32 count_;
};

}

#endif
