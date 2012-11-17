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

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <whisperlib/base/errno.h>
#include <whisperlib/io/checkpoint/checkpointing.h>
#include <whisperlib/io/ioutil.h>

//////////////////////////////////////////////////////////////////////

namespace {

static const char kCheckpointBegin[] = "__checkpoint_begin__";
static const char kCheckpointEnd[] = "__checkpoint_end__";

const string ComposeFileName(const string& log_dir,
                             const string& file_base,
                             int32 file_num) {
  return strutil::JoinPaths(log_dir,
          strutil::StringPrintf("%s_%010d", file_base.c_str(), file_num));
}
}

//////////////////////////////////////////////////////////////////////

namespace io {

bool GetCheckpointFiles(const string& checkpoint_dir,
                        const string& file_base,
                        vector<string>* files) {
  re::RE reg("^" + file_base +
             "_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$");

  if ( !DirList(checkpoint_dir, LIST_FILES, &reg, files) ) {
    LOG_ERROR << "Cannot list directory: [" << checkpoint_dir
              << "] for file base: " << file_base;
    return false;
  }
  if ( !files->empty() ) {
    sort(files->begin(), files->end());
  }
  return true;
}

// A checkpoint file contains records. Each record is a string and
// represents a name or a value.
// File layout is: <kCheckpointBegin>,
//                 <name>, <value>, <name>, <value>, ...
//                 <kCheckpointEnd>,
bool ReadCheckpointFile(const string& filename,
                        map<string, string>* checkpoint) {
  io::File f;
  if ( !f.Open(filename, io::File::GENERIC_READ, io::File::OPEN_EXISTING) ) {
    LOG_ERROR << "Cannot open checkpoint file: [" << filename << "]";
    return false;
  }

  checkpoint->clear();
  bool reading_name = true;
  string name;
  io::RecordReader reader(io::kDefaultCheckpointBlockSize);
  bool first_record = true;
  while ( true ) {
    // read a chunk of data from file
    io::MemoryStream input;
    const int32 cb = f.Read(&input, io::kDefaultCheckpointBlockSize);
    if ( cb < 0 ) {
      LOG_ERROR << "Error reading file: [" << filename << "]";
      return false;
    }
    if ( cb == 0 ) {
      // success, file completely read
      if ( name  != kCheckpointEnd ) {
        LOG_ERROR << "Checkpoint finalized incorrectly: [" << filename
                  << "] ends in name: [" << name << "]";
        return false;
      }
      return true;
    }
    if ( cb != io::kDefaultCheckpointBlockSize ) {
      LOG_ERROR << "Incomplete read in: [" << filename << "], cb: " << cb;
      return false;
    }

    // extract records from the chunk
    while ( true ) {
      io::MemoryStream rec;
      int num_skipped = 0;
      io::RecordReader::ReadResult err = reader.ReadRecord(&input, &rec, &num_skipped, 0);
      if ( err == io::RecordReader::READ_NO_DATA ) {
        CHECK(input.IsEmpty());
        break;
      }
      if ( err != io::RecordReader::READ_OK ) {
        LOG_ERROR << "Error reading next record: "
                  << io::RecordReader::ReadResultName(err)
                  << ", corrupted checkpoint file: [" << filename << "]";
        return false;
      }
      if ( first_record ) {
        first_record = false;
        string name;
        rec.ReadString(&name);
        if ( name != kCheckpointBegin ) {
          LOG_ERROR << "Checkpoint begins incorrectly: [" << filename
                    << "] with name: [" << name;
          return false;
        }
        continue;
      }
      // group records into name/value
      if ( reading_name ) {
        reading_name = false;
        rec.ReadString(&name);
        continue;
      }
      string value;
      rec.ReadString(&value);
      reading_name = true;
      checkpoint->insert(make_pair(name, value));
    }
  }
  LOG_FATAL << "Unreachable line";
  return true;
}

//////////////////////////////////////////////////////////////////////

CheckpointWriter::CheckpointWriter(const string& checkpoint_dir,
                                   const string& file_base)
  : checkpoint_dir_(checkpoint_dir),
    file_base_(file_base),
    re_("^" + file_base +
        "_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$"),
    recorder_(NULL) {
  CHECK(checkpoint_dir == "" || io::IsDir(checkpoint_dir))
      << " Not a directory: [" << checkpoint_dir << "]";
}

CheckpointWriter::~CheckpointWriter() {
  ClearCheckpoint();
}

int32 CheckpointWriter::BeginCheckpoint() {
  CHECK(!file_.is_open());
  CHECK_NULL(recorder_);
  CHECK(names_.empty());
  CHECK(buf_.IsEmpty());

  const int32 file_num = GetLastNumberedFile(checkpoint_dir_, &re_, 10) + 1;
  if ( file_num < 0 ) {
    return -1;
  }
  string filename = ComposeFileName(checkpoint_dir_, file_base_, file_num);
  if ( !file_.Open(filename, io::File::GENERIC_WRITE, io::File::CREATE_ALWAYS) ) {
    LOG_ERROR << "Error creating file: ["<< filename << "]";
    return -1;
  }
  LOG_INFO << "Starting a new checkpoint: " << filename;
  recorder_ = new RecordWriter(kDefaultCheckpointBlockSize);
  recorder_->AppendRecord(kCheckpointBegin, sizeof(kCheckpointBegin)-1, &buf_);
  return file_num;
}

void CheckpointWriter::CleanOldCheckpoints(int num_checkpoints_to_keep) {
  vector<string> files;
  if ( !GetCheckpointFiles(checkpoint_dir_, file_base_, &files) ) {
    LOG_ERROR << "Cannot scan for checkpoint files"
                 ", in dir: [" << checkpoint_dir_ << "]"
                 ", file_base: [" << file_base_ << "]";
    return;
  }
  const int32 limit = files.size() - num_checkpoints_to_keep;
  for ( int32 i = 0; i < limit; ++i ) {
    io::Rm(strutil::JoinPaths(checkpoint_dir_, files[i]));
  }
}

bool CheckpointWriter::AddRecord(const string& name, const string& value) {
  CHECK(file_.is_open());
  if ( names_.find(name) != names_.end() ) {
    LOG_ERROR << "Duplicate name: [" << name << "]. All values in checkpoint"
                 " must have unique names.";
    return false;
  }
  names_.insert(name);
  const bool fin1 = recorder_->AppendRecord(name.data(), name.size(), &buf_);
  const bool fin2 = recorder_->AppendRecord(value.data(), value.size(), &buf_);
  if ( fin1 || fin2 ) {
    return WriteBuffer();
  }
  return true;
}

void CheckpointWriter::ClearCheckpoint() {
  delete recorder_;
  recorder_ = NULL;
  names_.clear();
  file_.Close();
  buf_.Clear();
}

bool CheckpointWriter::EndCheckpoint() {
  CHECK(file_.is_open());
  CHECK_NOT_NULL(recorder_);
  recorder_->AppendRecord(kCheckpointEnd, sizeof(kCheckpointEnd)-1, &buf_);
  recorder_->FinalizeContent(&buf_);
  if ( !WriteBuffer() ) {
    ClearCheckpoint();
    return false;
  }
  LOG_INFO << "Checkpoint closing OK. file: [" << file_.filename()
           << "], name count: " << names_.size()
           << ", file size: " << file_.Size();
  ClearCheckpoint();
  return true;
}

bool CheckpointWriter::WriteBuffer() {
  CHECK(file_.is_open());
  CHECK(buf_.Size() % kDefaultCheckpointBlockSize == 0)
      << " - buf_.Size(): " << buf_.Size();

  buf_.MarkerSet();
  const int32 cb = file_.Write(&buf_);
  if ( cb < 0 ) {
    buf_.MarkerRestore();
    LOG_ERROR << "Error writing file: [" << file_.filename()
              << "], err: " << ::GetLastSystemErrorDescription();
    return false;
  }
  if ( !buf_.IsEmpty() ) {
    buf_.MarkerRestore();
    LOG_ERROR << "Partial write, succeeded only: " << cb
              << " out of " << buf_.Size()
              << ", file: " << file_.filename()
              << ", err: " << GetLastSystemErrorDescription();
    return false;
  }
  buf_.MarkerClear();
  return true;
}

//////////////////////////////////////////////////////////////////////

bool ReadCheckpoint(const string& checkpoint_dir,
                    const string& file_base,
                    map<string, string>* checkpoint) {
  vector<string> files;
  if ( !GetCheckpointFiles(checkpoint_dir, file_base, &files) ) {
    LOG_ERROR << "Cannot scan for checkpoint files"
                 ", in dir: [" << checkpoint_dir << "]"
                 ", file_base: [" << file_base << "]";
    return false;
  }
  if ( files.empty() ) {
    LOG_INFO << "No files to read for checkpoint: " << checkpoint_dir
             << " | " << file_base;
    return false;
  }
  for ( int i = files.size() - 1; i >= 0; --i ) {
    const string filename = strutil::JoinPaths(checkpoint_dir, files[i]);
    if ( ReadCheckpointFile(filename, checkpoint) ) {
      LOG_INFO << "Checkpoint loaded from: [" << filename << "]"
                  ", values: " << checkpoint->size();
      return true;
    }
    LOG_ERROR << "Corrupted checkpoint file: [" << filename << "]";
  }

  LOG_ERROR << "All checkpoints corrupted: " << checkpoint_dir
            << " | " << file_base;
  return false;
}

bool WriteCheckpointFile(const map<string, string>& checkpoint,
                         const string& output_filename) {
  CheckpointWriter writer(strutil::Dirname(output_filename),
                          strutil::Basename(output_filename));
  if ( writer.BeginCheckpoint() == -1 ) {
    LOG_ERROR << "Cannot BeginCheckpoint, for output_filename: ["
              << output_filename << "]";
    return false;
  }
  for ( map<string, string>::const_iterator it = checkpoint.begin();
        it != checkpoint.end(); ++it ) {
    if ( !writer.AddRecord(it->first, it->second) ) {
      LOG_ERROR << "Failed to add pair: [" << it->first << "] -> ["
                << it->second << "]";
      return false;
    }
  }
  if ( !writer.EndCheckpoint() ) {
    LOG_ERROR << "Failed to EndCheckpoint, for output_filename: ["
              << output_filename << "]";
    return false;
  }
  return true;
}

void CleanCheckpointFiles(const string& checkpoint_dir,
                          const string& file_base,
                          uint32 num_checkpoints_to_keep) {
  CheckpointWriter writer(checkpoint_dir, file_base);
  writer.CleanOldCheckpoints(num_checkpoints_to_keep);
}
}
