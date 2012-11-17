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

#include <whisperlib/io/checkpoint/checkpointing.h>
#include <whisperlib/base/gflags.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "Read checkpoint from this file.");

DEFINE_string(out,
              "",
              "Write checkpoint (with imports) into this file.");

DEFINE_bool(print,
            false,
            "Print the content of the checkpoint");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  //////////////////////////////////////////////////////////////////////////

  map<string, string> checkpoint;

  // maybe read input checkpoint
  if ( FLAGS_in != "" ) {
    if ( !io::ReadCheckpointFile(FLAGS_in, &checkpoint) ) {
      LOG_ERROR << "Failed to read checkpoint file: [" << FLAGS_in << "]";
      common::Exit(1);
    }
    LOG_INFO << "Successfully read checkpoint file: [" << FLAGS_in << "]";
  }

  // maybe print the checkpoint
  if ( FLAGS_print ) {
    for ( map<string, string>::const_iterator it = checkpoint.begin();
          it != checkpoint.end(); ++it ) {
      LOG_INFO << "Name: [" << it->first << "]";
      LOG_INFO << "Value: [" << it->second << "]";
    }
  }

  // maybe write checkpoint to file
  if ( FLAGS_out != "" ) {
    if ( !io::WriteCheckpointFile(checkpoint, FLAGS_out) ) {
      LOG_ERROR << "Failed to WriteCheckpointFile, to: [" << FLAGS_out << "]";
      common::Exit(1);
    }
  }

  LOG_INFO << "Done #" << checkpoint.size() << " pairs";

  common::Exit(0);
}
