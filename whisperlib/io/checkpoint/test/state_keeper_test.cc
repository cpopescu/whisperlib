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

#include <stdio.h>
#include <whisperlib/io/checkpoint/state_keeper.h>
#include <whisperlib/io/ioutil.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/base/util.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(dir,
              "/tmp",
              "Where to write test logs");

DEFINE_string(file_base,
              "teststate",
              "Write logs w/ this prefix");

DEFINE_int32(block_size,
             128,
             "We create state logs w/ this size");

DEFINE_int32(blocks_per_file,
             500,
             "We create state logs w/ this size");

DEFINE_int32(rand_seed,
             3274,
             "Seed the random number generator w/ this");

DEFINE_string(name_len,
              "3-150",
              "Interval: state names have a random length in this interval");

DEFINE_string(value_len,
              "1-1500",
              "Interval: state values have a random length in this interval");

DEFINE_int32(num_ops,
             5000,
             "Number of operation per step. An op is: add,del,modify,..");

DEFINE_int32(num_steps,
             7,
             "On each step: a StateKeeper is created, some values are "
             "added/deleted/modified, the StateKeeper is destroyed, state data "
             "is stored on disk.");

//////////////////////////////////////////////////////////////////////

struct Action {
  enum ID {
    CHECKPOINT,
    TRANSACTION,
    ADD,
    DEL_EXISTENT,
    DEL_NONEXISTENT,
    MODIFY,
  };
  ID id_;
  // The chance of taking an action is proportioned with this value.
  uint32 chance_;
  Action(ID id, int32 chance) : id_(id), chance_(chance) {}
} g_actions[] = {
    Action(Action::CHECKPOINT, 1),
    Action(Action::TRANSACTION, 5),
    Action(Action::ADD, 70),
    Action(Action::DEL_EXISTENT, 20),
    Action(Action::DEL_NONEXISTENT, 20),
    Action(Action::MODIFY, 200)
};

static unsigned int g_rand_seed = 0;
static util::Interval g_name_len(0,0);
static util::Interval g_value_len(0,0);

