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

#include <stdio.h>
#include <whisperlib/io/checkpoint/checkpointing.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/base/util.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(dir,
              "/tmp",
              "Where to write test logs");

DEFINE_string(filebase,
              "testcheckpoint",
              "Write logs w/ this prefix");

DEFINE_int32(rand_seed,
             3274,
             "Seed the random number generator w/ this");

DEFINE_string(name_len,
              "10-30",
              "Interval: names have a random length within this interval.");

DEFINE_string(value_len,
              "0-300",
              "Interval: values have a random length within this interval.");

DEFINE_int32(num_values,
             10000,
             "Number of name/value pairs to store in checkpoint.");

//////////////////////////////////////////////////////////////////////

static const char kLetters[] =
  "1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_";
static unsigned int g_rand_seed = 0;
static util::Interval g_name_len(0,0);
static util::Interval g_value_len(0,0);

string RandomString(const util::Interval& interval) {
  const int len = interval.Rand(&g_rand_seed);
  char* tmp = new char[len+1];
  for ( int i = 0; i < len; ++i ) {
    tmp[i] = kLetters[rand_r(&g_rand_seed) % (sizeof(kLetters) - 1)];
  }
  tmp[len] = 0;
  string s(tmp);
  delete [] tmp;
  return s;
}

void GenerateCheckpoint(map<string,string>* out) {
  io::CheckpointWriter checkpointer(FLAGS_dir, FLAGS_filebase);
  CHECK(checkpointer.BeginCheckpoint() >= 0);
  for ( uint32 i = 0; i < FLAGS_num_values; i++ ) {
    string name = RandomString(g_name_len);
    string value = RandomString(g_value_len);
    if ( out != NULL ) {
      out->insert(make_pair(name, value));
    }
    checkpointer.AddRecord(name, value);
  }
  if ( out != NULL ) {
    out->insert(make_pair("empty", ""));
  }
  checkpointer.AddRecord("empty", "");
  CHECK(checkpointer.EndCheckpoint());
}
void VerifyCheckpoint(const map<string, string>& expected) {
  map<string, string> values;
  CHECK(io::ReadCheckpoint(FLAGS_dir, FLAGS_filebase, &values));
  CHECK(values.size() == expected.size()) << " Found " << values.size()
      << " values, expected: " << expected.size();
  for ( map<string, string>::const_iterator it = expected.begin();
        it != expected.end(); ++it ) {
    const string& name = it->first;
    const string& value = it->second;
    CHECK(values.find(name) != values.end()) << " Cannot find name: " << name;
    CHECK(values[name] == value) << " Name: " << name << ", found value: "
        << values[name] << " vs expected value: " << value;
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  g_rand_seed = FLAGS_rand_seed;
  srand(FLAGS_rand_seed);

  if ( !g_name_len.Read(FLAGS_name_len) ) {
    LOG_ERROR << "Invalid interval: FLAGS_name_len: [" << FLAGS_name_len << "]";
    common::Exit(1);
  }
  if ( !g_value_len.Read(FLAGS_value_len) ) {
    LOG_ERROR << "Invalid interval: FLAGS_value_len: [" << FLAGS_value_len << "]";
    common::Exit(1);
  }

  // cleanup before any test
  io::CleanCheckpointFiles(FLAGS_dir, FLAGS_filebase, 0);

  // Test1: write a checkpoint, read the checkpoint, verify data
  {
    map<string, string> cache;
    GenerateCheckpoint(&cache);
    VerifyCheckpoint(cache);
  }

  // cleanup between tests
  io::CleanCheckpointFiles(FLAGS_dir, FLAGS_filebase, 0);

  // Test2: write 2 checkpoints, load, verify the last one is loaded
  {
    map<string, string> cache;
    GenerateCheckpoint(NULL);
    GenerateCheckpoint(&cache);
    VerifyCheckpoint(cache);
  }

  // cleanup between tests
  io::CleanCheckpointFiles(FLAGS_dir, FLAGS_filebase, 0);

  // Test3: write 3 checkpoints, deliberately corrupt the 3nd, load,
  //        verify the 2nd is loaded
  {
    map<string, string> cache;
    GenerateCheckpoint(NULL);
    GenerateCheckpoint(&cache); // remember the 2nd
    GenerateCheckpoint(NULL);
    vector<string> files;
    io::GetCheckpointFiles(FLAGS_dir, FLAGS_filebase, &files);
    CHECK(files.size() == 3) << "Found " << files.size()
                             << " files, instead of 3";
    io::File corruptor;
    CHECK(corruptor.Open(strutil::JoinPaths(FLAGS_dir, files[2]),
        io::File::GENERIC_WRITE, io::File::OPEN_EXISTING));
    corruptor.Write(kLetters, sizeof(kLetters));
    corruptor.Close();
    LOG_WARNING << "Checkpoint [" << files[2] << "] corrupted on purpose."
                   " The CRC errors below are expected..";
    VerifyCheckpoint(cache);
  }
  printf("PASS\n");
  common::Exit(0);
}
