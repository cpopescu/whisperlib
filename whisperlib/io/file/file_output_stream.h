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

#ifndef __COMMON_IO_FILE_FILE_OUTPUT_STREAM_H__
#define __COMMON_IO_FILE_FILE_OUTPUT_STREAM_H__

#include <string>
#include "whisperlib/io/file/file.h"
#include "whisperlib/io/output_stream.h"

namespace whisper {
namespace io {

class FileOutputStream : public OutputStream {
 public:
  FileOutputStream(File* file);
  ~FileOutputStream();

  static void WriteFileOrDie(const char* filename, const std::string& content);
  static bool TryWriteFile(const char* filename, const std::string& content);

  virtual ssize_t WriteBuffer(const void* buf, size_t len);
  virtual uint64_t Writable() const;

  void Flush() const { file_->Flush(); }

 private:
  File* file_;
  DISALLOW_EVIL_CONSTRUCTORS(FileOutputStream);
};
}  // namespace io
}  // namespace whisper

#endif  // __COMMON_IO_FILE_FILE_OUTPUT_STREAM_H__
