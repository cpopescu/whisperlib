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

#include <list>
#include <whisperlib/base/date.h>
#include <whisperlib/base/scoped_ptr.h>
#include <whisperlib/base/strutil.h>
#include <whisperlib/io/ioutil.h>
#include <whisperlib/io/checkpoint/state_keeper.h>

#define ILOG_DEBUG   DLOG_INFO << info() << ": "
#define ILOG_INFO    LOG_INFO << info() << ": "
#define ILOG_WARNING LOG_WARNING << info() << ": "
#define ILOG_ERROR   LOG_ERROR << info() << ": "
#define ILOG_FATAL   LOG_FATAL << info() << ": "

namespace {
static const char kStateKeeperLogPosKey[] = "__checkpoint_pos__";
static const char kStateKeeperTimeoutKey[] = "__t__";
static const string kCheckpointPrefix = "_checkpoint";
static const string kLogPrefix = "_statelog";
}

namespace io {

bool StateKeeper::ReadState(const string& state_dir, const string& state_name,
    map<string, string>* out, bool ops_log, int32 block_size,
    int32 blocks_per_file) {
  string info = "[StateKeeper " + state_dir + " | " + state_name + "]: ";
  out->clear();
  LogPos pos;
  if ( io::ReadCheckpoint(state_dir, CheckpointFilename(state_name), out) ) {
    map<string, string>::iterator it = out->find(kStateKeeperLogPosKey);
    if ( it == out->end() ) {
      LOG_ERROR << info << "Checkpoint totally fucked up";
      return false;
    }
    const string& str_pos = it->second;
    if ( !pos.StrDecode(str_pos) ) {
      LOG_ERROR << info << "Checkpoint totally fucked up";
      return false;
    }
  }
  out->erase(kStateKeeperLogPosKey);
  int32 num_keys = 0;
  LOG_INFO << info << "State Keeper initialized from checkpoint with "
           << out->size() << " keys" << ", reading change log.. seeking to: "
           << pos.ToString();
  LogReader reader(state_dir, LogFilename(state_name),
                   block_size, blocks_per_file);
  if ( !reader.Seek(pos) ) {
    LOG_ERROR << info << "Cannot seek to position: " << pos.ToString();
    return true;
  }
  io::MemoryStream buf;
  while ( reader.GetNextRecord(&buf) ) {
    while ( !buf.IsEmpty() ) {
      const int16 name_size =
          io::NumStreamer::ReadInt16(&buf, common::BIGENDIAN);
      string name;
      const int32 cb = buf.ReadString(&name, name_size);
      if ( name_size != cb ) {
        LOG_ERROR << info << "Fucked up size for name: [" << name << "]"
                     ", expected: " << name_size << ", found: " << cb;
        continue;
      }
      if ( buf.Size() < 1 ) {
        LOG_ERROR << info << "OpCode missing";
        continue;
      }
      const OpCode op_code = OpCode(io::NumStreamer::ReadByte(&buf));
      switch ( op_code ) {
        case OP_SET: {
          if ( buf.Size() < sizeof(int32) ) {
            LOG_ERROR << info << "Invalid bufer - missing value size";
            continue;
          }
          const int32 value_size =
              io::NumStreamer::ReadInt32(&buf, common::BIGENDIAN);
          string value;
          const int32 cb = buf.ReadString(&value, value_size);
          if ( value_size != cb ) {
            LOG_ERROR << info << "Fucked up size for value: [" << value << "]"
                         ", expected: " << value_size << ", found: " << cb;
            continue;
          }
          LOG_INFO_IF(ops_log) << info << "OP_SET [" << name << "] = ["
                                << value << "]";
          num_keys++;
          (*out)[name] = value;
        }
          break;
        case OP_DELETE:
          LOG_INFO_IF(ops_log) << info << "OP_DELETE [" << name << "]";
          out->erase(name);
          break;
        case OP_CLEARPREFIX: {
          LOG_INFO_IF(ops_log) << info << "OP_CLEARPREFIX [" << name << "]";
          map<string, string>::iterator begin;
          map<string, string>::iterator end;
          strutil::GetBounds(name, out, &begin, &end);
          out->erase(begin, end);
        }
          break;
        default:
          LOG_ERROR << info << "Ignore invalid operation: " << op_code;
      }
    }
  }
  LOG_INFO << info << "State Keeper initialized"
           << " with " << out->size() << " data keys "
           << " out of which " << num_keys << " changed post-checkpoint.";

  return true;
}

StateKeeper::StateKeeper(const string& state_dir,
                         const string& state_name,
                         int32 block_size,
                         int32 blocks_per_file,
                         int32 checkpoints_to_keep)
    : tid_(0),
      state_dir_(state_dir),
      state_name_(state_name),
      checkpoint_name_(state_name + kCheckpointPrefix),
      log_name_(state_name + kLogPrefix),
      block_size_(block_size),
      blocks_per_file_(blocks_per_file),
      checkpoints_to_keep_(checkpoints_to_keep),
      checkpointer_(state_dir, checkpoint_name_),
      log_writer_(state_dir, log_name_,
                  block_size, blocks_per_file, false, false),
      in_transaction_(false),
      writer_thread_(NULL),
      writer_queue_(kMaxWriterQueueSize) {
}

StateKeeper::~StateKeeper() {
  DCHECK_EQ(tid_, pthread_self());
  if ( in_transaction_ ) {
    ILOG_WARNING << "Aborting a transaction on destructor.";
  }
  if ( writer_thread_ != NULL ) {
    writer_queue_.Put(new WriteThreadCommand(NULL, NULL));
    writer_thread_->Join();
  }
  delete writer_thread_;
}

bool StateKeeper::Initialize() {
  CHECK_EQ(tid_, static_cast<pthread_t>(0));
  tid_ = pthread_self();

  if ( !ReadState(state_dir_, state_name_, &data_, false,
                  block_size_, blocks_per_file_) ) {
    ILOG_ERROR << "Failed to read state from disk";
    return false;
  }

  if ( !log_writer_.Initialize() ) {
    ILOG_ERROR << "Failed to initialize log_writer";
    return false;
  }
  CHECK_NULL(writer_thread_);
  writer_thread_ = new thread::Thread(NewCallback(this,
      &StateKeeper::WriterThread));
  writer_thread_->SetJoinable();
  writer_thread_->Start();

  return true;
}

void StateKeeper::WriterThread() {
  ILOG_INFO << "Writer thread started";
  while ( true ) {
    WriteThreadCommand* crt = writer_queue_.Get(kFlushIntervalMs);
    scoped_ptr<WriteThreadCommand> auto_del_crt(crt);

    if ( crt == NULL  ) {
      log_writer_.Flush();
      continue;
    }
    if ( crt->log_data_ == NULL && crt->checkpoint_data_ == NULL  ) {
      log_writer_.Flush();
      ILOG_INFO << "Writer thread ended";
      return;
    }
    if ( crt->log_data_ != NULL ) {
      log_writer_.WriteRecord(crt->log_data_->data(),
                              crt->log_data_->size());
    }
    if ( crt->checkpoint_data_ != NULL ) {
      CheckpointInternal(crt->checkpoint_data_);
      ILOG_INFO << "Done checkpoint w/ " << crt->checkpoint_data_->size()
                << " keys.";
    }
  }
}

bool StateKeeper::CheckpointInternal(map<string, string>* data) {
  DCHECK(writer_thread_->IsInThread());
  if ( !log_writer_.Flush() ) {
    ILOG_ERROR << "Error flushing state log writer";
    return false;
  }
  LogPos pos = log_writer_.Tell();
  if ( checkpointer_.BeginCheckpoint() < 0 ) {
    ILOG_ERROR << "Error beginning checkpoint";
    return false;
  }
  if ( !checkpointer_.AddRecord(kStateKeeperLogPosKey, pos.StrEncode()) ) {
    ILOG_ERROR << "Error adding log pos to checkpoint";
    return false;
  }
  for ( map<string, string>::const_iterator it = data->begin();
        it != data->end(); ++it ) {
    if ( !checkpointer_.AddRecord(it->first, it->second) ) {
      ILOG_ERROR << "Error adding a value in checkpoint";
      return false;
    }
  }
  if ( !checkpointer_.EndCheckpoint() ) {
    ILOG_ERROR << "Error ending checkpoint";
    return false;
  }
  ILOG_INFO << "After checkpoint at pos: " << pos.ToString();
  CleanOldState(checkpoints_to_keep_);
  return true;
}

bool StateKeeper::Checkpoint() {
  DCHECK_EQ(tid_, pthread_self());
  CHECK(!in_transaction_) << "DO NOT checkpoint while in transaction..";
  ExpireTimeoutedKeys();
  map<string, string>* data = new map<string, string>(data_);
  writer_queue_.Put(new WriteThreadCommand(NULL, data));
  return true;
}

void StateKeeper::CleanOldState(int num_checkpoints_to_keep) {
  DCHECK(writer_thread_->IsInThread());
  if ( num_checkpoints_to_keep < kMinNumCheckpointsToKeep ) {
    num_checkpoints_to_keep = kMinNumCheckpointsToKeep;
  }
  vector<string> v;
  if ( !io::GetCheckpointFiles(state_dir_, checkpoint_name_, &v) ) {
    ILOG_ERROR << "GetCheckpointFiles failed, state_dir_: [" << state_dir_
               << "], checkpoint_name_: [" << checkpoint_name_ << "]";
    return;
  }
  list<string> files(v.begin(), v.end());

  // first: delete corrupted checkpoints
  for ( list<string>::iterator it = files.begin(); it != files.end(); ) {
    const string filename = strutil::JoinPaths(state_dir_, *it);
    map<string, string> checkpoint;
    if ( !ReadCheckpointFile(filename, &checkpoint) ) {
      LOG_WARNING << "Delete corrupted checkpoint: " << filename;
      if ( !io::Rm(filename) ) {
        ILOG_ERROR << "Error removing file: [" << filename << "]";
      }
      list<string>::iterator it_to_del = it;
      ++it;
      files.erase(it_to_del);
      continue;
    }
    ++it;
  }

  // second: delete old checkpoints
  const int32 limit = files.size() - num_checkpoints_to_keep;
  for ( int32 i = 0; i < limit; ++i ) {
    const string filename = strutil::JoinPaths(state_dir_, files.front());
    files.erase(files.begin());

    map<string, string> checkpoint;
    ReadCheckpointFile(filename, &checkpoint);

    if ( !io::Rm(filename) ) {
      ILOG_ERROR << "Error removing file: [" << filename << "]";
    }

    map<string, string>::const_iterator const
      it = checkpoint.find(kStateKeeperLogPosKey);
    LogPos pos;
    if ( it != data_.end() && pos.StrDecode(it->second) ) {
      const int32 num_cleared_logs = io::CleanLog(
          state_dir_, log_name_, pos, block_size_);
      ILOG_INFO << "State cleaned old logs " << num_cleared_logs << " files"
                << " for: " << filename
                << " pos: " << pos.ToString();
    }
  }
}

void StateKeeper::QueueOp(const string& key, OpCode op, const string* value) {
  DCHECK_EQ(tid_, pthread_self());
  CHECK(key.size() < kMaxInt16) << "Key too big: [" << key << "], has size: "
      << key.size() << ", should be less than: " << kMaxInt16;
  const int16 key_size = int16(key.size());
  io::NumStreamer::WriteInt16(&op_buf_, key_size, common::BIGENDIAN);
  op_buf_.Write(key.data(), key.size());
  io::NumStreamer::WriteByte(&op_buf_, op);
  if ( value != NULL ) {
    io::NumStreamer::WriteInt32(&op_buf_, value->size(), common::BIGENDIAN);
    op_buf_.Write(value->data(), value->size());
  }
  if ( !in_transaction_ ) {
    QueueWrite();
  }
}

void StateKeeper::GetBounds(const string& prefix,
                            map<string, string>::iterator* begin,
                            map<string, string>::iterator* end) {
  DCHECK_EQ(tid_, pthread_self());
  return strutil::GetBounds(prefix, &data_, begin, end);
}

void StateKeeper::GetBounds(const string& prefix,
                            map<string, string>::const_iterator* begin,
                            map<string, string>::const_iterator* end) const {
  DCHECK_EQ(tid_, pthread_self());
  return strutil::GetBounds(prefix, data_, begin, end);
}
const char* StateKeeper::OpCodeName(OpCode op) {
  switch ( op ) {
    CONSIDER(OP_SET);
    CONSIDER(OP_DELETE);
    CONSIDER(OP_CLEARPREFIX);
  }
  LOG_FATAL << "Illegal OpCode: " << op;
  return "Unknown";
}
void StateKeeper::ClearMapPrefix(const string& prefix) {
  DCHECK_EQ(tid_, pthread_self());
  map<string, string>::iterator begin;
  map<string, string>::iterator end;
  GetBounds(prefix, &begin, &end);
  data_.erase(begin, end);
}

bool StateKeeper::DeleteValue(const string& key) {
  DCHECK_EQ(tid_, pthread_self());
  DCHECK(in_transaction_ || op_buf_.IsEmpty());
  if ( !data_.erase(key) ) {
    return true;
  }
  QueueOp(key, OP_DELETE);
  return true;
}

bool StateKeeper::DeletePrefix(const string& prefix) {
  DCHECK_EQ(tid_, pthread_self());
  DCHECK(in_transaction_ || op_buf_.IsEmpty());
  ClearMapPrefix(prefix);
  QueueOp(prefix, OP_CLEARPREFIX);
  return true;
}

bool StateKeeper::SetValue(const string& key, const string& value) {
  DCHECK_EQ(tid_, pthread_self());
  DCHECK(in_transaction_ || op_buf_.IsEmpty());
  string existing_value;
  if ( GetValue(key, &existing_value) && existing_value == value ) {
    // value already exists, avoid useless SET
    return true;
  }
  data_[key] = value;
  QueueOp(key, OP_SET, &value);
  return true;
}

int StateKeeper::ExpireTimeoutedKeys() {
  DCHECK_EQ(tid_, pthread_self());
  const int64 now = timer::Date::Now();

  map<string, string>::iterator begin, end, it;
  GetBounds(string(kStateKeeperTimeoutKey) + "/", &begin, &end);
  // see bellow how we format the timeout key
  static const int kNumTimeoutPosition = 25;

  BeginTransaction();
  vector<string> prefixes_to_delete;
  vector<string> keys_to_delete;
  bool delete_key = true;
  for ( it = begin; it != end && delete_key; ++it ) {
    if ( it->first.size() >
         sizeof(kStateKeeperTimeoutKey) + kNumTimeoutPosition + 1 ) {
      const string crt_time_str =
          it->first.substr(sizeof(kStateKeeperTimeoutKey) + 1,
                           kNumTimeoutPosition);
      errno = 0;
      const int64 crt_time = strtoll(crt_time_str.c_str(), NULL, 10);
      if ( errno == 0 ) {
        delete_key = crt_time < now;
      }
    }
    if ( delete_key ) {
      prefixes_to_delete.push_back(it->second);
      keys_to_delete.push_back(it->first);
    }
  }
  for ( int i = 0; i < prefixes_to_delete.size(); ++i ) {
    ILOG_DEBUG << "Prefix deleted on timeout: [" << keys_to_delete[i] << "]";
    DeletePrefix(prefixes_to_delete[i]);
    DeleteValue(keys_to_delete[i]);
  }
  CommitTransaction();
  return prefixes_to_delete.size();
}

//////////////////////////////////////////////////////////////////////

StateKeepUser::StateKeepUser(StateKeeper* state_keeper,
                             const string& prefix,
                             int timeout_ms)
    : state_keeper_(state_keeper),
      prefix_(prefix),
      timeout_ms_(timeout_ms),
      crt_timeout_(0) {
  CHECK(!prefix.empty());
  ILOG_DEBUG << "Timeout: " << timeout_ms;
  string str_crt_timeout;
  if ( GetValue(kStateKeeperTimeoutKey, &str_crt_timeout) ) {
    crt_timeout_ = ::strtol(str_crt_timeout.c_str(), NULL, 10);
    ILOG_DEBUG << "Relinquished timout @" << crt_timeout_;
  }
}
StateKeepUser::~StateKeepUser() {
}

bool StateKeepUser::CleanTimeout() {
  if ( crt_timeout_ <= 0 ) {
    return true;
  }
  const string original_timeout_key(strutil::StringPrintf("%s/%025"PRId64"/%s",
          kStateKeeperTimeoutKey, crt_timeout_, prefix_.c_str()));
  if ( !state_keeper_->DeleteValue(original_timeout_key) ) {
    return false;
  }
  crt_timeout_ = 0;
  return true;
}

bool StateKeepUser::UpdateTimeout() {
  if ( timeout_ms_ <= 0 ) {
    return true;
  }
  const int64 new_timeout = timer::Date::Now() + timeout_ms_;
  if ( new_timeout - crt_timeout_ < kMinUpdateTimeoutThresholdMs ) {
    return true;
  }
  string original_timeout_key;
  if ( crt_timeout_ > 0 ) {
    original_timeout_key = strutil::StringPrintf("%s/%025"PRId64"/%s",
        kStateKeeperTimeoutKey, crt_timeout_, prefix_.c_str());
  }
  crt_timeout_ = new_timeout;
  const string new_timeout_key(strutil::StringPrintf("%s/%025"PRId64"/%s",
      kStateKeeperTimeoutKey, crt_timeout_, prefix_.c_str()));
  if ( !state_keeper_->SetValue(new_timeout_key, prefix_) ) {
    return false;
  }
  SetValue(kStateKeeperTimeoutKey,
      strutil::StringPrintf("%"PRId64, crt_timeout_));
  ILOG_DEBUG << "Added NEW timeout key: [" << new_timeout_key << "]";
  // Only now delete the old key !!
  if ( !original_timeout_key.empty() ) {
    ILOG_DEBUG << "Deleting OLD key: [" << original_timeout_key << "]";
    state_keeper_->DeleteValue(original_timeout_key);
  }
  return true;
}

bool DetectStateKeeperSettings(const string& dir, string* out_file_base,
    int32* out_block_size, int32* out_blocks_per_file) {
  if ( !DetectLogSettings(dir, out_file_base, out_block_size,
                          out_blocks_per_file) ) {
    LOG_ERROR << "DetectLogSettings failed for dir: [" << dir << "]";
    return false;
  }
  // now the file_base contains the log prefix, remove it
  if ( !strutil::StrEndsWith(*out_file_base, kLogPrefix) ) {
    LOG_ERROR << "Log file base: [" << (*out_file_base) << "] does not end"
                 " with: [" << kLogPrefix << "]";
    return false;
  }
  (*out_file_base) = out_file_base->substr(0,
      out_file_base->size() - kLogPrefix.size());
  return true;
}

}
