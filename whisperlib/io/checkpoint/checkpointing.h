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

#ifndef __COMMON_IO_CHECKPOINT_CHECKPOINTING_H__
#define __COMMON_IO_CHECKPOINT_CHECKPOINTING_H__

#include <map>
#include <string>
#include <vector>
#include <whisperlib/base/types.h>
#include <whisperlib/base/re.h>
#include <whisperlib/io/buffer/memory_stream.h>
#include <whisperlib/io/file/file.h>
#include <whisperlib/io/logio/recordio.h>

namespace io {

static const int32 kDefaultCheckpointBlockSize = 65536;

class CheckpointWriter {
 public:
  CheckpointWriter(const string& checkpoint_dir,
                   const string& file_base);
  ~CheckpointWriter();

  // Starts a new checkpoint file -- will return the id of the new
  // file (0 based) or -1 on error
  int32 BeginCheckpoint();

  // Adds a new checkpoint record under the provided name.
  bool AddRecord(const string& name, const string& value);

  // End the current checkpoint file.
  bool EndCheckpoint();

  // Closes a checkpoint in middle of writing
  void ClearCheckpoint();

  // Cleans old files..
  void CleanOldCheckpoints(int num_checkpoints_to_keep);

 private:
  bool WriteBuffer();

  const string checkpoint_dir_;  // checkpoints lay here..
  const string file_base_;       // and begin w/ this name
  re::RE       re_;              // matches log files

  io::File file_;                // current file
  MemoryStream buf_;             // used to accumulate records
  RecordWriter* recorder_;       // encapsulates records for us
  set<string> names_;            // names in the checkpoint

  DISALLOW_EVIL_CONSTRUCTORS(CheckpointWriter);
};


bool GetCheckpointFiles(const string& checkpoint_dir,
                        const string& file_base,
                        vector<string>* files);

bool ReadCheckpointFile(const string& filename,
                        map<string, string>* checkpoint);

bool ReadCheckpoint(const string& checkpoint_dir,
                    const string& file_base,
                    map<string, string>* checkpoint);

bool WriteCheckpointFile(const map<string, string>& checkpoint,
                         const string& output_filename);

void CleanCheckpointFiles(const string& checkpoint_dir,
                          const string& file_base,
                          uint32 num_checkpoints_to_keep);
}
#endif  // __COMMON_IO_CHECKPOINT_H__
