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

#include "whisperlib/io/file/file.h"
#include "whisperlib/io/file/file_output_stream.h"

namespace whisper {
namespace io {

FileOutputStream::FileOutputStream(File* file)
  : OutputStream(),
    file_(file) {
}

FileOutputStream::~FileOutputStream() {
  delete file_;
}

void FileOutputStream::WriteFileOrDie(const char* filename,
                                      const std::string& content) {
  io::File* const outfile = io::File::CreateFileOrDie(filename);
  io::FileOutputStream fis(outfile);
  fis.Write(content.data(), content.size());
}
bool FileOutputStream::TryWriteFile(const char* filename,
                                    const std::string& content) {
  io::File* const outfile = io::File::TryCreateFile(filename);
  if ( outfile == NULL )
    return false;
  io::FileOutputStream fis(outfile);
  return content.size() == fis.Write(content.data(), content.size());
}

int32 FileOutputStream::Write(const void* buf, int32 len) {
  int32 written = 0;
  const char* p = reinterpret_cast<const char*>(buf);
  while ( len > 0 ) {
    const int32 cb = file_->Write(p, len);
    if ( cb < 0 ) {
      return cb;
    }
    if ( cb == 0 ) {
      return written;
    }
    written += cb;
    p += cb;
    len -= cb;
  }
  return written;
}

int64 FileOutputStream::Writable() const {
  return -1;   // no limit
}
}  // namespace io
}  // namespace whisper
