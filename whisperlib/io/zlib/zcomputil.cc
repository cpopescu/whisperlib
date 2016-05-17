// Copyright (c) 2014 Urban Engines inc.
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
//

#include <stdio.h>
#include "whisperlib/io/zlib/zlibwrapper.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/base/util.h"
#include "whisperlib/base/system.h"

DEFINE_string(d, "", "Directory to compact / where to expand");
DEFINE_string(f, "", "File to expand");
DEFINE_bool(compact, false, " Want to compact d->f ?");
DEFINE_bool(expand, false, " Want to expand f->d ?");
DEFINE_bool(append, false, " Append to f when compacting ");
DEFINE_int32(level, 5, "Level of compaction");
DEFINE_int32(chunk_size, whisper::io::kCompressMaxChunkSize, "Chunk files to these sizes");

int main(int argc, char* argv[]) {
  whisper::common::Init(argc, argv);
  CHECK(!FLAGS_d.empty()) << "Please specify a directory w/ -d";
  CHECK(!FLAGS_f.empty()) << "Please specify a  file w/ -f";
  if (FLAGS_compact) {
      whisper::io::ZlibDeflateWrapper defl(FLAGS_level);
      whisper::io::DeflateDir(&defl, FLAGS_d, NULL, FLAGS_f, FLAGS_append, FLAGS_chunk_size);
  } else if (FLAGS_expand) {
      whisper::io::ZlibInflateWrapper infl;
      whisper::io::InflateToDir(&infl, FLAGS_f, FLAGS_d);
  }
}
