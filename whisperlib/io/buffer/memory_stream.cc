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

#include "whisperlib/io/buffer/memory_stream.h"
#include "whisperlib/base/scoped_ptr.h"

using namespace std;

namespace whisper {
namespace io {

MemoryStream::MemoryStream(BlockSize block_size)
  : block_size_(block_size),
    read_pointer_(&blocks_, -1, 0),
    write_pointer_(&blocks_, -1, 0),
    scratch_pointer_(&blocks_, -1, 0),
    invalid_markers_size_(false),
    size_(0) {
  CHECK_GT(block_size_, 0);
}

MemoryStream::~MemoryStream() {
  Clear();
}

void MemoryStream::Clear() {
  CHECK(!MarkerIsSet());
  read_pointer_ = DataBlockPointer(&blocks_, -1, 0);
  write_pointer_ = DataBlockPointer(&blocks_, -1, 0);
  for ( BlockDqueue::const_iterator it = blocks_.begin();
        it != blocks_.end(); ++it ) {
    (*it)->DecRef();
  }
  blocks_.clear();
  size_ = 0;
}

void MemoryStream::AppendRaw(const char* data, size_t size, Closure* disposer) {
  AppendBlock(new DataBlock(data, size, disposer, NULL));
}

void MemoryStream::AppendBlock(DataBlock* block) {
  block->IncRef();
  blocks_.push_back(block);
  invalid_markers_size_ = true;
  write_pointer_ = DataBlockPointer(&blocks_, blocks_.end_id() - 1,
                                    block->size());
  size_ += block->size();
}

//////////////////////////////////////////////////////////////////////

#if defined(HAVE_SYS_UIO_H)
size_t MemoryStream::ReadForWritev(struct ::iovec** iov,
                                   int* iovcnt,
                                   size_t max_size) {
  CHECK(scratch_pointer_.IsNull())
    << "Unconfirmed scratch before Read Next..";
  if ( !MaybeInitReadPointer() ) {
    return 0;
  }
  MaybeDisposeBlocks();
  const char* buffer = NULL;
  BlockSize size = 0;
  std::vector <struct ::iovec> v;
  // int initial_size = Size();
  DCHECK_EQ(read_pointer_.Distance(write_pointer_), size_);
  while ( max_size > 0 && read_pointer_.ReadBlock(&buffer, &size) ) {
    if ( size > 0 ) {
      DCHECK_GE(size_, size);
      size_ -= size;
      if (size > max_size) max_size = 0;
      else max_size -= size;
      v.push_back(::iovec());
      v.back().iov_base = const_cast<void*>(
        reinterpret_cast<const void*>(buffer));
      v.back().iov_len = size;
    }
    size = 0;
  }
  if ( v.empty() ) return 0;
  *iov = new struct ::iovec[v.size()];
  size_t sz = 0;
  for ( size_t i = 0; i < v.size(); ++i ) {
    (*iov)[i].iov_base = v[i].iov_base;
    (*iov)[i].iov_len = v[i].iov_len;
    sz += v[i].iov_len;
  }
  *iovcnt = v.size();
  DCHECK_EQ(read_pointer_.Distance(write_pointer_), size_);
  return sz;
}
#endif   // (HAVE_SYS_UIO_H)



bool MemoryStream::ReadNext(const char** buffer, size_t* size) {
  CHECK(scratch_pointer_.IsNull())
    << "Unconfirmed scratch before Read Next..";
  if ( !MaybeInitReadPointer() ) {
    return false;
  }
  MaybeDisposeBlocks();
  BlockSize psize = 0;
  if ( read_pointer_.ReadBlock(buffer, &psize) ) {
    *size = psize;
    size_ -= psize;
    return true;
  }
  return false;
}

void MemoryStream::GetScratchSpace(char** buffer, size_t* size) {
  invalid_markers_size_ = true;
  CHECK(scratch_pointer_.IsNull())
    << "Unconfirmed scratch before new scratch required..";
  if ( write_pointer_.IsNull() || write_pointer_.AvailableForWrite() == 0 ) {
    DataBlock* const new_block = new DataBlock(block_size_);
    new_block->IncRef();
    blocks_.push_back(new_block);
    write_pointer_ = DataBlockPointer(&blocks_, blocks_.end_id() - 1, 0);
  }
  scratch_pointer_ = write_pointer_;
  *buffer = ((*write_pointer_.block_it())->mutable_buffer() +
             write_pointer_.pos());
  *size = write_pointer_.AdvanceToCurrentBlockEnd();
}

void MemoryStream::ConfirmScratch(size_t size) {
  invalid_markers_size_ = true;
  CHECK(!scratch_pointer_.IsNull())
    << "Must request scratch space before confirming it !";
  write_pointer_ = scratch_pointer_;
  write_pointer_.Advance(size);
  write_pointer_.MarkCurrentBlockEndAtPointer();
  scratch_pointer_.Clear();
  size_ += size;
}

size_t MemoryStream::ReadInternal(void* buffer, size_t len, bool dispose) {
  if ( len == 0 ) {
    return 0;
  }
  if ( !MaybeInitReadPointer() ) {
    return 0;
  }
  const size_t cb =  static_cast<size_t>(
    read_pointer_.ReadData(reinterpret_cast<char*>(buffer), len));
  DCHECK(read_pointer_ <= write_pointer_);
  DCHECK_LT(read_pointer_.block_id(), blocks_.end_id());
  size_ -= cb;
  if ( dispose ) MaybeDisposeBlocks();
  return cb;
}

size_t MemoryStream::Skip(size_t len) {
  CHECK_GE(len, 0);
  if ( !MaybeInitReadPointer() ) {
    return 0;
  }
  const size_t cb = read_pointer_.Advance(len);
  DCHECK(read_pointer_ <= write_pointer_);
  size_ -= cb;
  MaybeDisposeBlocks();
  return cb;
}

size_t MemoryStream::ReadString(string* s, ssize_t len) {
  if ( len  < 0 ) len = size_;
  string tmp;
  tmp.reserve(len);
  const size_t cb = Read(&tmp[0], len);
  // this tends to be cheap but we need it in order to set the correct size
  s->assign(tmp.c_str(), cb);
  return cb;
}

bool MemoryStream::Equals(const io::MemoryStream& other) const {
  io::MemoryStream& a = const_cast<io::MemoryStream&>(*this);
  io::MemoryStream& b = const_cast<io::MemoryStream&>(other);
  if ( a.Size() != b.Size() ) {
    return false;
  }
  a.MarkerSet();
  b.MarkerSet();
  while ( true ) {
    char a_buf[128];
    const size_t a_size = a.Read(a_buf, sizeof(a_buf));
    CHECK(a_size == sizeof(a_buf) || a.IsEmpty());
    char b_buf[128];
    const size_t b_size = b.Read(b_buf, sizeof(b_buf));
    CHECK(b_size == sizeof(b_buf) || b.IsEmpty());
    if ( a_size != b_size ||
         0 != ::memcmp(a_buf, b_buf, a_size) ) {
      a.MarkerRestore();
      b.MarkerRestore();
      return false;
    }
    if ( a_size == 0 ) {
      break;
    }
  }
  a.MarkerRestore();
  b.MarkerRestore();
  return true;
}

void MemoryStream::MaybeDisposeBlocks() {
  while ( blocks_.begin_id() < read_pointer_.block_id() &&
          blocks_.begin_id() < blocks_.end_id() &&
          (markers_.empty() ||
           blocks_.begin_id() < markers_.front().second->block_id()) ) {
    blocks_.front()->DecRef();
    blocks_.pop_front();
  }
  blocks_.correct_buffer();
}

size_t MemoryStream::Write(const void* buffer, size_t len) {
  invalid_markers_size_ = true;
  if ( write_pointer_.IsNull() ) {
    DataBlock* const pblock = new DataBlock(std::max(len, size_t(block_size_)));
    pblock->IncRef();
    blocks_.push_back(pblock);
    write_pointer_ = DataBlockPointer(&blocks_, blocks_.begin_id(), 0);
  }

  const char* p = reinterpret_cast<const char*>(buffer);
  size_t crt_len = len;
  while ( crt_len > 0 ) {
    const size_t delta = write_pointer_.WriteData(p, crt_len);
    crt_len -= delta;
    p += delta;
    size_ += delta;
    if ( crt_len > 0 ) {
      DataBlock* const pblock = new DataBlock(std::max(crt_len, size_t(block_size_)));
      pblock->IncRef();
      blocks_.push_back(pblock);
    }
  }
  return len - crt_len;
}

string MemoryStream::DumpContent(ssize_t max_size) const {
  size_t size = Size();
  if ( max_size >= 0 ) size = std::min(size_t(max_size), size);
  uint8* buffer = new uint8[size];
  scoped_ptr<uint8> auto_del_buffer(buffer);
  Peek(buffer, size);
  return strutil::StringPrintf("Stream size: %d bytes, dump: ", Size()) +
         strutil::PrintableDataBuffer(buffer, size);
}

string MemoryStream::DumpContentHex(ssize_t max_size) const {
  size_t size = Size();
  if ( max_size >= 0 ) size = std::min(size_t(max_size), size);
  uint8* buffer = new uint8[size];
  scoped_ptr<uint8> auto_del_buffer(buffer);
  Peek(buffer, size);
  return strutil::StringPrintf("Stream size: %d bytes, dump: ", Size()) +
         strutil::PrintableDataBufferHexa(buffer, size);
}

string MemoryStream::DumpContentInline(ssize_t max_size) const {
  size_t size = Size();
  if ( max_size >= 0 ) size = std::min(size_t(max_size), Size());
  uint8* buffer = new uint8[size];
  scoped_ptr<uint8> auto_del_buffer(buffer);
  Peek(buffer, size);
  return strutil::PrintableDataBufferInline(buffer, size);
}

string MemoryStream::DetailedContent() const {
  string s = "";
  s += strutil::StringPrintf("  Size: %8zd\n", Size());
  s += strutil::StringPrintf("  Read Pointer: @ %8d - %8d\n",
                             read_pointer_.block_id(), read_pointer_.pos());
  s += strutil::StringPrintf(" Write Pointer: @ %8d - %8d\n",
                             write_pointer_.block_id(), write_pointer_.pos());
  if ( blocks_.empty() ) {
    s += "[ EMPTY ]\n";
  } else {
    size_t size = 0;
    BlockDqueue::const_iterator it = blocks_.buffer_it(blocks_.begin_id());
    for ( BlockId i = blocks_.begin_id(); i < blocks_.end_id(); ++i ) {
      s += strutil::StringPrintf("%4d @%4zd [%8d] %c %8d\n",
                                 i, size, (*it)->buffer_size(),
                                 (*it)->is_mutable() ? 'M' : 'F', (*it)->size());
      size += size_t((*it)->size());
      ++it;
    }
  }
  if ( markers_.empty() ) {
    s += "[ No markers ]";
  } else {
    s += "Markers:\n";
    size_t i = 0;
    for ( MarkersList::const_iterator it_markers = markers_.begin();
          it_markers != markers_.end(); ++it_markers ) {
      s += strutil::StringPrintf(
          "%4zd sz:%4zd @%4zd - %4zd\n", i, it_markers->first,
          it_markers->second->block_id(), it_markers->second->pos());
      ++i;
    }
  }
  return s;
}

//////////////////////////////////////////////////////////////////////
//
// Appends - with PERFORMANCE discussion:
//

//////////////////////////////////////////////////////////////////////
//
// Performance test:
//
//  500 rtmp simultaneous rtmp clients for h264 streaming,
//  through a switching source
//  (and rest the same) @ 560 kbps
//  on one threaded whispercast server - 2.0 GHz Xeon, Ubuntu 8.04 32 bit
//

//////////////////////////////////////////////////////////////////////
//
// NO - __APPEND_WITH_BLOCK_REUSE__
// (the rest of defines have no effect)

// CPU: 22.8%  / 358 M  -- w/ initial reserve(kResizeThreshold) for BlockDeque
// CPU: 22.2%  / 355 M  -- w/o initial reserve for BlockDeque
// #undef  __APPEND_WITH_BLOCK_REUSE__

//////////////////////////////////////////////////////////////////////
//
// with __APPEND_WITH_BLOCK_REUSE__
//

// CPU: 18.5 / 352 M -- w/ initial reserve(kResizeThreshold) for BlockDeque
// CPU: 17.4 / 348 M -- w/o no reserve for BlockDeque
//
// ==== BEST PERFORMANCE ====
//
// Other performance numbers:
//  - for 1000 simultaneous rtmp clients  (and rest the same):
//      CPU: 40.64 / 410M
//  - for 2000 simultaneous rtmp clients  (and rest the same):
//      CPU: 92.44 / 550M
//  - for h264 streaming, direct from http source (and rest the same):
//      CPU: 16.9% / 350M
//  - for h263 streaming, through switching source (and rest the same):
//      CPU: 15.5% / 335M
//  - for h263 streaming, direct from http source (and rest the same):
//      CPU: 14.58% / 335M
#define  __APPEND_WITH_BLOCK_REUSE__
#undef   __APPEND_WITH_BLOCK_REUSE_PARTIAL__
#undef   __APPEND_BLOCK_WITH_PARTIAL_BLOCK_REUSE__

////////////////////

// CPU: 24.3 / 628M  -- w/ initial reserve(kResizeThreshold) for BlockDeque
// #define  __APPEND_WITH_BLOCK_REUSE__
// #undef   __APPEND_WITH_BLOCK_REUSE_PARTIAL__
// #define  __APPEND_BLOCK_WITH_PARTIAL_BLOCK_REUSE__

//////////////////////////////////////////////////////////////////////
//
// with __APPEND_WITH_BLOCK_REUSE_PARTIAL__
//

// CPU: 19.33 / 349M  -- w/o initial reserve for BlockDeque
// #define  __APPEND_WITH_BLOCK_REUSE__
// #define  __APPEND_WITH_BLOCK_REUSE_PARTIAL__
// #undef   __APPEND_BLOCK_WITH_PARTIAL_BLOCK_REUSE__

////////////////////

// CPU: 18.66 / 375M -- w/o initial reserve for BlockDeque
// #define  __APPEND_WITH_BLOCK_REUSE__
// #define  __APPEND_WITH_BLOCK_REUSE_PARTIAL__
// #define  __APPEND_BLOCK_WITH_PARTIAL_BLOCK_REUSE__

//////////////////////////////////////////////////////////////////////

// The other version performance (in "variant/memory_stream.{h,cc}")
// CPU: 20.6 / 409 M


#ifdef __APPEND_WITH_BLOCK_REUSE__

void MemoryStream::AppendStreamNonDestructive(const MemoryStream* buffer, ssize_t size) {
  DataBlockPointer begin(buffer->GetReadPointer());
  if ( begin.IsNull() )
    return;
  DataBlockPointer end(buffer->write_pointer_);
  if ( size >= 0 ) {
      size = std::min(size_t(size), buffer->Size());
    end = begin;
    end.Advance(size);
  } else {
    size = buffer->Size();
  }
  if ( !buffer->IsEmpty() ) {
    AppendStreamNonDestructive(&begin, &end);
  }
}

void MemoryStream::AppendStreamNonDestructive(DataBlockPointer* begin,
                                              const DataBlockPointer* end) {
  if ( *begin < *end ) {
    BlockDqueue::const_iterator
        it = begin->owner()->buffer_it(begin->block_id());
    while ( begin->block_id() <= end->block_id() ) {
      const BlockSize pos_begin = begin->pos();
      const BlockSize pos_end = (begin->block_id() == end->block_id() ?
                                 end->pos() : (*it)->size());
      if ( pos_begin < pos_end ) {
#ifdef __APPEND_WITH_BLOCK_REUSE_PARTIAL__
        AppendNewBlock((*it), pos_begin, pos_end);
#else
        if ( pos_begin == 0 && pos_end == (*it)->buffer_size() )  {
          AppendBlock(*it);
        } else {
          AppendNewBlock((*it), pos_begin, pos_end);
        }
#endif
      }
      ++it;
      begin->set_block_id(begin->block_id() + 1);
      begin->set_pos(0);
    }
  }
  write_pointer_.SkipToCurrentBlockEnd();
}
void MemoryStream::AppendStream(MemoryStream* buffer, ssize_t size) {
  AppendStreamNonDestructive(buffer, size);
  if ( size >= 0 && size_t(size) < buffer->Size() ) {
    buffer->Skip(size);
  } else {
    buffer->Skip(buffer->Size());
  }
}

#else

#error "This should not be turned on"

//////////////////////////////////////////////////////////////////////
//
// Another version for appends w/ more buffer copies.
//

// !!!!!!! IMPORTANT -- this is not read-thread safe !!!!!!!
//  DO NOT ENABLE THIS CODE !!!
void MemoryStream::AppendStreamNonDestructive(const MemoryStream* buffer,
                                              ssize_t size) {
  MemoryStream* mbuffer = const_cast<MemoryStream*>(buffer);
  mbuffer->MarkerSet();
  AppendStream(mbuffer, size);
  mbuffer->MarkerRestore();
}

void MemoryStream::AppendStream(MemoryStream* buffer, ssize_t size) {
  if ( !buffer->MaybeInitReadPointer() ) {
    return;
  }
  char* my_buf;
  size_t my_size;
  GetScratchSpace(&my_buf, &my_size);

  size_t to_read = size == size_t(0) ? buffer->Size() : std::min(size, buffer->Size());
  size_t their_size = buffer->Read(my_buf, std::min(to_read, my_size));
  ConfirmScratch(their_size);
  if (to_read > their_size) {
      to_read -= their_size;
      my_buf = new char[to_read];
      CHECK_EQ(buffer->Read(my_buf, to_read), to_read);
      AppendRaw(my_buf, to_read);
  }
}

#endif  //  __APPEND_WITH_BLOCK_REUSE__

void MemoryStream::AppendNewBlock(DataBlock* data,
                                  BlockSize pos_begin, BlockSize pos_end) {
  DCHECK_GE(pos_end, pos_begin);
  size_t size = pos_end - pos_begin;

  if ( size > 0 ) {
#if defined(__APPEND_BLOCK_WITH_PARTIAL_BLOCK_REUSE__)
    size_t available = write_pointer_.AvailableForWrite();
    size_t delta = 0;
    if ( available > 0 ) {
       delta = Write(data->buffer() + pos_begin, std::min(available, size));
    }
    if ( delta < size ) {
      AppendBlock(new DataBlock(data->buffer() + pos_begin + delta,
                                size - delta,
                                data->GetAllocBlock()));
    }
#else
    Write(data->buffer() + pos_begin, size);
#endif  // __APPEND_BLOCK_WITH_PARTIAL_BLOCK_REUSE__
  }
}
}  // namespace io
}  // namespace whisper
