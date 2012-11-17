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
// This defines a way to classify an IP address (a more intricated way
// of ip filtering). You normally specify a classifier in a string:
//
// <classifier spec> := <classifier name>"("<argument>")"
//
// Where name:
//  "AND" -> IpAndClassifier - makes an AND on the classifiers found in argument
//       <argument> := <classifier spec>[","<classifier spec>]*
//
//  "OR" -> IpOrlassifier - makes an OR on the classifiers found in argument
//       <argument> := <classifier spec>[","<classifier spec>]*
//
//  "NOT" -> IpNotClassifier - return the NOT  on the classifier in argument
//       <argument> := <classifier spec>
//
//  "IPLOC" -> IpLocationClassifier - is in the class if we are in the location
//       specified in argument
//      <argument> := <location spec>[","<location spec>]*
//      <location spec> := "C:"<country> |
//                         "CS:"<country short> |
//                         "REG:"<region> |
//                         "CITY:"<city> |
//                         "ISP:"<isp>
//
//  "IPFILTER" -> IpFilterStringClassifier - filters an IP that matches a
//      ip filter specification:
//     <argument> := <filter spec>[","<filter spec>]*
//     <filter spec> := <ip spec> | <ip range spec>
//     <ip spec> := <uint8>"."<uint8>"."<uint8>"."<uint8>
//     <ip range spec> := <uint8>"."<uint8>"."<uint8>"."<uint8>"/"<uint8>
//
//  "IPFILTERFILE" -> IpFilterFileClassifier - same as IpFilterStringClassifier,
//     but the filter specifications are given in a file (one per line).
//     <argument> := <filename>
//     content of <filename> := <filter spec>["\n"<filter spec>]*
//
// A specification like:
//
//  OR(IPLOC(CITY:BUCHAREST,CITY:PLOIESTI,ISP:EVOLVA),IPFILTER(171.1.2.0/24),
//     AND(IPFILTER(201.1.2.240/26), NOT(IPLOC(CITY:PITESTI))))
//
// will make a class that has ips in:
//   - city of BUCHAREST or PLOIEST and isp w/ Evolva
//  OR
//   - under ip mask 171.1.2.0/24
//  OR
//   - under ip mask 201.1.2.240/26
//     AND not in PITESTI
//
#ifndef __NET_UTIL_IPCLASSIFIER_H__
#define __NET_UTIL_IPCLASSIFIER_H__

#include <vector>
#include <set>
#include <whisperlib/base/types.h>
#include <whisperlib/net/address.h>

// TODO(cpopescu): maybe add this if we use ip2location
// #include <whisperlib/net/util/ip2location.h>

namespace net {

class IpClassifier {
 public:
  IpClassifier() {}
  virtual ~IpClassifier() {}
  virtual bool IsInClass(const IpAddress& ip) const = 0;

  // Factory method:
  static IpClassifier* CreateClassifier(const string& spec);
 private:
  DISALLOW_EVIL_CONSTRUCTORS(IpClassifier);
};

//////////////////////////////////////////////////////////////////////

class IpNoneClassifier : public IpClassifier {
 public:
  IpNoneClassifier() : IpClassifier() {}
  virtual ~IpNoneClassifier() {}
  virtual bool IsInClass(const IpAddress& ip) const {
    return false;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(IpNoneClassifier);
};


class IpOrClassifier : public IpClassifier {
 public:
  IpOrClassifier() : IpClassifier() {}
  IpOrClassifier(const string& members);
  virtual ~IpOrClassifier() {
    for ( int i = 0; i < members_.size(); ++i ) {
      delete members_[i];
    }
  }
  virtual bool IsInClass(const IpAddress& ip) const {
    for ( int i = 0; i < members_.size(); ++i ) {
      if ( members_[i]->IsInClass(ip) )
        return true;
    }
    return false;
  }
  void Add(IpClassifier* member) {
    members_.push_back(member);
  }
 private:
  vector<IpClassifier*> members_;
  DISALLOW_EVIL_CONSTRUCTORS(IpOrClassifier);
};

class IpAndClassifier : public IpClassifier  {
 public:
  IpAndClassifier() : IpClassifier() {}
  IpAndClassifier(const string& members);
  virtual ~IpAndClassifier() {
    for ( int i = 0; i < members_.size(); ++i ) {
      delete members_[i];
    }
  }
  virtual bool IsInClass(const IpAddress& ip) const {
    for ( int i = 0; i < members_.size(); ++i ) {
      if ( !members_[i]->IsInClass(ip) )
        return false;
    }
    return true;
  }
  void Add(IpClassifier* member) {
    members_.push_back(member);
  }
 private:
  vector<IpClassifier*> members_;
  DISALLOW_EVIL_CONSTRUCTORS(IpAndClassifier);
};

class IpNotClassifier : public IpClassifier  {
 public:
  IpNotClassifier()
      : IpClassifier(), member_(NULL) {}
  IpNotClassifier(const string& member);
  virtual ~IpNotClassifier() {
    delete member_;
  }
  virtual bool IsInClass(const IpAddress& ip) const {
    CHECK(member_);
    return !member_->IsInClass(ip);
  }
 private:
  IpClassifier* member_;
  DISALLOW_EVIL_CONSTRUCTORS(IpNotClassifier);
};

//////////////////////////////////////////////////////////////////////
/*
class IpLocationClassifier : public IpClassifier  {
 public:
  IpLocationClassifier() : IpClassifier() {}
  IpLocationClassifier(const string& spec);

  virtual bool IsInClass(const IpAddress& ip) const;

 private:
  static void InitResolver();
  // we do an AND on all features
  set<string> countries_short_;  // and OR inside each field
  set<string> countries_;
  set<string> regions_;
  set<string> cities_;
  set<string> isps_;

  static net::Ip2Location* resolver_;
  DISALLOW_EVIL_CONSTRUCTORS(IpLocationClassifier);
};
*/

//////////////////////////////////////////////////////////////////////

class IpFilterStringClassifier : public IpClassifier  {
 public:
  IpFilterStringClassifier() : IpClassifier() {}
  IpFilterStringClassifier(const string& spec);

  virtual bool IsInClass(const IpAddress& ip) const {
    return filter_.Matches(ip);
  }

 private:
  IpV4Filter filter_;
  DISALLOW_EVIL_CONSTRUCTORS(IpFilterStringClassifier);
};

class IpFilterFileClassifier : public IpClassifier  {
 public:
  IpFilterFileClassifier() : IpClassifier() {}
  IpFilterFileClassifier(const string& spec);

  virtual bool IsInClass(const IpAddress& ip) const {
    return filter_.Matches(ip);
  }

 private:
  IpV4Filter filter_;
  DISALLOW_EVIL_CONSTRUCTORS(IpFilterFileClassifier);
};

//////////////////////////////////////////////////////////////////////
}

#endif  //  __NET_UTIL_IPCLASSIFIER_H__
