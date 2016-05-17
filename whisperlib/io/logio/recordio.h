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

// This contains utilities for reading and writing records to / from
// a stream.
//
// The main idea is that we add pieces of information (memory stream data)
// to a record stream. We add these in blocks of fixed size (so in case of
// corrupted blocks we can skip them). Records may spawn over multiple blocks.
//
// The reading part builds pieces of file (block_size) and from there extracts
// records.
//
// Is needless to say that in order to work, the reader should be created
// with the same record size as the writer.
//

#ifndef __COMMON_IO_LOGIO_RECORDIO_H__
#define __COMMON_IO_LOGIO_RECORDIO_H__

#include <zlib.h>
#include "whisperlib/io/buffer/memory_stream.h"
#include "whisperlib/io/zlib/zlibwrapper.h"

namespace whisper {
namespace io {

static const int32 kDefaultRecordBlockSize = 65536;
static const int32 kMaximumRecordBlockSize = 0xFFFFFF; // 16MB

class RecordWriter {
 public:
  enum {
    HAS_CONT = 1,
    IS_ZIPPED = 2,
    IS_FIRST  = 4,
  };

  RecordWriter(int32 block_size = kDefaultRecordBlockSize,
               bool deflate = false,
               float dumpable_percent = 0.9f);
  ~RecordWriter();

  // Appends the provided content from 'in' to the record.
  // returns: true: 'out' contains 1 or more complete blocks, ready to
  //                be appended to the file as a one shot-write.
  //          false: 'in' data goes to the internal buffer. 'out' contains
  //                 nothing.
  // Depending on the 'in' size, the 'out' may contain multiple blocks.
  bool AppendRecord(io::MemoryStream* in, io::MemoryStream* out) {
    return AppendRecord(in, out, false);
  }
  bool AppendRecord(const char* buffer, int32 size, io::MemoryStream* out);

  // This returns the current content (accumulated so far) as a one block
  // to be written to the disk.
  void FinalizeContent(io::MemoryStream* out);

  // number of records currently accumulated in 'content_'.
  int32 PendingRecordCount() const { return content_record_count_; }

  int32 leftover() const {
    return content_.Size();
  }
  void Clear() {
    prev_block_crc_ = 0;
  }
 private:
  bool AppendRecord(io::MemoryStream* in, io::MemoryStream* out,
                    bool is_zipped);
  // We trail each block with the content size and crc..
  static const int32 kBlockTrailerEnd = 3 * sizeof(int32);
  // Each record is prepended with a header (type 1 + size 3)
  static const int32 kRecordHeaderSize = 4;
  // some zeroes used for padding
  static const char* padding_;

  const int32 block_size_;     // we write record blocks of this size
  const int32 dumpable_size_;  // we can finish a record if we have more
                               // than this in the buffer or the next
                               // records overflows
  io::MemoryStream content_;   // accumulated content so far..
  ZlibDeflateWrapper* zlib_;   // for compressing content
  io::MemoryStream zlib_content_;
                               // buffer of compressed content
  int32 content_record_count_; // number or records currently in 'content_'

  int32 prev_block_crc_;

  DISALLOW_EVIL_CONSTRUCTORS(RecordWriter);
};

class RecordReader {
 public:
  explicit RecordReader(int32 block_size = kDefaultRecordBlockSize);
  ~RecordReader();

  // Reads the content of the next record from the provided memory stream
  enum ReadResult {
    READ_OK = 0,         // out contains some valid data
    READ_NO_DATA,        // there is not enough data in 'in' to be read
    READ_CRC_CORRUPTED,  // the data was corrupted (crc wise)
    READ_ZIP_CORRUPTED,  // the zipped recored was corrupted
    READ_CRC_BLOCK_BROKEN,  // the continuation from previous block broken
  };
  static const char* ReadResultName(ReadResult result);
  // out: if NULL, the record is read but dropped.
  ReadResult ReadRecord(io::MemoryStream* in, io::MemoryStream* out,
                        int* num_skipped, int max_num_skipped);

  void Clear() {
    temp_.Clear();
    content_.Clear();
    prev_block_crc_ = 0;
  }
 private:
  void SkipRecord();
  RecordReader::ReadResult ReadNextBlock(io::MemoryStream* in);

  static const int32 kBlockTrailerEnd = 3 * sizeof(int32);

  const int32 block_size_;    // we read records of this size
  io::MemoryStream temp_;     // a temp buffer
  io::MemoryStream content_;  // raw data extracted from blocks (it contains
                              // just records)
  io::MemoryStream record_content_; // current record content, may be spread
                                    // across multiple blocks (that's why
                                    // this buffer accumulator is used)
  ZlibInflateWrapper zlib_;   // inflates stuff for us

  int32 prev_block_crc_;
  bool skip_record_;

  DISALLOW_EVIL_CONSTRUCTORS(RecordReader);
};
}  // namespace io
}  // namespace whisper

#endif  // __COMMON_IO_LOGIO_RECORDIO_H__
