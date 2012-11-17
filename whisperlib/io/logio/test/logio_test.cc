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

// Simple tests for logio module

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

DEFINE_string(test_dir,
              "/tmp",
              "Where to write test logs");

DEFINE_string(test_filebase,
              "testlog",
              "Write logs w/ this prefix");

DEFINE_int32(rand_seed,
             0,
             "Seed the random number generator w/ this. If 0, init w/ time().");

DEFINE_int64(num_records,
             70000,
             "Number of records for the test");

DEFINE_int32(block_size,
             16385,
             "Create blocks of this size");

DEFINE_int32(blocks_per_file,
             13,
             "Create files of this size (in blocks)");

DEFINE_int32(record_size,
             200,
             "Generate records of this size");

DEFINE_double(writer_stop_probability,
              0.001,
              "Reinitialize the writer w/ this probability");

DEFINE_double(reader_stop_probability,
              0.001,
              "Reinitialize the reader w/ this probability");

DEFINE_bool(deflate,
            false,
            "Zip records for writing");

//////////////////////////////////////////////////////////////////////

static unsigned int g_rand_seed = 0;

bool TestProbability(double prob) {
  return ::rand_r(&g_rand_seed) < prob * RAND_MAX;
}

void GenerateRecord(int64 rid, io::MemoryStream* rec) {
  io::NumStreamer::WriteInt64(rec, rid, common::kByteOrder);
  for ( uint32 i = 0; i < FLAGS_record_size - sizeof(int64); i++ ) {
    io::NumStreamer::WriteByte(rec, (uint8)i);
  }
}

int64 VerifyRecord(io::MemoryStream* rec) {
  CHECK_EQ(rec->Size(), FLAGS_record_size);
  int64 rid = io::NumStreamer::ReadInt64(rec, common::kByteOrder);
  CHECK_GE(rid, 0);
  for ( uint32 i = 0; i < FLAGS_record_size - sizeof(int64); i++ ) {
    uint8 b = io::NumStreamer::ReadByte(rec);
    CHECK_EQ(b, (uint8)i);
  }
  return rid;
}

void WriterThread() {
  string info = "writer";
  io::LogWriter* writer = new io::LogWriter(FLAGS_test_dir,
                                            FLAGS_test_filebase,
                                            FLAGS_block_size,
                                            FLAGS_blocks_per_file,
                                            false,
                                            FLAGS_deflate);
  CHECK(writer->Initialize());
  for ( int64 rid = 0; rid < FLAGS_num_records; ++rid ) {
    if ( TestProbability(FLAGS_writer_stop_probability) ) {
      LOG_WARNING << "Re-opening the writer at record: " << rid;
      delete writer;
      writer = new io::LogWriter(FLAGS_test_dir,
                                 FLAGS_test_filebase,
                                 FLAGS_block_size,
                                 FLAGS_blocks_per_file,
                                 false,
                                 FLAGS_deflate);
      CHECK(writer->Initialize());
    }
    // LOG_EVERY_N(INFO, 1000) << "Writing record: " << rid;
    io::MemoryStream rec;
    GenerateRecord(rid, &rec);
    CHECK(writer->WriteRecord(&rec));
  }
  delete writer;
}

