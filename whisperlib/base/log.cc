// Copyright 2010 Google
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

// NOTE: contains modifications by giao and cp@1618labs.com

#include <whisperlib/base/log.h>

#if !defined(_LOGGING_H_) && !defined(HAVE_GLOG)

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

#endif
