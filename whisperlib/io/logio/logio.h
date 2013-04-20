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

// Logging interface - one for write and one for read .
//
// Logs are composed from files of (approximately) fixed length,
// sequentialy numbered. These file are composed from fixed sized
// blocks of variable length records.
//
// The position in a log is a tripet: (file_num, block_num, record_num)
//

#ifndef __COMMON_IO_LOGIO_LOGIO_H__
#define __COMMON_IO_LOGIO_LOGIO_H__

#include <string>
#include <whisperlib/base/types.h>
#include <whisperlib/base/re.h>
#include <whisperlib/io/file/file.h>
#include <whisperlib/io/buffer/memory_stream.h>
#include <whisperlib/io/logio/recordio.h>
#include <google/protobuf/message_lite.h>

namespace io {

static const int32 kDefaultBlocksPerFile = 1 << 14;

//////////////////////////////////////////////////////////////////////

struct LogPos {
  int32 file_num_;
  int32 block_num_;
  int32 record_num_;
  LogPos()
    : file_num_(-1), block_num_(0), record_num_(0) {
  }
  LogPos(int32 file_num, int32 block_num, int32 record_num)
    : file_num_(file_num), block_num_(block_num), record_num_(record_num) {
  }
  LogPos(const LogPos& other)
    : file_num_(other.file_num_),
      block_num_(other.block_num_),
      record_num_(other.record_num_) {
  };

  const LogPos& operator=(const LogPos& other) {
    file_num_ = other.file_num_;
    block_num_ = other.block_num_;
    record_num_ = other.record_num_;
    return *this;
  }

  // 3-way compare function returns:  -1 for less, 0 for equal, 1 for greater
  int Compare(const LogPos& other) const {
    if (file_num_ < other.file_num_)  return -1;
    if (file_num_ > other.file_num_)  return 1;
    if (block_num_ < other.block_num_)  return -1;
    if (block_num_ > other.block_num_)  return 1;
    if (record_num_ < other.record_num_)  return -1;
    if (record_num_ > other.record_num_)  return 1;
    return 0;
  }

  bool operator==(const LogPos& other) const {
    return (file_num_ == other.file_num_ &&
            block_num_ == other.block_num_ &&
            record_num_ == other.record_num_);
  }

  bool IsNull() const {
    return file_num_ == -1;
  }

  string ToString() const {
    return strutil::StringPrintf(
      "LogPos{file#: %5d; block#: %5d; record#: %5d}",
      file_num_, block_num_, record_num_);
  }
  string StrEncode() const {
    return strutil::StringPrintf(
        "LogPos{file: %d; block: %d; record: %d}",
        file_num_, block_num_, record_num_);
  }
  bool StrDecode(const string& str) {
    if ( str.size() <= 8 ) {
      LOG_ERROR << "Cannot decode, string too small: [" << str << "]";
      return false;
    }
    vector< pair<string, string> > pairs;
    strutil::SplitPairs(str.substr(7, str.size() - 8), ";", ":", &pairs);
    if ( pairs.size() != 3 ) {
      LOG_ERROR << "Cannot decode str: [" << str << "]";
      return false;
    }
    file_num_ = ::strtol(pairs[0].second.c_str(), NULL, 10);
    block_num_ = ::strtol(pairs[1].second.c_str(), NULL, 10);
    record_num_ = ::strtol(pairs[2].second.c_str(), NULL, 10);
    return true;
  }
};

//////////////////////////////////////////////////////////////////////

class LogWriter {
 public:
  LogWriter(const string& log_dir,
            const string& file_base,
            int32 block_size = kDefaultRecordBlockSize,
            int32 blocks_per_file = kDefaultBlocksPerFile,
            bool temporary_incomplete_file = false,
            bool deflate = false);
  ~LogWriter();

  // true: success, the log_dir and file_base are marked as locked
  // false: failure, a lock file already exists
  bool Initialize();

  const string& file_name() const { return file_.filename(); }

  bool WriteRecord(io::MemoryStream* in);
  bool WriteRecord(const char* buffer, int32 size);
  bool WriteRecord(const google::protobuf::MessageLite* message) {
      string output;
      if (!message->SerializeToString(&output)) {
          return false;
      }
      return WriteRecord(output.data(), output.size());
  }

