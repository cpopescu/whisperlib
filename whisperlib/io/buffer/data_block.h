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

//////////////////////////////////////////////////////////////////////
//
// VERY IMPORTANT : ********  Not Thread Safe  ********
//
//////////////////////////////////////////////////////////////////////

#ifndef __WHISPERLIB_IO_BUFFER_DATA_BUFFER_H__
#define __WHISPERLIB_IO_BUFFER_DATA_BUFFER_H__

#define __USE_VECTOR_FOR_BLOCK_DQUEUE__
#ifdef __USE_VECTOR_FOR_BLOCK_DQUEUE__
#include <vector>
#else
#include <deque>
#endif

#include <string>
#include <whisperlib/base/types.h>
#include <whisperlib/base/callback.h>
#include <whisperlib/io/input_stream.h>
#include <whisperlib/io/output_stream.h>
#include <whisperlib/io/iomarker.h>
#include <whisperlib/io/seeker.h>
#include <whisperlib/sync/mutex.h>
#include <whisperlib/base/ref_counted.h>

namespace io {

typedef int32 BlockId;
typedef int32 BlockSize;

enum TokenReadError {
  TOKEN_OK = 0,
  TOKEN_QUOTED_OK = 1,
  TOKEN_SEP_OK = 2,
  TOKEN_ERROR_CHAR = 3,
  TOKEN_NO_DATA = 4,
};

class DataBlock;

#ifdef __USE_VECTOR_FOR_BLOCK_DQUEUE__

const int32 kResizeThreshold = 25;
class BlockDqueue : public vector<DataBlock*> {
 public:
  BlockDqueue()
    : vector<DataBlock*>(),
      begin_id_(0),
      correction_(0) {
    // This reserve would make the performance slightly worse in whispercast
    // reserve(kResizeThreshold);
  }
  const DataBlock* block(BlockId id) const {
    return *(buffer_it(id));
  }
  DataBlock* mutable_block(BlockId id) const {
    return *(buffer_it(id));
  }
  BlockId begin_id() const {
    return begin_id_ + correction_;
  }
  BlockId end_id() const {
    return vector<DataBlock*>::size() + correction_;
  }
  void pop_front() {
    begin_id_++;
  }
  BlockDqueue::const_iterator buffer_it(BlockId id) const {
    return vector<DataBlock*>::begin() + (id - correction_);
  }
  const DataBlock* front() const {
    return *begin();
  }
  DataBlock* front() {
    return *begin();
  }
  BlockDqueue::const_iterator begin() const {
    return vector<DataBlock*>::begin() + begin_id_;
  }
  BlockDqueue::iterator begin() {
    return vector<DataBlock*>::begin() + begin_id_;
  }
  int32 size() const {
    return vector<DataBlock*>::size() - begin_id_;
  }
  bool empty() const {
    return begin_id_ == vector<DataBlock*>::size();
  }
  void clear() {
    vector<DataBlock*>::clear();
    correction_ += begin_id_;
    begin_id_ = 0;
  }
  DataBlock* operator[](size_t n) {
    return *(begin() + n);
  }
  const DataBlock* operator[](size_t n) const {
    return *(begin() + n);
  }

  void correct_buffer() {
    const int32 elem_size = size();
    if ( begin_id_ > kResizeThreshold && begin_id_ > elem_size ) {
      vector<DataBlock*>::iterator it = vector<DataBlock*>::begin();
      for ( int32 i = 0; i < elem_size; ++i ) {
        *it = *(it + begin_id_);
        ++it;
      }
      vector<DataBlock*>::resize(elem_size);
      correction_ += begin_id_;
      begin_id_ = 0;
    }
  }
 private:
  BlockId begin_id_;
  BlockId correction_;
  DISALLOW_EVIL_CONSTRUCTORS(BlockDqueue);
};


#else

//////////////////////////////////////////////////////////////////////

class BlockDqueue : public deque<DataBlock*> {
 public:
  BlockDqueue()
    : deque<DataBlock*>(),
      begin_id_(0) {
  }
  const DataBlock* block(BlockId id) const {
    DCHECK_GE(id, begin_id_);
    DCHECK_LT(id, end_id());
    return *(begin() + (id - begin_id_));
  }
  DataBlock* mutable_block(BlockId id) const {
    DCHECK_GE(id, begin_id_);
    DCHECK_LT(id, end_id());
    return *(begin() + (id - begin_id_));
  }
  BlockId begin_id() const {
    return begin_id_;
  }
  BlockId end_id() const {
    return begin_id_ + size();
  }
  void pop_front() {
    begin_id_++;
    deque<DataBlock*>::pop_front();
  }
  BlockDqueue::const_iterator buffer_it(BlockId id) const {
    DCHECK_GE(id, begin_id_);
    DCHECK_LT(id, end_id());
    return begin() + (id - begin_id_);
  }
  void correct_buffer() {
  }

