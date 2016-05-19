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

#include <pthread.h>
#include "whisperlib/net/ipclassifier.h"
#include "whisperlib/io/file/file_input_stream.h"
#include "whisperlib/base/gflags.h"

//////////////////////////////////////////////////////////////////////
/*
DEFINE_string(ip2location_classifier_db,
              "",
              "Where the file ip2location database file is placed");
*/
//////////////////////////////////////////////////////////////////////

namespace {
template <class C>
void SplitClassifiers(C* c, const char* members) {
  std::vector<std::string> components;
  CHECK(strutil::SplitBracketedString(members, ',', '(', ')',  &components))
        << " Invalid classifier spec: [" << members << "]";
  for ( size_t i = 0; i < components.size(); ++i ) {
    c->Add(whisper::net::IpClassifier::CreateClassifier(components[i]));
  }
}
}

namespace whisper {
namespace net {
IpClassifier* IpClassifier::CreateClassifier(const std::string& spec) {
  size_t pos_open = spec.find("(");
  size_t pos_close = spec.rfind(")");
  CHECK(pos_open != std::string::npos)
    << " Invalid IpClassifier specication (missing '(') : ["
    << spec << "]";
  CHECK(pos_close != std::string::npos)
    << " Invalid IpClassifier specication (missing ')') : ["
    << spec << "]";
  CHECK_GT(pos_close, pos_open);
  const std::string name = strutil::StrTrim(spec.substr(0, pos_open));
  const std::string arg = strutil::StrTrim(spec.substr(pos_open + 1,
                                                  pos_close - pos_open - 1));
  LOG_INFO << "Creating " << name << " classifier w/ arg: [" << arg << "]";
  if ( name == "NONE" ) {
    return new IpNoneClassifier();
  } else if ( name == "AND" ) {
    return new IpAndClassifier(arg);
  } else if ( name == "OR" ) {
    return new IpOrClassifier(arg);
  } else if ( name == "NOT" ) {
    return new IpNotClassifier(arg);
  /*
  } else if ( name == "IPLOC" ) {
    return new IpLocationClassifier(arg);
  */
  } else if ( name == "IPFILTER" ) {
    return new IpFilterStringClassifier(arg);
  } else if ( name == "IPFILTERFILE" ) {
    return new IpFilterFileClassifier(arg);
  }
  LOG_FATAL << " Invalid classifier type: [" << name << "]";
  return NULL;
}

//////////////////////////////////////////////////////////////////////

// Logical classifiers

IpOrClassifier::IpOrClassifier(const std::string& members)
  : IpClassifier() {
  SplitClassifiers<IpOrClassifier>(this, members.c_str());
}
IpAndClassifier::IpAndClassifier(const std::string& members)
  : IpClassifier() {
  SplitClassifiers<IpAndClassifier>(this, members.c_str());
}
IpNotClassifier::IpNotClassifier(const std::string& member)
  : IpClassifier(),
    member_(IpClassifier::CreateClassifier(member)) {
}

//////////////////////////////////////////////////////////////////////

// IP location classifier
/*
static pthread_once_t resolver_control;
net::Ip2Location* IpLocationClassifier::resolver_ = NULL;

IpLocationClassifier::IpLocationClassifier(const std::string& spec) {
  CHECK_SYS_FUN(
    pthread_once(&resolver_control, &IpLocationClassifier::InitResolver), 0);
  std::vector< std::pair<std::string, std::string> > conditions;
  strutil::SplitPairs(spec, ",", ":", &conditions);
  for ( int i = 0; i < conditions.size(); ++i ) {
    const std::string& key = conditions[i].first;
    const std::string& val = conditions[i].second;
    if ( key == "C" ) {
      countries_.insert(val);
    } else if ( key == "CS" ) {
      countries_short_.insert(val);
    } else if ( key == "REG" ) {
      regions_.insert(val);
    } else if ( key == "CITY" ) {
      cities_.insert(val);
    } else if ( key == "ISP" ) {
      isps_.insert(val);
    } else {
      LOG_WARNING << "===> UNRECOGNIZED IpLocationClassifier key: " << key;
    }
  }
}
bool IpLocationClassifier::IsInClass(const IpAddress& ip) const {
  Ip2Location::Record record;
  if ( !resolver_->LookupAll(ip, &record) ) {
    return false;
  }
  if ( !countries_.empty() &&
       countries_.find(record.country_long_) ==
       countries_.end() ) {
    return false;
  }
  if ( !countries_short_.empty() &&
       countries_short_.find(record.country_short_) ==
       countries_short_.end() ) {
    return false;
  }
  if ( !regions_.empty() && regions_.find(record.region_) == regions_.end() ) {
    return false;
  }
  if ( !cities_.empty() && cities_.find(record.city_) == cities_.end() ) {
    return false;
  }
  if ( !isps_.empty() && isps_.find(record.isp_) == isps_.end() ) {
    return false;
  }
  return true;
}

void IpLocationClassifier::InitResolver() {
  CHECK(resolver_ == NULL);
  resolver_ = new Ip2Location(FLAGS_ip2location_classifier_db.c_str());
}
*/


IpFilterFileClassifier::IpFilterFileClassifier(const std::string& spec) {
  const std::string content(io::FileInputStream::ReadFileOrDie(spec.c_str()));
  std::vector<std::string> ips;
  strutil::SplitString(std::string(content), "\n", &ips);
  for ( size_t i = 0; i < ips.size(); ++i ) {
    filter_.Add(strutil::StrTrim(ips[i]).c_str());
  }
}

IpFilterStringClassifier::IpFilterStringClassifier(const std::string& spec) {
  std::vector<std::string> ips;
  strutil::SplitString(spec, ",", &ips);
  for ( size_t i = 0; i < ips.size(); ++i ) {
    filter_.Add(strutil::StrTrim(ips[i]).c_str());
  }
}

}  // namespace net
}  // namespace whisper
