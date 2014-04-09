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
// Authors: Catalin Popescu

// One simpletest for filereader - starts a command and pipes the input
// to our filereader. We print what we get from that and on close we kill the
// test.

// Example for test:
// ./selectable_filereader_test
//       --alsologtostderr
//       --cmd_path=./selectable_filereader_test.sh
//       --expected_bytes=27059
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/base/core_errno.h>

#include <whisperlib/net/selector.h>
#include <whisperlib/net/selectable_filereader.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(cmd_path,
              "",
              "Command for our test to pipe");

DEFINE_string(cmd_args,
              "",
              "Command for our test to pipe");

DEFINE_int32(expected_bytes,
             -1,
             "If set we check if the command generates these may bytes.");

//////////////////////////////////////////////////////////////////////

int64 total_bytes = 0;

void PrintFileData(io::MemoryStream* in) {
  string s;
  in->ReadString(&s);
  total_bytes += s.size();
  LOG_INFO << "READ: [" << s << "]";
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_cmd_path.empty())
    << " Please specify a command for the test !";
  int fd[2];
  CHECK_EQ(pipe(fd), 0);
  const int pid = fork();
  CHECK_NE(pid, -1);
  if ( pid == 0 ) {
    net::Selector selector;
    net::SelectableFilereader reader(&selector);
    CHECK(reader.InitializeFd(fd[0],
                              NewPermanentCallback(PrintFileData),
                              NewCallback(&selector,
                                          &net::Selector::MakeLoopExit)));
    selector.Loop();
    if ( FLAGS_expected_bytes >= 0 ) {
      CHECK_EQ(total_bytes, FLAGS_expected_bytes);
    }
    LOG_INFO << "PASS";
  } else {
    // redirect stdout to fd[1]
    close(fd[0]);
    close(1);
    CHECK_EQ(1, dup(fd[1]));
    execl(FLAGS_cmd_path.c_str(), FLAGS_cmd_args.c_str(), NULL);
    LOG_INFO << " Command pid ended !";
  }
}
