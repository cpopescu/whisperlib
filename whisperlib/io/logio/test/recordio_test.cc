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

// Simple tests for recordio

#include <stdio.h>

#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/timer.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/gflags.h>

#include <whisperlib/io/buffer/memory_stream.h>
#include <whisperlib/io/logio/recordio.h>

//////////////////////////////////////////////////////////////////////

DEFINE_int32(num_records,
             1000,
             "Number of records for the test");

DEFINE_int32(rand_seed,
             3274,
             "Seed the random number generator w/ this");

DEFINE_int32(block_size,
             16384,
             "Create blocks of this size");

DEFINE_int32(max_record_size,
             65536,
             "Generate records of at most this size");

DEFINE_bool(deflate,
            false,
            "Zip records for writing");

//////////////////////////////////////////////////////////////////////

static unsigned int g_rand_seed;

io::MemoryStream* GenerateRecord() {
  const int32 rec_size = rand_r(&g_rand_seed) %
                         (FLAGS_max_record_size / sizeof(rec_size));
  int32* buf = new int32[rec_size];
  for ( int32 i = 0; i < rec_size; ++i ) {
    buf[i] = rand_r(&g_rand_seed);
  }
  io::MemoryStream* rec = new io::MemoryStream();
  rec->Write(buf, rec_size * sizeof(*buf));
  delete [] buf;
  return rec;
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  g_rand_seed = FLAGS_rand_seed;
  srand(g_rand_seed);

  io::RecordWriter rw(FLAGS_block_size, FLAGS_deflate);
  vector<io::MemoryStream*> recs;
  io::MemoryStream out;

  for ( int32 i = 0; i < FLAGS_num_records; ++i ) {
    io::MemoryStream* rec = GenerateRecord();
    LOG_INFO << "#" << i << " New record: " << rec->Size() << " bytes";
    if ( rec->IsEmpty() ) {
      LOG_INFO << "================ > EMPTY record: " << i;
    }

    rec->MarkerSet();
    uint32 original_out_size = out.Size();
    bool new_block_generated = rw.AppendRecord(rec, &out);
    rec->MarkerRestore();
    if ( new_block_generated ) {
      uint32 add_size = out.Size() - original_out_size;
      CHECK(add_size % FLAGS_block_size == 0) << " add_size: " << add_size
           << " should be a multiple of block_size: " << FLAGS_block_size;
      uint32 nblocks = add_size / FLAGS_block_size;
      LOG_INFO << "Another " << nblocks << " blocks generated @"
               << original_out_size << ", add_size: " << add_size;
    } else {
      CHECK_EQ(out.Size(), original_out_size);
    }
    recs.push_back(rec);
  }
  rw.FinalizeContent(&out);
  LOG_INFO << "Final records size: " << out.Size();

  io::RecordReader rd(FLAGS_block_size);
  io::MemoryStream in;
  int32 rec_id = 0;
  while ( !out.IsEmpty() ) {
    int32 to_pull = min(rand_r(&g_rand_seed) % 2048, out.Size());
    in.AppendStream(&out, to_pull);
    while ( true ) {
      int num_skipped = 0;
      io::MemoryStream crt;
      io::RecordReader::ReadResult err = rd.ReadRecord(&in, &crt, &num_skipped, 0);
      if ( err == io::RecordReader::READ_NO_DATA ) {
        break;
      }
      CHECK(err == io::RecordReader::READ_OK) << " err: " << err
          << ", rec_id: " << rec_id;
      CHECK(recs[rec_id]->Equals(crt))
          << "\n recs[" << rec_id << "]: " << recs[rec_id]->DumpContentHex()
          << "\n crt: " << crt.DumpContentHex();
      delete recs[rec_id];
      ++rec_id;
    }
  }
  CHECK_EQ(rec_id, recs.size());
  LOG(INFO) << "PASS";
}
