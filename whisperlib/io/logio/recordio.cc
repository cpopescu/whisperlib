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

#include "whisperlib/io/logio/recordio.h"

using namespace std;

#define LOG_REC if ( true ); else DLOG_INFO

namespace {
int32 ComputeCRC(whisper::io::MemoryStream* buf) {
  int32 crc = static_cast<int32>(crc32(0L, Z_NULL, 0));
  buf->MarkerSet();
  while ( !buf->IsEmpty() ) {
    int32 crt_size = buf->Size();
    const char* crt_buf = NULL;
    CHECK(buf->ReadNext(&crt_buf, &crt_size));
    crc = crc32(crc, reinterpret_cast<const Bytef*>(crt_buf), crt_size);
  }
  buf->MarkerRestore();
  return crc;
}
char* NewZeroes(int32 size) {
  char* p = new char[size];
  memset(p, 0, size);
  return p;
}
}

// COMPILE_ASSERT(sizeof(uLong) == sizeof(int32),  zlib_uLong_should_be_32_bit);

namespace whisper {
namespace io {

//////////////////////////////////////////////////////////////////////

const char* RecordWriter::padding_ = NewZeroes(kMaximumRecordBlockSize);

RecordWriter::RecordWriter(int32 block_size,
                           bool deflate,
                           float dumpable_percent)
    : block_size_(block_size),
      dumpable_size_(static_cast<int32>(
          dumpable_percent * (block_size_ - kBlockTrailerEnd))),
      content_(block_size_),
      zlib_(deflate ? new ZlibDeflateWrapper() : NULL),
      zlib_content_(block_size_),
      content_record_count_(0),
      prev_block_crc_(0) {
  CHECK_LT(block_size_, kMaximumRecordBlockSize);
}


RecordWriter::~RecordWriter() {
  CHECK(content_.IsEmpty()) << " " << content_.Size() << " remaining bytes";
  delete zlib_;
  zlib_ = NULL;
}

bool RecordWriter::AppendRecord(io::MemoryStream* in,
                                io::MemoryStream* out,
                                bool is_zipped) {
  if ( zlib_ != NULL && !is_zipped ) {
    DCHECK(zlib_content_.IsEmpty());
    zlib_->Clear();
    CHECK(zlib_->Deflate(in, &zlib_content_));
    return AppendRecord(&zlib_content_, out, true);
  }
  bool is_first = true;
  uint32 written_block_count = 0;
  while ( !in->IsEmpty() ) {
    const int32 available = block_size_ - kBlockTrailerEnd - kRecordHeaderSize
                            - content_.Size();
    CHECK(available > 0) << ", available: " << available
                         << ", block_size_: " << block_size_
                         << ", content_.Size(): " << content_.Size();
    // If there's enough space, write the whole record in current block
    if ( in->Size() <= available ) {
      io::NumStreamer::WriteByte(&content_, is_first ? IS_FIRST : 0);
      io::NumStreamer::WriteUInt24(&content_, in->Size(), common::BIGENDIAN);
      content_.AppendStream(in);
      content_record_count_++;
      continue; // breaks the loop, because 'in' is empty now
    }
    // So: not enough space, write partial record in current block
    CHECK_GT(in->Size(), available);
    io::NumStreamer::WriteByte(&content_, HAS_CONT |
                                          (is_first ? IS_FIRST : 0));
    io::NumStreamer::WriteUInt24(&content_, available, common::BIGENDIAN);
    content_.AppendStream(in, available);
    content_record_count_++;
    is_first = false;
    FinalizeContent(out);
    written_block_count += 1;
  }
  if ( content_.Size() > dumpable_size_ ) {
    FinalizeContent(out);
    written_block_count += 1;
  }
  return written_block_count >= 1;
}

bool RecordWriter::AppendRecord(const char* buffer, int32 size,
                                io::MemoryStream* out) {
  if ( zlib_ ) {
    DCHECK(zlib_content_.IsEmpty());
    zlib_->Clear();
    CHECK(zlib_->Deflate(buffer, size, &zlib_content_));
    return AppendRecord(&zlib_content_, out, true);
  }
  const char* p = buffer;
  int32 p_size = size;
  bool is_first = true;
  uint32 written_block_count = 0;
  // start with DO, because p_size may be 0 which is an empty value: ""
  // which is perfectly legal.
  do {
    const int32 available = block_size_ - kBlockTrailerEnd - kRecordHeaderSize
                            - content_.Size();
    CHECK(available > 0) << ", available: " << available
                         << ", block_size_: " << block_size_
                         << ", content_.Size(): " << content_.Size();
    // If there's enough space, write the whole record in current block
    if ( p_size <= available ) {
      LOG_REC << "Add full record in one block. record_size: " << p_size
              << ", available: " << available;
      io::NumStreamer::WriteByte(&content_, is_first ? IS_FIRST : 0);
      io::NumStreamer::WriteUInt24(&content_, p_size, common::BIGENDIAN);
      content_.Write(p, p_size);
      content_record_count_++;
      p += p_size;
      p_size = 0;
      break; // break the loop, because p_size == 0.
             // Don't use continue, because it jumps at loop begin!
    }
    // So: not enough space, write partial record in current block
    CHECK_GT(p_size, available);
    LOG_REC << "Add partial record in current block. record_size: " << p_size
            << ", available: " << available;
    io::NumStreamer::WriteByte(&content_, HAS_CONT |
                                          (is_first ? IS_FIRST : 0));
    io::NumStreamer::WriteUInt24(&content_, available, common::BIGENDIAN);
    content_.Write(p, available);
    content_record_count_++;
    p += available;
    p_size -= available;
    is_first = false;
    FinalizeContent(out);
    written_block_count += 1;
  } while ( p_size > 0 );
  if ( content_.Size() > dumpable_size_ ) {
    FinalizeContent(out);
    written_block_count += 1;
  }
  return written_block_count >= 1;
}

void RecordWriter::FinalizeContent(io::MemoryStream* out) {
  if ( content_.IsEmpty() ) return;
  const int32 content_size = content_.Size();
  const int32 padding_size = block_size_ - content_size - kBlockTrailerEnd;
  LOG_REC << "Finalize block. content: " << content_size
          << ", padding: " << padding_size;
  CHECK(padding_size >= 0) << ", content: " << content_.Size()
                           << ", padding_size: " << padding_size
                           << ", block_size_: " << block_size_;
  content_.Write(padding_, padding_size);
  io::NumStreamer::WriteInt32(&content_, content_size,
                              common::BIGENDIAN);
  io::NumStreamer::WriteInt32(&content_, prev_block_crc_,
                              common::BIGENDIAN);

  const int32 crc = ComputeCRC(&content_);
  io::NumStreamer::WriteInt32(&content_, crc, common::BIGENDIAN);
  out->AppendStream(&content_);
  LOG_REC << "Content Finalized:"
          << " prev_block_crc_: " << hex << prev_block_crc_
          << ", crc: " << hex << crc;

  prev_block_crc_ = crc;

  DCHECK(content_.IsEmpty());
  content_record_count_ = 0;
}

//////////////////////////////////////////////////////////////////////

RecordReader::RecordReader(int32 block_size)
  : block_size_(block_size),
    temp_(block_size_),
    content_(block_size_),
    record_content_(block_size_),
    prev_block_crc_(0),
    skip_record_(false) {
}

RecordReader::~RecordReader() {
}

const char* RecordReader::ReadResultName(ReadResult result) {
  switch ( result ) {
    CONSIDER(READ_OK);
    CONSIDER(READ_NO_DATA);
    CONSIDER(READ_CRC_CORRUPTED);
    CONSIDER(READ_ZIP_CORRUPTED);
    CONSIDER(READ_CRC_BLOCK_BROKEN);
  }
  LOG_FATAL << "Illegal result: " << result;
  return "Unknown";
}

RecordReader::ReadResult RecordReader::ReadNextBlock(io::MemoryStream* in) {
  if ( in->Size() < block_size_ ) {
    return READ_NO_DATA;
  }

  io::MemoryStream temp;
  temp.AppendStream(in, block_size_ - kBlockTrailerEnd);
  const int32 content_size = io::NumStreamer::ReadInt32(in, common::BIGENDIAN);
  const int32 prev_crc = io::NumStreamer::ReadInt32(in, common::BIGENDIAN);
  const int32 crc = io::NumStreamer::ReadInt32(in, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(&temp, content_size, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(&temp, prev_crc, common::BIGENDIAN);
  const int32 expected_crc = ComputeCRC(&temp);

  if ( expected_crc != crc ||
       content_size > block_size_ - kBlockTrailerEnd ) {
    LOG_ERROR << "CRC Error in I/O record: crc: " << hex << crc
              << ", expected: " << expected_crc
              << ", prev_crc: " << prev_crc << dec
              << ", content_size: " << content_size
              << ", max content size: "
              << (block_size_ - kBlockTrailerEnd);
    return READ_CRC_CORRUPTED;
  }
  RecordReader::ReadResult ret = READ_OK;
  // prev_crc is 0 when you continue writing an existing log, not an error
  // prev_block_crc_ is 0 when we continue reading an existing log
  // through seek, not an error
  if ( prev_crc != 0 && prev_block_crc_ != 0 && prev_crc != prev_block_crc_ ) {
    LOG_ERROR << "Block out of order, prev CRC mismatch"
              << ", found prev_crc: " << hex << prev_crc
              << ", expected prev crc: " << prev_block_crc_
              << ", current block crc: " << crc;
    ret = READ_CRC_BLOCK_BROKEN;
  }
  prev_block_crc_ = crc;
  CHECK_LE(content_size, block_size_ - kBlockTrailerEnd);
  content_.AppendStream(&temp, content_size);
  return ret;
}

void RecordReader::SkipRecord() {
  CHECK_GE(content_.Size(), sizeof(int32));
  uint8 flags = io::NumStreamer::ReadByte(&content_);
  int32 crt_len = io::NumStreamer::ReadUInt24(&content_, common::BIGENDIAN);
  content_.Skip(crt_len);
  skip_record_ = (flags & RecordWriter::HAS_CONT) != 0;
}

RecordReader::ReadResult RecordReader::ReadRecord(io::MemoryStream* in,
                                                  io::MemoryStream* out,
                                                  int* num_skipped,
                                                  int max_num_skip) {
  CHECK_NOT_NULL(in);
  while ( true ) {
    if ( content_.IsEmpty() ) {
      RecordReader::ReadResult ret = ReadNextBlock(in);
      if ( ret == READ_CRC_BLOCK_BROKEN && !record_content_.IsEmpty() ) {
        skip_record_ = true;
        ret = READ_OK;
      }
      if ( ret != READ_OK ) {
        return ret;
      }
    }
    // if current record is corrupt, skip all partial records
    while ( skip_record_ && !content_.IsEmpty() ) {
      ++(*num_skipped);
      SkipRecord();
      if (max_num_skip && max_num_skip <= *num_skipped) {
          return READ_OK;
      }
    }
    // if all content was exhausted, start again by reading next block
    if ( content_.IsEmpty() ) {
      continue;
    }

    // content_ is OK: verified and valid

    CHECK_GE(content_.Size(), sizeof(int32));
    uint8 flags = io::NumStreamer::ReadByte(&content_);
    int32 len = io::NumStreamer::ReadUInt24(&content_, common::BIGENDIAN);
    if ( (flags & RecordWriter::IS_FIRST) != 0 &&
         !record_content_.IsEmpty() ) {
      ++(*num_skipped);
      // after HAS_CONT found IS_FIRST, stream inconsistency.
      // We have 2 choices: return error, or continue!
      content_.Skip(len);
      record_content_.Clear();
      return READ_CRC_CORRUPTED; // chose error, no reason
    }
    if ( out != NULL &&
         record_content_.IsEmpty() &&
         (flags & RecordWriter::IS_FIRST) == 0 ) {
      // this piece does not start with IS_FIRST. It must be the continuation
      // of a previous record. (happens if a record is split part in block A,
      // part in block B and you seek to the beginning of a block B)
      content_.Skip(len);
      ++(*num_skipped);
      if (max_num_skip && max_num_skip <= *num_skipped) {
          return READ_OK;
      }
      continue;
    }
    // Test that 'out' is valid before appending. Serious performance boost when skipping records.
    if ( out != NULL ) {
      record_content_.AppendStream(&content_, len);
    } else {
      content_.Skip(len);
    }

    if ( (flags & RecordWriter::HAS_CONT) != 0 ) {
      continue;
    }

    // record is OK

    if ( out == NULL ) {
      record_content_.Clear();
      return READ_OK;
    }

    // if zipped -> unzip and append to out
    if ( (flags & RecordWriter::IS_ZIPPED) != 0 ) {
      int err = zlib_.Inflate(&record_content_, out);
      record_content_.Clear();
      return err == Z_STREAM_END ? READ_OK : READ_ZIP_CORRUPTED;
    }

    // not zipped -> just append to out
    out->AppendStream(&record_content_);
    CHECK(record_content_.IsEmpty());
    return READ_OK;
  }
  LOG_FATAL << "Unreachable code";
  return READ_OK;
}
}  // namespace io
}  // namespace whisper
