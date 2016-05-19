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

#include <sys/types.h>
#include "whisperlib/io/file/file_input_stream.h"
#include "whisperlib/base/log.h"

namespace whisper {
namespace io {

static const uint64_t kMaxFileInputStreamReadSize = 128 << 20;

std::string FileInputStream::ReadFileOrDie(const char* filename) {
  std::string s;
  CHECK(TryReadFile(filename, &s));
  return s;
}
std::string FileInputStream::ReadFileOrDie(const std::string& filename) {
  return ReadFileOrDie(filename.c_str());
}
bool FileInputStream::TryReadFile(const char* filename, std::string* content) {
  io::File* const infile = io::File::TryOpenFile(filename);
  if ( infile == NULL ) {
    return false;
  }
  io::FileInputStream fis(infile);
  uint64_t size = fis.Readable();
  if ( size > kMaxFileInputStreamReadSize ) {
    return false;
  }
  std::string s;
  return ssize_t(size) == fis.ReadString(content, size_t(size));
}
bool FileInputStream::TryReadFile(const std::string& filename, std::string* content) {
  return TryReadFile(filename.c_str(), content);
}

FileInputStream::FileInputStream(File* file)
  : InputStream(),
    file_(file) {
}
FileInputStream::~FileInputStream() {
  delete file_;
}

ssize_t FileInputStream::Read(io::MemoryStream* ms, size_t len) {
  return file_->Read(ms, len);
}

ssize_t FileInputStream::ReadBuffer(void* buffer, size_t len) {
  return file_->ReadBuffer(buffer, len);
}

int64_t FileInputStream::Skip(int64_t len) {
  return file_->SetPosition(len, File::FILE_CUR);
}

uint64_t FileInputStream::Readable() const {
  const uint64_t size = file_->Size();
  const uint64_t pos = file_->Position();
  if (pos > size) return 0;
  return size - pos;
}

bool FileInputStream::IsEos() const {
  return file_->Size() <= file_->Position();
}

void FileInputStream::MarkerSet() {
  read_mark_positions_.push_back(file_->Position());
}

void FileInputStream::MarkerRestore() {
  CHECK(!read_mark_positions_.empty());
  const int64 mark_position = read_mark_positions_.back();
  read_mark_positions_.pop_back();
  file_->SetPosition(mark_position, File::FILE_SET);
}

void FileInputStream::MarkerClear() {
  CHECK(!read_mark_positions_.empty());
  read_mark_positions_.pop_back();
}
}
}
