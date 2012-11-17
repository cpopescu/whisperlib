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

// A class that keeps persistently a state.
//
// !!! NOT THREAD SAFE !!!
//

#ifndef __COMMON_IO_CHECKPOINT_STATE_KEEPER_H__
#define __COMMON_IO_CHECKPOINT_STATE_KEEPER_H__

#include <pthread.h>
#include <map>
#include <string>
#include <whisperlib/io/logio/logio.h>
#include <whisperlib/io/checkpoint/checkpointing.h>
#include <whisperlib/sync/thread.h>
#include <whisperlib/sync/producer_consumer_queue.h>

namespace io {

static const int32 kDefaultStateKeeperBlockSize = 128;
static const int32 kDefaultStateKeeperBlocksPerFile = 100000;
static const int32 kMinNumCheckpointsToKeep = 2;
static const int32 kDefaultCheckpointsToKeep = 4;

class StateKeeper {
 public:
  static string CheckpointFilename(const string& state_name) {
    return state_name + "_checkpoint";
  }
  static string LogFilename(const string& state_name) {
    return state_name + "_statelog";
  }
  // Read StateKeeper state: a map of string -> string, output to 'out'.
  // ops_log_level: log add/del/modify operations on this level (just for debug)
  // returns success.
  static bool ReadState(const string& state_dir, const string& state_name,
      map<string, string>* out, bool ops_log,
      int32 block_size, int32 blocks_per_file);

 public:
  // take care in giving block_size / blocks_per_file parameters !
  // on this depends the wasted space ..
  StateKeeper(const string& state_dir,
              const string& state_name,
              int32 block_size = kDefaultStateKeeperBlockSize,
              int32 blocks_per_file = kDefaultStateKeeperBlocksPerFile,
              int32 checkpoints_to_keep = kDefaultCheckpointsToKeep);
  ~StateKeeper();

  const string& state_dir() const { return state_dir_; }
  const string& state_name() const { return state_name_; }

  // After creating a state keeper use this to actually read the previous
  // data saved in the state.
  bool Initialize();

  // Use this to write a checkpoint of the state from time to time ..
  // Also cleans the old checkpoints and state logs (up to checkpoints_to_keep)
  bool Checkpoint();

  // Use these to update the state. Returns success status (according to
  // the log writing success)
  bool SetValue(const string& key, const string& value);

  // Use this to delete the value corresponding to the given key from the map
  bool DeleteValue(const string& key);

  // Use this to delete values from the state that begin with given prefix
  bool DeletePrefix(const string& prefix);

  // Delete old checkpoints and log files (we protect at least
  // kMinNumCheckpointsToKeep checkpoints.. )
  void CleanOldState(int32 num_checkpoints_to_keep);

  // A simple transaction mechanism to write a number of changes
  // at once. Once a transaction begins all changes are accumulated in internal
  // structures in memory but are *not* committed to the disk.
  //
  // RESTRICTION :: You cannot abort a transaction !!
  //
  // RESTRICTION :: do not checkpoint while a transaction is active !!
  //
  void BeginTransaction() {
    CHECK(!in_transaction_);
    in_transaction_ = true;
  }
  void CommitTransaction() {
    CHECK(in_transaction_);
    in_transaction_ = false;
    QueueWrite();
  }

  ///////////////////////////////////////////////////////////////////////
  //
  //  State access
  //

  bool HasValue(const string& key) const {
    return data_.find(key) != data_.end();
  }
  const map<string, string>& data() const { return data_; }

  bool GetValue(const string& key, string* value) const {
    map<string, string>::const_iterator it = data_.find(key);
    if ( it == data_.end() ) return false;
    *value = it->second;
    return true;
  }
  uint32 Size() const {
    return data_.size();
  }

  // Returns iterators for a range of keys (the ones that begin with the given
  // prefix).
  void GetBounds(const string& prefix,
                 map<string, string>::const_iterator* begin,
                 map<string, string>::const_iterator* end) const;

  void GetBounds(const string& prefix,
                 map<string, string>::iterator* begin,
                 map<string, string>::iterator* end);

  // Expires all the timeouted prefixes (under __timeout__/...)
  // Returns the number of expired prefixes
  int ExpireTimeoutedKeys();

 private:
  string info() const {
    return "[StateKeeper " + state_dir_ + " | " + state_name_ + "]: ";
  }
  struct WriteThreadCommand {
    string* log_data_;
    map<string, string>* checkpoint_data_;
    WriteThreadCommand(string* log_data, map<string, string>* checkpoint_data)
        : log_data_(log_data), checkpoint_data_(checkpoint_data) {
    }
    ~WriteThreadCommand() {
      delete log_data_;
      delete checkpoint_data_;
    }
  };