// generate a random string, with length: random in 'interval'
string RandomString(const util::Interval& interval) {
  static const char kLetters[] =
    "1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_";
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

// print these many characters of each string value
static const uint32 kShortStringLen = 32;
inline ostream& operator<<(ostream& os, const string& s) {
  if ( s.size() < kShortStringLen ) {
    return os << s.c_str();
  }
  return os << s.substr(0, kShortStringLen).c_str() << "...";
}

// Choose a random action, based on each action chance.
Action::ID RandomAction() {
  static uint32 g_total_chances = 0;
  if ( g_total_chances == 0 ) {
    for ( uint32 i = 0; i < NUMBEROF(g_actions); i++ ) {
      g_total_chances += g_actions[i].chance_;
    }
  }
  int32 rnd = ::rand_r(&g_rand_seed) % g_total_chances;
  for ( uint32 i = 0; i < NUMBEROF(g_actions); i++ ) {
    rnd -= g_actions[i].chance_;
    if ( rnd <= 0 ) {
      return g_actions[i].id_;
    }
  }
  LOG_FATAL << "g_total_chances: " << g_total_chances << ", rnd: " << rnd;
  return Action::ADD;
}

// state: a duplicate of the StateKeeper
// names: contains all names that have ever been in state. (i.e. current names
//        in 'state' + old names that were deleted from state). Used to verify
//        that the old names were indeed deleted.
void PerformStep(map<string, string>& state, vector<string>& names) {
  io::StateKeeper sk(FLAGS_dir,
                     FLAGS_file_base,
                     FLAGS_block_size,
                     FLAGS_blocks_per_file);
  CHECK(sk.Initialize());

  // Verify the 'state' map is identical with the StateKeeper
  LOG_WARNING << "Initial verification: names.size(): " << names.size();
  for ( int i = 0; i < names.size(); ++i ) {
    if ( state.find(names[i]) != state.end() ) {
      string val;
      CHECK(sk.GetValue(names[i], &val)) << " key: " << names[i];
      CHECK(state[names[i]] == val) << "i: " << i
          << ", state[" << names[i] << "] = [" << state[names[i]]
          << "], found in keeper: [" << val << "]";
    } else {
      CHECK(!sk.HasValue(names[i])) << " key: " << names[i];
    }
  }
  // count modifications, just for log
  int32 records_added_since_checkpoint = 0;
  int32 records_modified_since_checkpoint = 0;
  bool in_transaction = false;
  for ( int i = 0; i < FLAGS_num_ops; ++i ) {
    const Action::ID op = RandomAction();
    switch ( op ) {
      case Action::CHECKPOINT:
        LOG_INFO << "OP Checkpointing " << names.size()
                 << " values, added since checkpoint: "
                 << records_added_since_checkpoint
                 << ", modified since checkpoint: "
                 << records_modified_since_checkpoint;
        if ( in_transaction ) {
          LOG_INFO << "CommitTransaction before checkpoint";
          sk.CommitTransaction();
          in_transaction = false;
        }
        CHECK(sk.Checkpoint());
        records_added_since_checkpoint = 0;
        records_modified_since_checkpoint = 0;
        break;
      case Action::TRANSACTION:
        if ( in_transaction ) {
          LOG_INFO << "OP CommitTransaction";
          sk.CommitTransaction();
          in_transaction = false;
        } else {
          LOG_INFO << "OP BeginTransaction";
          sk.BeginTransaction();
          in_transaction = true;
        }
        break;
      case Action::DEL_EXISTENT:
        if ( !names.empty() ) {
          int rand_ndx = ::rand_r(&g_rand_seed) % names.size();
          const string& key = names[rand_ndx];
          if ( state.find(key) == state.end() ) {
            CHECK(!sk.HasValue(key));
            continue;
          }
          DLOG_INFO << "OP DeleteValue: [" << key << "]";
          sk.DeleteValue(key);
          int32 count = state.erase(key);
          CHECK_EQ(count, 1);
        }
        break;
      case Action::DEL_NONEXISTENT: {
          const string key = RandomString(g_name_len);
          if ( state.find(key) != state.end() ) {
            CHECK(sk.HasValue(key));
            LOG_ERROR << "Go play lottery: " << key;
            continue;
          }
          DLOG_INFO << "OP DeleteValue: [" << key << "]";
          sk.DeleteValue(key);
          int32 count = state.erase(key);
          CHECK_EQ(count, 0);
        }
        break;
      case Action::ADD: {
          const string k = RandomString(g_name_len);
          if ( state.find(k) != state.end() ) {
            CHECK(sk.HasValue(k));
            LOG_ERROR << "Go play lottery: " << k;
            continue;
          }
          const string val = RandomString(g_value_len);
          DLOG_INFO << "OP AddValue: [" << k << "] -> [" << val << "]";
          records_added_since_checkpoint++;
          CHECK(sk.SetValue(k, val));
          names.push_back(k);
          state[k] = val;
        }
        break;
      case Action::MODIFY: {
          if ( names.empty() ) {
            continue;
          }
          const string key = names[rand_r(&g_rand_seed) % names.size()];
          const string val = RandomString(g_value_len);
          records_modified_since_checkpoint++;
          DLOG_INFO << "OP ModifyValue: [" << key << "] -> [" << val << "]";
          CHECK(sk.SetValue(key, val));
          state[key] = val;
        }
        break;
    }
  }
  // complete transaction before ending this step
  if ( in_transaction ) {
    LOG_INFO << "CommitTransaction before destroying StateKeeper";
    sk.CommitTransaction();
    in_transaction = false;
  }
  // Verify again that the 'state' map is identical with the StateKeeper
  LOG_WARNING << "Final verification: names.size(): " << names.size()
              << ", records_added_since_checkpoint: "
              << records_added_since_checkpoint
              << ", records_modified_since_checkpoint: "
              << records_modified_since_checkpoint;
  for ( size_t i = 0; i < names.size(); ++i ) {
    if ( state.find(names[i]) != state.end() ) {
      string val;
      CHECK(sk.GetValue(names[i], &val)) << " key: " << names[i];
      CHECK(state[names[i]] == val)
          << "state[" << names[i] << "]: " << state[names[i]]
          << ", expected: " << val;
    } else {
      CHECK(!sk.HasValue(names[i])) << " key: " << names[i];
    }
  }
}

void PerformTimingTest() {
  io::StateKeeper sk(FLAGS_dir.c_str(),
                     FLAGS_file_base.c_str(),
                     FLAGS_block_size,
                     FLAGS_blocks_per_file);
  CHECK(sk.Initialize());

  io::StateKeepUser sku1(&sk, "gigi_marga/", 5000);
  io::StateKeepUser sku2(&sk, "vasile_ionica/", 10000);
  sku1.SetValue("key", "10");
  sku2.SetValue("key", "11");
  string value;
  CHECK(sku1.GetValue("key", &value));
  CHECK_EQ(value, "10");
  CHECK(sku2.GetValue("key", &value));
  CHECK_EQ(value, "11");

  ::sleep(6);

  CHECK(sku1.GetValue("key", &value));
  CHECK_EQ(value, "10");
  CHECK_EQ(sk.ExpireTimeoutedKeys(), 1);
  CHECK(!sku1.GetValue("key", &value));
  CHECK(sku2.GetValue("key", &value));
  CHECK_EQ(value, "11");

  ::sleep(6);

  CHECK(sku2.GetValue("key", &value));
  CHECK_EQ(value, "11");
  CHECK_EQ(sk.ExpireTimeoutedKeys(), 1);
  CHECK(!sku1.GetValue("key", &value));
  CHECK(!sku2.GetValue("key", &value));

  CHECK(sk.data().empty());
}

void ClearAllFiles() {
  vector<string> files;
  re::RE re(strutil::StringPrintf("%s_.*", FLAGS_file_base.c_str()));
  io::DirList(FLAGS_dir, io::LIST_FILES, &re, &files);
  LOG_INFO << "Clearing " << files.size() << " old files..";
  for ( uint32 i = 0; i < files.size(); i++ ) {
    io::Rm(strutil::JoinPaths(FLAGS_dir, files[i]));
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

  ClearAllFiles();

  LOG_INFO << "Starting timing test ";
  LOG_WARNING << "Timing test disabled!";
  //PerformTimingTest();
  LOG_INFO << "Ended  timing test ";

  ClearAllFiles();

  map<string, string> state;
  vector<string> names;
  for ( int i = 0; i < FLAGS_num_steps; ++i ) {
    LOG_INFO << "============= STEP : " << i;
    PerformStep(state, names);
  }
  printf("PASS\n");
}
