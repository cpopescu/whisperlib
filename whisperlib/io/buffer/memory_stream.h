// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
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

#ifndef __WHISPERLIB_IO_BUFFER_MEMORY_STREAM_H__
#define __WHISPERLIB_IO_BUFFER_MEMORY_STREAM_H__

#include <utility>
#include <string>
#include <vector>

#include "whisperlib/base/types.h"
#include "whisperlib/io/buffer/data_block.h"
#include "whisperlib/io/num_streaming.h"

#if defined(HAVE_SYS_UIO_H)
#include <sys/uio.h>
#elif defined(__has_include)
 #if __has_include(<sys/uio.h>)
  #include <sys/uio.h>
  #define HAVE_SYS_UIO_H
 #endif
#endif

namespace whisper {
namespace io {

//
// A buffer used for passing data around without copying.
//
// Thread safe notice: this is *not* thread safe in general. However it is
// safe to use const MemoryStream-s (*just* const functions), concurrently in
// different threads (provided that nobody alters the memory stream).
//
class MemoryStream {
 public:
  MemoryStream(BlockSize block_size = DataBlock::kDefaultBufferSize);
  ~MemoryStream();

  BlockSize block_size() const { return block_size_; }
  //////////////////////////////////////////////////////////////////////
  //
  // "FAST" interface - works mostly by actioning on the underneath
  //                    data buffers and moving them around.
  //

  // Appends a raw block to the buffer - creates a new block and
  // calls AppendBlock
  // The block becomes ours and is integrated in a new DataBlock.
  // The data should be deletable (e.g. is OK to do delete [] data)
  // NEVER - AppendRaw the same buffer to two MemoryStreams !! (instead
  // append to one temp buffer and use AppendStream /
  // AppendStreamNonDestructive)
  void AppendRaw(const char* data, size_t size, Closure* disposer = NULL);

  // Appends this block to our internal list (increments the reference
  // count)
  void AppendBlock(DataBlock* data);

  // Returns a piece of data from the buffer and advances the read.
  // If not 0, *size may specify the maximum ammount ot be read..
  // The returned buffer is valid until next read / append of any kind
  // to this buffer. You do not own the pointer.
  // Retruns false on end-of-buffer.
  bool ReadNext(const char** buffer, size_t* size);

  // Reads all the internal buffers for a writev operation
#if defined(HAVE_SYS_UIO_H)
  size_t ReadForWritev(struct ::iovec** iov, int* iovcnt, size_t max_size);
#endif

  // Returns a piece of buffer already allocated and ready to be written to
  // (it is reserved and considered written). Use ConfirmScratch to confirm
  // how much you used from returned size.
  // IMPORTANT NOTE: after a GetScratchSpace the next operation w/ this
  //                 stream must be ConfirmScratch !!!
  void GetScratchSpace(char** buffer, size_t* size);
  void ConfirmScratch(size_t size);

  // Returns true if the buffer is empty.
  bool IsEmpty() const {
    return size_ <= 0;
  }

  // Returns the total amount of data in the buffer
  size_t Size() const {
    return size_;
  }

  // Appends a portion of the given data block to our list (makes a copy)
  // in a fresh new block..
  void AppendNewBlock(DataBlock* data,
                      BlockSize pos_begin, BlockSize pos_end);

  //////////////////////////////////////////////////////////////////////
  //
  // "MEDIUM" inteface functions - these may take a long time, or a small
  //                               time, depending on conditions..

  // Appends a full buffer to this one -
  void AppendStream(MemoryStream* buffer, ssize_t size = -1);

  // Appends without destroying the buffer - appends just size bytes
  // (or the entire buffer if size == -1)
  void AppendStreamNonDestructive(const MemoryStream* buffer,
                                  ssize_t size = -1);

  // As above, but appends between the pointers of the same owner
  void AppendStreamNonDestructive(DataBlockPointer* begin,
                                  const DataBlockPointer* end);

  //////////////////////////////////////////////////////////////////////
  //
  // "SLOWER" inteface functions - these may take longer as they involve
  //                               data copying and buffer allocation

  // Reads "len" bytes of data into the given "buffer".
  // Returns the number of bytes read. Can be less than "len" if less bytes
  // are available.
  size_t Read(void* buffer, size_t len) {
    return ReadInternal(buffer, len, true);
  }
  size_t ReadBuffer(void* buffer, size_t len) {
    return ReadInternal(buffer, len, true);
  }

  // Same as read, but w/ strings - this tends to be slower..
  // If len == -1 read to the end of input, else read len bytes.
  // Returns anyway the number of read bytes
  size_t ReadString(std::string* s, ssize_t len = -1);

  bool Equals(const io::MemoryStream& other) const;

  // These are really slow - do not use them unless for debugging..
  std::string ToString() {
    std::string s;
    ReadString(&s);
    return s;
  }
  std::string DebugString() const {
#if 1 //  _DEBUG
    char* t = new char[Size()];
    std::string s;
    const size_t size = Peek(t, Size());
    s.assign(t, size);
    delete [] t;
    return s;
#else
    return "!!! io::MemoryStream::DebugString in Release Mode !!!";
#endif
  }

  // Utility to read from the pointer to a CRLF. Will leave the pointer
  // after the CRLF or at the end of the buffer. Returns true (and reads)
  // a line if found. On true, s will contain the line *and* the CRLF
  bool ReadCRLFLine(std::string* s) {
    if ( !MaybeInitReadPointer() ) {
      return false;
    }
    if ( read_pointer_.ReadCRLFLine(s) ) {
      size_ -= s->size();
      return true;
    }
    return false;
  }

