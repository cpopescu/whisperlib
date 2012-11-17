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

// Analyze a logio directory.

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/timer.h>
#include <whisperlib/base/system.h>
#include <whisperlib/sync/thread.h>
#include <whisperlib/base/gflags.h>

#include <whisperlib/io/buffer/memory_stream.h>
#include <whisperlib/io/ioutil.h>
#include <whisperlib/io/logio/logio.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(dir,
              "",
              "The directory to analyze");
DEFINE_string(file_base,
              "",
              "logio file_base. If empty, use autodetect.");
DEFINE_int32(block_size,
             0,
             "logio blocks size. If 0, use autodetect.");
DEFINE_int32(blocks_per_file,
             0,
             "logio blocks per file. If 0, use autodetect.");
DEFINE_bool(log_records_text,
            false,
            "Log found records as strings.");
DEFINE_bool(log_records_hex,
            false,
            "Log found records in hex.");
DEFINE_bool(log_records_inline,
            false,
            "Log found records inline hex.");


//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  //////////////////////////////////////////////////////////////////////////
  // auto-detect part
  string file_base = FLAGS_file_base;
  int32 block_size = FLAGS_block_size;
  int32 blocks_per_file = FLAGS_blocks_per_file;
  if ( file_base == "" || block_size == 0 || blocks_per_file == 0 ) {
    string f;
    int32 bs, bpf;
    if ( !io::DetectLogSettings(FLAGS_dir, &f, &bs, &bpf) ) {
      LOG_ERROR << "DetectLogSettings failed for dir: [" << FLAGS_dir << "]";
      common::Exit(1);
    }
    if ( file_base == "" ) {
      file_base = f;
      LOG_INFO << "Detected file_base: [" << file_base << "]";
    }
    if ( block_size == 0 ) {
      block_size = bs;
      LOG_INFO << "Detected blocksize: " << block_size;
    }
    if ( blocks_per_file == 0 ) {
      blocks_per_file = bpf;
      LOG_INFO << "Detected blocks_per_file: " << blocks_per_file;
    }
  }
  if ( file_base == "" || block_size == 0 || blocks_per_file == 0 ) {
    LOG_ERROR << "Invalid settings, file_base: [" << file_base << "]"
                 ", block_size: " << block_size
              << ", blocks_per_file: " << blocks_per_file;
    common::Exit(1);
  }
  ::sleep(2);
  //////////////////////////////////////////////////////////////////////////


  io::LogReader reader(FLAGS_dir, file_base, block_size, blocks_per_file);
  uint64 num_records = 0;

  while ( true ) {
    io::LogPos pos = reader.Tell();
    io::MemoryStream rec;
    if ( !reader.GetNextRecord(&rec) ) {
      break;
    }
    uint32 rec_size = rec.Size();

    string content;
    if ( FLAGS_log_records_text ) {
      rec.ReadString(&content);
      content = strutil::StrEscape(content, '%', "");
    }
    if ( FLAGS_log_records_hex ) {
      content = rec.DumpContentHex();
    }
    if ( FLAGS_log_records_inline ) {
      content = rec.DumpContentInline();
    }

    LOG_INFO << "Record: " << rec_size << " bytes at pos: " << pos.ToString()
             << (content.empty() ? "" : (" Content: " + content).c_str());

    num_records++;
  }
  LOG_INFO << "Done. #" << num_records << " records found.";

  common::Exit(0);
}
