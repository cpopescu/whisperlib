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

#ifndef __COMMON_IO_FILE_FILE_INPUT_STREAM_H__
#define __COMMON_IO_FILE_FILE_INPUT_STREAM_H__

#include <vector>
#include <string>
#include "whisperlib/io/input_stream.h"
#include "whisperlib/io/file/file.h"

namespace whisper {
namespace io {

class FileInputStream : public InputStream {
 public:
  FileInputStream(File* file);
  virtual ~FileInputStream();

  static std::string ReadFileOrDie(const char* filename);
  static std::string ReadFileOrDie(const std::string& filename);
  static bool TryReadFile(const char* filename, std::string* content);
  static bool TryReadFile(const std::string& filename, std::string* content);

  int32 Read(io::MemoryStream* ms, int32 len);

  // Input Stream interface
  virtual int32 Read(void* buffer, int32 len);
  virtual int32 Peek(void* buffer, int32 len) {
    MarkerSet();
    const int32 ret = Read(buffer, len);
    MarkerRestore();
    return ret;
  }
  virtual int64 Skip(int64 len);
  virtual int64 Readable() const;
  virtual bool IsEos() const;

  void MarkerSet();
  void MarkerRestore();
  void MarkerClear();

 private:
  File* const file_;
  std::vector<int64> read_mark_positions_;

  DISALLOW_EVIL_CONSTRUCTORS(FileInputStream);
};
}  // namespace io
}  // namespace whisper

#endif  // __COMMON_IO_FILE_FILE_INPUT_STREAM_H__
