// Copyright 2010 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// NOTE: contains modifications by giao and cp@urbanengines.com
//
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
// * Neither the name of Whispersoft s.r.l nor the names of its
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

#include "whisperlib/base/log.h"
#include "whisperlib/base/timer.h"
#include <iomanip>

namespace {
struct NullBuffer : std::streambuf {
    int overflow(int c) { return c; }
};
static NullBuffer glb_null_buffer;
static std::ostream glb_null_stream(&glb_null_buffer);
}
namespace whisper_base {
std::ostream& null_stream() { return glb_null_stream; }
}

#if !defined(USE_GLOG_LOGGING) || !defined(HAVE_GLOG)

// We just define some flags in here
DEFINE_bool(logtostderr, false,
            "log messages go to stderr instead of logfiles");
DEFINE_bool(alsologtostderr, false,
            "log messages go to stderr in addition to logfiles");
DEFINE_string(log_dir, "/tmp",
              "If specified, logfiles are written into this directory instead "
              "of the default logging directory.");
DEFINE_int32(v, 0, "Show all VLOG(m) messages for m <= this.");
DEFINE_int32(log_level, LogLevel_DEBUG, "The level at which we allow logging.");


namespace GLOG_NAMESPACE {
  void InitGoogleLogging(const char* arg0) {}
  void InstallFailureSignalHandler() {}
}

namespace whisper_base {
std::ostream& LogPrefix::DateLogger::HumanDate(std::ostream& str) const {
#if defined(_MSC_VER)
     static const char buffer_[20];
     _strtime_s(buffer_, sizeof(buffer_));
     return str << buffer_;
#else
     time_t time_value = time(NULL);
     struct tm now;
     localtime_r(&time_value, &now);
     const int64 usec = whisper::timer::TicksUsec() % 1000000LL;
     return str << std::setfill('0')
                << std::setw(2) << 1 + now.tm_mon
                << std::setw(2) << now.tm_mday
                << ' '
                << std::setw(2) << now.tm_hour  << ':'
                << std::setw(2) << now.tm_min   << ':'
                << std::setw(2) << now.tm_sec   << "."
                << std::setw(6) << usec
                << std::setfill(' '); //  << std::setw(5);
}
}  // namespace ext_base
#endif

#endif
