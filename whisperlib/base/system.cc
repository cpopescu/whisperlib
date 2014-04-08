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
// Author: Cosmin Tudorache & Catalin Popescu

#include "whisperlib/base/system.h"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include "whisperlib/base/log.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/base/gflags.h"


#include "whisperlib/net/dns_resolver.h"

//////////////////////////////////////////////////////////////////////

DEFINE_bool(loop_on_exit, false,
            "If this is turned on, we loop on exit waiting for your debuger");

namespace common {

void InternalSystemExit(int error) {
  if ( FLAGS_loop_on_exit ) {
    while ( true ) {
      sleep(40);
    }
  }
}

// This is used by the unittest to test error-exit code
int Exit(int error, bool forced) {
  net::DnsExit();
#if defined(HAVE_GLOG) && defined(USE_GLOG_LOGGING)
  google::FlushLogFiles(0);
#endif
  if ( forced ) {
    // forcefully exit, avoiding any destructor/atexit()/etc calls
    cerr << "Forcefully exiting with error: " << error << endl;
    ::_exit(error);
  } else {
    InternalSystemExit(error);
    ::exit(error);
  }
  // keep things simple in main()...
  return error;
}

void Init(int argc, char* argv[]) {
  //  bfd_filename = strdup(argv[0]);
  string full_command(
    strutil::JoinStrings(const_cast<const char**>(argv), argc, " "));
  char cwd[2048] = { 0, };
  fprintf(stderr, "CWD: [%s] CMD: [%s]\n",
          getcwd(cwd, sizeof(cwd)), full_command.c_str());

#if defined(HAVE_GLOG) && defined(USE_GLOG_LOGGING)
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif
#if defined(HAVE_GFLAGS)
  gflags::ParseCommandLineFlags(&argc, &argv, true);
#endif
  net::DnsInit();

  LOG_INFO << "Started : " << argv[0];
  LOG_INFO << "Command Line: " << full_command;
}

const char* ByteOrderName(ByteOrder order) {
  switch ( order ) {
    CONSIDER(BIGENDIAN);
    CONSIDER(LILENDIAN);
  default: LOG_FATAL << " Unsupported byte order: " << order;
  }
  return "Unknown";
}

}   // end namespace system