  void WriterThread();
  bool CheckpointInternal(map<string, string>* data);
  void QueueWrite() {
    string* s = new string();
    op_buf_.ReadString(s);
    writer_queue_.Put(new WriteThreadCommand(s, NULL));
  }
  enum OpCode {
    OP_SET = 0,
    OP_DELETE = 1,
    OP_CLEARPREFIX = 2,
  };
  static const char* OpCodeName(OpCode op);
  // Helper - erases all keys starting w/ given prefix - returns
  // the number of deleted keys
  void ClearMapPrefix(const string& prefix);
  // Helper for preparing op_buf_ w/ key and op
  void QueueOp(const string& key, OpCode op, const string* value = NULL);

 private:
  // protect against multiple threads
  pthread_t tid_;

  const string state_dir_;
  const string state_name_;
  const string checkpoint_name_;
  const string log_name_;
  const int32 block_size_;
  const int32 blocks_per_file_;
  const int32 checkpoints_to_keep_;

  // Access these two only from writer thread
  CheckpointWriter checkpointer_;
  LogWriter log_writer_;

  map<string, string> data_;

  bool in_transaction_;
  io::MemoryStream op_buf_;

  thread::Thread* writer_thread_;
  static const int kMaxWriterQueueSize = 1000;

  synch::ProducerConsumerQueue<WriteThreadCommand*> writer_queue_;
  static const int kFlushIntervalMs = 10000;


  DISALLOW_EVIL_CONSTRUCTORS(StateKeeper);
};


class StateKeepUser {
 public:
  StateKeepUser(StateKeeper* state_keeper,
                const string& prefix,
                int timeout_ms);
  ~StateKeepUser();

  string prefix() const { return prefix_; }

  bool SetValue(const string& key, const string& value) {
    if ( timeout_ms_ == 0 ) {
      return true;   // we basicaly don't save state ..
    }
    if ( state_keeper_->SetValue(prefix_ + key, value) ) {
      return (timeout_ms_ < 0) || StateKeepUser::UpdateTimeout();
    }
    return false;
  }

  bool DeleteValue(const string& key) {
    return state_keeper_->DeleteValue(prefix_ + key);
  }
  bool DeletePrefix(const string& prefix) {
    if ( prefix.empty() ) {
      CleanTimeout();
    }
    return state_keeper_->DeletePrefix(prefix_ + prefix);
  }
  bool DeleteAllValues() {
    return state_keeper_->DeletePrefix(prefix_);
  }
  void GetBounds(const string& prefix,
                 map<string, string>::const_iterator* begin,
                 map<string, string>::const_iterator* end) {
    state_keeper_->GetBounds(prefix_ + prefix, begin, end);
  }
  bool GetValue(const string& key, string* value) const {
    return state_keeper_->GetValue(prefix_ + key, value);
  }
  bool HasValue(const string& key) const {
    return state_keeper_->HasValue(prefix_ + key);
  }
  void BeginTransaction() {
    state_keeper_->BeginTransaction();
  }
  void CommitTransaction() {
    StateKeepUser::UpdateTimeout();
    state_keeper_->CommitTransaction();
  }
  int64 timeout_ms() const {
    return timeout_ms_;
  }
  // You can set this once, and *only* from unset value
  void set_timeout_ms(int64 timeout_ms) {
    // CHECK_EQ(timeout_ms_, 0);
    timeout_ms_ = timeout_ms;
  }
 private:
  string info() const {
    return "[StateKeeperUser " + state_keeper_->state_dir() + " | " +
           state_keeper_->state_name() + " | " + prefix_ + "]: ";
  }

  bool UpdateTimeout();
  bool CleanTimeout();

  // We don't update timeouts if they fall withing this interval from the
  // old one.
  static const int64 kMinUpdateTimeoutThresholdMs = 1000;   // 1 secs

  StateKeeper* const state_keeper_;
  const string prefix_;
  // A positive timeout will make the entire prefix expire at these many
  // milliseconds after last value is set.
  int64 timeout_ms_;
  // The running  absolute timeout for this user..
  int64 crt_timeout_;
  DISALLOW_EVIL_CONSTRUCTORS(StateKeepUser);
};

// Autodetect StateKeeper parameters
bool DetectStateKeeperSettings(const string& log_dir, string* out_file_base,
    int32* out_block_size, int32* out_blocks_per_file);
}

#endif  // __COMMON_IO_CHECKPOINT_STATE_KEEPER_H__