  // Flush internal buffer to file.
  bool Flush();

  // This makes sense after a Flush() call
  LogPos Tell() const;

  // close everything
  void Close();

 private:
  // Name of the current file to open (according to our internal members)
  // If temp == true and the real file does not exist:
  //   => generates a temporary filename
  // else => returns the real file name
  string ComposeFileName(bool temp) const;
  // Puts the current buf_ into the log file
  bool WriteBuffer(bool force_flush);
  // Determines and opens the next log file
  bool OpenNextLog();
  // Close the current log file. Moves temporary file in final place.
  void CloseLog();

 private:
  const string log_dir_;         // log files are here..
  const string file_base_;       // and begin w/ this name
  const int32 block_size_;       // we create block of records of this size
  const int32 blocks_per_file_;  // and we write these many blocks per file
  const bool temporary_incomplete_file_;
                                 // incomplete files have a temporary name.
                                 // Once a file is complete it is moved in
                                 // the right place.
  const bool deflate_;           // deflate records on writing
                                 // (makes sense for txt)

  re::RE re_;                    // matches log files

  int32 file_num_;               // the current file number
                                 //  (for the opened file)
  io::File file_;                // current file
  MemoryStream buf_;             // used to accumulate records
  RecordWriter recorder_;        // encapsulates records for us


  DISALLOW_EVIL_CONSTRUCTORS(LogWriter);
};

//////////////////////////////////////////////////////////////////////

class LogReader {
 public:
  LogReader(const string& log_dir,
            const string& file_base,
            int32 block_size = kDefaultRecordBlockSize,
            int32 blocks_per_file = kDefaultBlocksPerFile);
  ~LogReader();

  // returns: true: success, there's a new record in 'out'
  //          false: no more data.
  // Corrupted records are automatically skipped.
  bool GetNextRecord(io::MemoryStream* out);

  uint32 num_errors() const   { return num_errors_; }

  LogPos Tell() const {
    int32 block_num = 0;
    if ( file_.is_open() && file_.Position() > 0 ) {
      CHECK(file_.Position() % block_size_ == 0) << "Illegal file position: "
          << file_.Position() << ", block_size_: " << block_size_;
      block_num = file_.Position() / block_size_ - (record_num_ == 0 ? 0 : 1);
    }
    return LogPos(file_num_, block_num, record_num_);
  }

  // try Seek to the given position.
  // returns: success. If seek fails => the log is automatically Rewind()
  bool Seek(const LogPos& pos);

  // Scans and go to the first file. Never fails.
  // No files => leave file_num == -1 and GetNextRecord() will Rewind() again
  // Some files => go to first file_num found.
  void Rewind();

  string ComposeFileName(int32 file_num) const;

 private:
  void CloseInternal(bool is_error);
  // close current file, open given file
  bool OpenFile(int32 file_num);
  // advance inside current opened file to given block/record
  bool AdvanceToPosInFile(const LogPos& log_pos);
  // read the next block at current file position.
  // Does not normally increment pos_.block_num_. However, it may go to
  // the next file (if this happens, then pos_.block_num_ is set 0)
  bool ReadBlock();

 private:
  const string log_dir_;         // log files are here..
  const string file_base_;       // and begin w/ this name
  re::RE       re_;              // matches log files
  const int32 block_size_;       // we create block of records of this size
  const int32 blocks_per_file_;  // and we write these many blocks per file

  // current file
  io::File file_;
  // current position, of the next record to read. Block num is deduced
  // from file position.
  int32 file_num_;
  int32 record_num_;

  // splits records for us
  RecordReader reader_;
  // count record errors (corrupted records are automatically discarded)
  uint32 num_errors_;
  // filled with 1 block at a time, consumed record by record
  io::MemoryStream buf_;

  DISALLOW_EVIL_CONSTRUCTORS(LogReader);
};

// Clean log files before the given position (i.e. you can seek to the
// given position, but probably not before .. )
//
// We return how many files we have deleted.
uint32 CleanLog(const string& log_dir, const string& file_base,
                LogPos first_pos, int32 block_size);

// Autodetect logio settings
bool DetectLogSettings(const string& log_dir, string* out_file_base,
    int32* out_block_size, int32* out_blocks_per_file);
}

#endif  // __COMMON_IO_LOGIO_LOGIO_H__