  // Same as above, but looks only for \n
  bool ReadLFLine(std::string* s) {
    if ( !MaybeInitReadPointer() ) {
      return false;
    }
    if ( read_pointer_.ReadLFLine(s) ) {
      size_ -= s->size();
      return true;
    }
    return false;
  }

  // Reads a line until CRLF. Leaves the stream pointer after CRLF, but does
  // not return the CRLF.
  bool ReadLine(std::string* s) {
    if ( !ReadCRLFLine(s) ) {
      return false;
    }
    const char* p = s->c_str() + s->size() - 1;
    while ( p >= s->c_str() && (*p == 0x0a || *p == 0x0d) ) {
      p--;
    }
    s->erase(p - s->c_str() + 1);
    return true;
  }

  // Read the next token - which can be:
  //  - a separator char by itself
  //  - a succession of non blank, non separator charactes
  //  - a full string beginning w/ ' or "
  //    - treats " and ' as begin of string / end of string
  //  - passes over blanks
  TokenReadError ReadNextAsciiToken(std::string* s) {
    if ( !MaybeInitReadPointer() ) {
      return TOKEN_NO_DATA;
    }
    int len;
    TokenReadError ret =  read_pointer_.ReadNextAsciiToken(s, &len);
    size_ -= len;
    return ret;
  }

  // Not actually reading len bytes into buffer :)
  size_t Peek(void* buffer, size_t len) const {
    DataBlockPointer reader(GetReadPointer());
    if ( reader.IsNull() ) {
      return 0;
    }
    return static_cast<size_t>(
        reader.ReadData(reinterpret_cast<char*>(buffer), len));
  }

  // Passes over "len" bytes. (Advances the read head by the "len" number of
  // bytes). Returns the number of bytes skipped. Can be less than "len"
  // if less bytes are available.
  size_t Skip(size_t len);

  // Clears the buffer
  void Clear();

  // Append "len" bytes of data from the given "buffer" to the stream
  // Returns the number of bytes written. Can be less than len if the stream
  // has not enough space, or negative on error.
  size_t Write(const void* buffer, size_t len);
  size_t WriteBuffer(const void* buffer, size_t len) {
      return Write(buffer, len);
  }

  // Convenience function for writing a NULL terminated string.
  size_t Write(const char* text) {
    return Write(text, strlen(text));
  }
  // Convenience function for writing a string
  size_t Write(const std::string& s) {
    return Write(s.data(), s.size());
  }

  //////////////////////////////////////////////////////////////////////
  //
  // MARKER interface (reasonaby fast, w/ exception of Restore()
  //

  // Sets the marker in the current stream possition
  void MarkerSet() {
    if ( markers_.empty() ) {
      invalid_markers_size_ = false;
    }
    markers_.push_back(std::make_pair(size_, new DataBlockPointer(read_pointer_)));
  }
  // Returns true iff the some marker is set
  bool MarkerIsSet() const {
    return !markers_.empty();
  }
  // Restores the position of the associated stream to the position
  // previously marked by set.
  void MarkerRestore() {
    DCHECK(!markers_.empty());
    read_pointer_ = *markers_.back().second;
    if ( invalid_markers_size_ ) {
      // unfortunately no smarter way for doing this, so we basically invalidate
      // it..
      MaybeInitReadPointer();
      size_ = read_pointer_.Distance(write_pointer_);
    } else {
      size_ = markers_.back().first;
    }
    delete markers_.back().second;
    markers_.pop_back();
  }
  // Removes the last mark for the underlying stream
  void MarkerClear() {
    delete markers_.back().second;
    markers_.pop_back();
    MaybeDisposeBlocks();
  }

  //////////////////////////////////////////////////////////////////////
  //
  // DEBUG functions (terribly slow)

  // Utility debug function
  std::string DumpContent(ssize_t max_size = -1) const;
  std::string DumpContentHex(ssize_t max_size = -1) const;
  std::string DumpContentInline(ssize_t max_size = -1) const;

  std::string DetailedContent() const;

  DataBlockPointer GetReadPointer() const {
    if ( read_pointer_.IsNull() && !write_pointer_.IsNull() ) {
      return DataBlockPointer(read_pointer_.owner(),
                              blocks_.begin_id(),
                              read_pointer_.pos());
    } else {
      return read_pointer_;
    }
  }

 protected:
  // Does the actual reading
  size_t ReadInternal(void* buffer, size_t len, bool dispose);

  // when the pointer advances, we may dispose some of the underlying blocks
  void MaybeDisposeBlocks();

  bool MaybeInitReadPointer() {
    if ( read_pointer_.IsNull() ) {
      if ( write_pointer_.IsNull() )
        return false;
      CHECK(read_pointer_.owner() != NULL);
      read_pointer_.set_block_id(blocks_.begin_id());
    }
    return true;
  }

  // We allocate buffer of this size by default
  const BlockSize block_size_;

  BlockDqueue blocks_;
  DataBlockPointer read_pointer_;
  DataBlockPointer write_pointer_;
  DataBlockPointer scratch_pointer_;

  // These are for marking positions in the stream
  typedef std::vector < std::pair<int, DataBlockPointer*> > MarkersList;
  MarkersList markers_;
  bool invalid_markers_size_;
  // We keep track of our internal size *all* the time - this is of
  // utmost importance in maintaining a fast response.
  // (At all times size_ == read_pointer_.Distance(write_pointer_);
  size_t size_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(MemoryStream);
};

typedef BaseNumStreamer<MemoryStream, MemoryStream> NumStreamer;
}  // namespace io
}  // namespace whisper
#endif  // __WHISPERLIB_IO_BUFFER_MEMORY_STREAM_H__