void ReaderThread() {
  string info = "reader";
  io::LogReader* reader = new io::LogReader(FLAGS_test_dir.c_str(),
                                            FLAGS_test_filebase.c_str(),
                                            FLAGS_block_size,
                                            FLAGS_blocks_per_file);
  int64 crt_rid = 0;
  while ( crt_rid < FLAGS_num_records ) {
    io::LogPos pos(reader->Tell());
    if ( TestProbability(FLAGS_reader_stop_probability) ) {
      LOG_WARNING << "Re-opening the Reader at: " << pos.ToString();
      delete reader;
      reader = new io::LogReader(FLAGS_test_dir.c_str(),
                                 FLAGS_test_filebase.c_str(),
                                 FLAGS_block_size,
                                 FLAGS_blocks_per_file);
      if ( !pos.IsNull() ) {
        CHECK(reader->Seek(pos));
        CHECK(reader->Tell() == pos) << " AT: " << reader->Tell().ToString()
                                     << " vs. " << pos.ToString();
        VLOG(5) << "OK - > AT: " << reader->Tell().ToString();
      }
    }
    io::MemoryStream rec;
    if ( !reader->GetNextRecord(&rec) ) {
      // writer is behind, just wait for the next record..
      CHECK_EQ(reader->num_errors(), 0);
      continue;
    }
    CHECK(!rec.IsEmpty());
    // LOG_EVERY_N(INFO, 1000) << "Read record: " << crt_rid;
    int64 rid = VerifyRecord(&rec);
    CHECK(rid == crt_rid) << "Found rid: " << rid << ", expected: " << crt_rid;
    crt_rid++;
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  g_rand_seed = FLAGS_rand_seed;
  if ( g_rand_seed == 0 ) {
    g_rand_seed = ::time(NULL);
    LOG_INFO << "Random Seed initialized to: " << g_rand_seed;
  }
  srand(g_rand_seed);

  // First: a simple test that Seek works at EVERY position.
  {
    LOG_INFO << "Testing seek";
    LOG_INFO << "Clearing old files.."
             << system(strutil::StringPrintf(
                           "rm -f %s/%s_??????????_??????????",
                           FLAGS_test_dir.c_str(),
                           FLAGS_test_filebase.c_str()).c_str());
    // Generate a log with random records, remember every record position.
    io::LogWriter writer(FLAGS_test_dir,
                         FLAGS_test_filebase,
                         FLAGS_block_size,
                         FLAGS_blocks_per_file,
                         false,
                         FLAGS_deflate);
    CHECK(writer.Initialize());
    vector<io::LogPos> pos;
    for ( uint32 i = 0; i < 1000; i++ ) {
      io::MemoryStream rec;
      GenerateRecord(i, &rec);
      pos.push_back(writer.Tell());
      CHECK(writer.WriteRecord(&rec));
    }
    writer.Close();
    LOG_INFO << "Log generated, going to seek..";
    // The actual test: Seek to each position, read 1 record, verify it.
    for ( uint32 i = 0; i < 1000; i++ ) {
      io::LogReader reader(FLAGS_test_dir.c_str(),
                           FLAGS_test_filebase.c_str(),
                           FLAGS_block_size,
                           FLAGS_blocks_per_file);
      reader.Seek(pos[i]);
      CHECK(reader.Tell() == pos[i]) << "Requested seek to pos: "
          << pos[i].ToString() << ", got to pos: " << reader.Tell().ToString();
      io::MemoryStream rec;
      CHECK(reader.GetNextRecord(&rec));
      if ( i + 1 < 1000 && pos[i+1].record_num_ != 0 ) {
        CHECK(reader.Tell() == pos[i+1]) << "Next reader pos: "
            << reader.Tell().ToString()
            << ", expected: " << pos[i+1].ToString();
      }
      int64 rid = VerifyRecord(&rec);
      CHECK_EQ(rid, i);
      DLOG_INFO << "#" << i << " Seek to: " << pos[i].ToString() << " => OK";
    }
    LOG_INFO << "Test seek PASS";
  }

  // Second: a producer / consumer test
  LOG_INFO << "Clearing old files.."
           << system(strutil::StringPrintf(
                         "rm -f %s/%s_??????????_??????????",
                         FLAGS_test_dir.c_str(),
                         FLAGS_test_filebase.c_str()).c_str());
  LOG_INFO << "Creating writer thread..";
  thread::Thread writer(NewCallback(&WriterThread));
  writer.SetJoinable();
  writer.Start();

  LOG_INFO << "Creating reader thread..";
  thread::Thread reader(NewCallback(&ReaderThread));
  reader.SetJoinable();
  reader.Start();

  LOG_INFO << " Waiting to join !";
  writer.Join();
  reader.Join();
  LOG_INFO << " PASS";
}