 private:
  BlockId begin_id_;
  DISALLOW_EVIL_CONSTRUCTORS(BlockDqueue);
};

#endif

//////////////////////////////////////////////////////////////////////

class DataBlock : public RefCounted {
 public:
  static const BlockSize kDefaultBufferSize = 4096; // 16384;
  // Constructs a buffer that is writable w/ a given size
  explicit DataBlock(BlockSize buffer_size = kDefaultBufferSize);
  // Constructs a raw buffer - we do not own it so we cannot write it
  explicit DataBlock(const char* buffer, BlockSize size,
                     Closure* disposer,
                     DataBlock* alloc_block);

  ~DataBlock();

  // Accessors
  BlockSize buffer_size() const {
    return buffer_size_;
  }
  BlockSize size() const {
    return size_;
  }
  void set_size(BlockSize size) {
    DCHECK_LE(size, buffer_size_);
    size_ = size;
  }

  bool is_mutable() const {
    return writable_buffer_ != NULL;
  }
  const char* buffer() const {
    return readable_buffer_;
  }
  char* mutable_buffer() {
    DCHECK_EQ(writable_buffer_, readable_buffer_);
    return writable_buffer_;
  }
  DataBlock* GetAllocBlock()  {
    return (alloc_block_ == NULL) ? this : alloc_block_;
  }

 private:

// This should be big on the servers, but on client, just to avoid
// contention between several threads.
#ifdef WHISPER_DATA_MUTEX_POOL_SIZE
  static const int kNumMutexes = WHISPER_DATA_MUTEX_POOL_SIZE;
#else
  static const int kNumMutexes = 1024;
#endif
  static synch::MutexPool mutex_pool_;

  // The beginning of the writable memory buffer (normally, if writable,
  // writable_buffer_ == readable_buffer_
  char* writable_buffer_;
  // The beginning of the readable memory buffer
  const char* readable_buffer_;
  // The allocation belongs to this guy..
  DataBlock* const alloc_block_;
  // Size of the buffer - total allocated memory
  const BlockSize buffer_size_;
  // Size of the data in the buffer
  BlockSize size_;

  // If we should call some callback in order to dispose
  Closure* const disposer_;

  DISALLOW_EVIL_CONSTRUCTORS(DataBlock);
};

//////////////////////////////////////////////////////////////////////

class DataBlockPointer {
 public:
  DataBlockPointer(const BlockDqueue* owner,
                   BlockId block_id,
                   BlockSize pos)
    : owner_(owner),
      block_id_(block_id),
      pos_(pos) {
  }
  ~DataBlockPointer() {
  }

  const BlockDqueue* owner() const { return owner_; }
  BlockId block_id() const { return block_id_; }
  BlockSize pos() const { return pos_; }
  void set_pos(BlockSize pos) { pos_ = pos; }
  void set_block_id(BlockId block_id) { block_id_ = block_id; }

  //////////////////////////////////////////////////////////////////////
  //
  // IMPORTANT: these are expensive - use with care !!
  //
  const DataBlock* block() const {
    return owner_->block(block_id_);
  }
  DataBlock* mutable_block() const {
    return owner_->mutable_block(block_id_);
  }
  BlockDqueue::const_iterator block_it() const {
    return owner_->buffer_it(block_id_);
    // return owner_->begin() + (block_id_ - owner_->begin_id());
  }
  //////////////////////////////////////////////////////////////////////

  void Clear() {
    block_id_ = -1;
    pos_ = -1;
  }
  bool IsNull() const {
    return block_id_ == -1;
  }
  // Returns the space available for write in the current block
  BlockSize AvailableForWrite() const {
    if ( IsNull() ) return 0;
    return block()->buffer_size() - pos_;
  }
  // Advances the pointer to the end of current block, marking the data
  // in between as written.
  BlockSize AdvanceToCurrentBlockEnd() {
    if ( IsNull() ) return 0;
    DataBlock* block = mutable_block();
    const BlockSize ret = block->buffer_size() - pos_;
    block->set_size(block->buffer_size());
    pos_ = block->size();
    return ret;
  }
  void SkipToCurrentBlockEnd() {
    if ( IsNull() ) return ;
    pos_ = block()->size();
  }
  // Marks the current block size to the current pointer position
  void MarkCurrentBlockEndAtPointer() {
    mutable_block()->set_size(pos_);
  }

  // Operators:
  const DataBlockPointer& operator=(const DataBlockPointer& m) {
    CHECK(owner_ == m.owner_);
    pos_ = m.pos_;
    set_block_id(m.block_id());
    return *this;
  }
  bool operator<(const DataBlockPointer& m) const {
    CHECK(m.owner_ == owner_);
    if ( block_id_ == m.block_id_ )
      return pos_ < m.pos_;
    return block_id_ < m.block_id_;
  }
  bool operator>(const DataBlockPointer& m) const {
    CHECK(m.owner_ == owner_);
    if ( block_id_ == m.block_id_ )
      return pos_ > m.pos_;
    return block_id_ > m.block_id_;
  }
  bool operator==(const DataBlockPointer& m) const {
    DCHECK(m.owner_ == owner_);
    return ( block_id_ == m.block_id_ ) && (pos_ == m.pos_);
  }
  bool operator<=(const DataBlockPointer& m) const {
    return (*this < m) || (*this == m);
  }
  bool operator>=(const DataBlockPointer& m) const {
    return (*this > m) || (*this == m);
  }

  // Returns how much data is available for read in the owning queue, starting
  // from the current position
  BlockSize ReadableSize() const;

  // Returns the distance in *valid* bytes between two pointers of the
  // same container
  BlockSize Distance(const DataBlockPointer& m) const;

  // Moves the pointer forward in the data list of block w/ cb bytes
  // Returns how much it was advanced.
  BlockSize Advance(BlockSize cb);

  // Moves the pointer backwords in the list of block w/ cb bytes
  // Returns how much it was devanced.
  BlockSize Devance(BlockSize cb);

  // Appends data at the current position - extending in the neighbouring blocks
  // (if available). We update our position at the end ot the written data.
  BlockSize WriteData(const char* buffer, BlockSize len);

  // Reads data starting w/ current position and continuing until we
  // read len bytes or we reach the end of the owner buffer chain;
  // We return the # of read bytes.
  BlockSize ReadData(char* buffer, BlockSize len);

  // Reads at most len bytes into s (Same as above)
  BlockSize ReadStringData(string* s, BlockSize len);

  // Reads the entire available data to given string
  void ReadToString(string* s);

  // Utility to read from the pointer to a CRLF. Will leave the pointer
  // after the CRLF or at the start position. Returns true (and reads)
  // a line if found. On true, s will contain the line *and* the CRLF
  bool ReadCRLFLine(string* s) {
    return ReadToChars('\n', '\r', s);
  }

  // Same as above, but looks only for \n
  bool ReadLFLine(string* s) {
    return ReadToChars('\n', '\0', s);
  }

  // Utility to read a token from a string.
  TokenReadError ReadNextAsciiToken(string* s, int* len_covered);

  // This will return in buffer and len the acutal block pointer and size in
  // the underneath data list.
  bool ReadBlock(const char** buffer, BlockSize* len);

 private:
  // Helper - if we are at the end of a block, it advances the pointer
  // to the beginning of the next block.
  bool AdvanceToNextBlock(BlockDqueue::const_iterator* it);

  // Helper to read from the pointer into s until the two chars are
  // found: fin at the end and prev before that. (If prev == '\0'
  // then the prev condition is ignored).
  bool ReadToChars(char fin, char prev, string* s);

  const BlockDqueue* const owner_;  // which container owns the iterator ?
  BlockId block_id_;                // points to the block in owner
  BlockSize pos_;                   // the position in the *
};

inline ostream& operator<<(ostream& os, const DataBlockPointer& dbp) {
  return os << "[DBP " << dbp.block_id() << ":" << dbp.pos() << "]";
}

//////////////////////////////////////////////////////////////////////
}
#endif  // __WHISPERLIB_IO_BUFFER_DATA_BUFFER_H__
