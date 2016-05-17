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

#include "whisperlib/base/log.h"
#include "whisperlib/io/buffer/data_block.h"

using namespace std;

namespace whisper {
namespace io {

//////////////////////////////////////////////////////////////////////
//

DataBlock::DataBlock(BlockSize buffer_size)
    : RefCounted(),
      writable_buffer_(new char[buffer_size]),
      readable_buffer_(writable_buffer_),
      alloc_block_(NULL),
      buffer_size_(buffer_size),
      size_(0),
      disposer_(NULL) {
  CHECK_GT(buffer_size_, 0);
}

// Constructs a raw buffer - we do not own it so we cannot write it
DataBlock::DataBlock(const char* buffer,
                     BlockSize size,
                     Closure* disposer,
                     DataBlock* alloc_block)
    : RefCounted(),
      writable_buffer_(NULL),
      readable_buffer_(buffer),
      alloc_block_(alloc_block),
      buffer_size_(size),
      size_(size),
      disposer_(disposer) {
  CHECK_GT(size_, 0);
}

DataBlock::~DataBlock() {
  if ( alloc_block_ != NULL ) {
    alloc_block_->DecRef();
  } else {
    if ( disposer_ != NULL ) {
      disposer_->Run();
    } else {
      delete[] readable_buffer_;
    }
  }
}


//////////////////////////////////////////////////////////////////////

BlockSize DataBlockPointer::ReadableSize() const {
  BlockSize size = 0;
  DCHECK_LE(pos_, block()->size());
  BlockDqueue::const_iterator it = block_it();
  for ( BlockId i = block_id_; i < owner_->end_id(); ++i ) {
    size += (*it)->size();
    ++it;
  }
  return size - pos_;
}

BlockSize DataBlockPointer::Distance(const DataBlockPointer& m) const {
  DCHECK(m.owner_ == owner_);
  if ( m < *this )
    return m.Distance(*this);
  BlockSize distance = 0;
  BlockDqueue::const_iterator it = block_it();
  for ( BlockId i = block_id_; i < m.block_id(); ++i ) {
    distance += (*it)->size();
    ++it;
  }
  return distance - pos_ + m.pos_;
}

BlockSize DataBlockPointer::Advance(BlockSize cb) {
  if ( cb < 0 ) {
    return Devance(-cb);
  }
  BlockSize delta = 0;
  BlockDqueue::const_iterator it = block_it();
  while ( cb > 0 || pos_ == (*it)->size() ) {
    if ( pos_ == (*it)->size() ) {
      if ( block_id_ + 1 >= owner_->end_id() ) {
        return delta;
      }
      ++block_id_;
      ++it;
      pos_ = 0;
    } else {
      const BlockSize to_add = min((*it)->size() - pos_, cb);
      pos_ += to_add;
      cb -= to_add;
      delta += to_add;
      DCHECK_LE(pos_, (*it)->size());
    }
  }
  return delta;
}

BlockSize DataBlockPointer::Devance(BlockSize cb) {
  if ( cb < 0 ) {
    return Advance(-cb);
  }
  BlockSize delta = 0;
  BlockDqueue::const_iterator it = block_it();
  while ( cb > 0 || pos_ == 0 ) {
    if ( pos_ == 0 ) {
      if ( block_id_ == owner_->begin_id() ) {
        return delta;
      }
      --block_id_;
      --it;
      pos_ = (*it)->size();
    } else {
      const BlockSize to_del = min(pos_, cb);
      pos_ -= to_del;
      cb -= to_del;
      delta += to_del;
      DCHECK_GE(pos_, 0);
    }
  }
  return delta;
}

BlockSize DataBlockPointer::WriteData(const char* buffer, BlockSize len) {
  BlockSize cb = 0;
  BlockDqueue::const_iterator it = block_it();
  while ( len > 0 ) {
    if ( pos_ == (*it)->buffer_size() ) {
      if ( block_id_ + 1 >= owner_->end_id() ) {
        return cb;
      }
      ++block_id_;
      ++it;
      // No stomping over data !
      CHECK_EQ((*it)->size(), 0);
      pos_ = 0;
    } else {
      const BlockSize to_write = min((*it)->buffer_size() - pos_, len);
      DCHECK_GT(to_write, 0);
      DCHECK((*it)->is_mutable());
      memcpy((*it)->mutable_buffer() + pos_, buffer, to_write);
      (*it)->set_size((*it)->size() + to_write);
      pos_ += to_write;
      cb += to_write;
      len -= to_write;
      buffer += to_write;
    }
  }
  return cb;
}

bool DataBlockPointer::ReadBlock(const char** buffer, BlockSize* len) {
  BlockDqueue::const_iterator it = block_it();
  while ( pos_ == (*it)->size() ) {
    if ( !AdvanceToNextBlock(&it) ) {
      *len = 0;
      return false;
    }
  }
  *buffer = (*it)->buffer() + pos_;
  if ( *len > 0 ) {
    *len = min(*len, (*it)->size() - pos_);
  } else {
    *len = (*it)->size() - pos_;
  }
  pos_ += *len;
  return true;
}

BlockSize DataBlockPointer::ReadData(char* buffer, BlockSize len) {
  BlockSize cb = 0;
  BlockDqueue::const_iterator it = block_it();
  while ( len > 0 ) {
    if ( pos_ == (*it)->size() ) {
      if ( !AdvanceToNextBlock(&it) ) {
        return cb;
      }
    } else {
      const BlockSize to_read = min((*it)->size() - pos_, len);
      DCHECK_GT(to_read, 0);
      memcpy(buffer, (*it)->buffer() + pos_, to_read);
      pos_ += to_read;
      cb += to_read;
      len -= to_read;
      buffer += to_read;
    }
  }
  return cb;
}

bool DataBlockPointer::AdvanceToNextBlock(
  BlockDqueue::const_iterator* it) {
  DCHECK(pos_ == (**it)->size());
  // Determine if we are at the very end ..
  BlockDqueue::const_iterator it_next = *it + 1;
  BlockId block_id = block_id_ + 1;
  while ( block_id < owner_->end_id() && (*it_next)->size() == 0 ) {
    ++it_next;
    ++block_id;
  }
  if ( block_id >= owner_->end_id() ) {
    return false;
  }
  block_id_ = block_id;
  *it = it_next;
  pos_ = 0;
  return true;
}

}
namespace {

enum CharAttr {
  ERROR = 1,
  QUOTE = 2,
  SEP   = 4,
  CHAR  = 8,
  SPACE = 16,
};
const unsigned char kCharLookup[0x100] = {
//   NULL     control chars...

     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  SPACE,  SPACE,  ERROR,  ERROR,  SPACE,  ERROR,  ERROR,
//   control chars...
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,
//   ' '      !        "        #        $        %        &        '        (        )        *        +        ,        -        .        /
     SPACE,   SEP,   QUOTE | SEP,  SEP,  SEP,    SEP,  SEP,    QUOTE | SEP,    SEP,    SEP,    SEP,    CHAR,    SEP,     CHAR,    CHAR,    CHAR,
//   0        1        2        3        4        5        6        7        8        9        :        ;        <        =        >        ?
     CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    SEP,     SEP,     SEP,     SEP,     SEP,     CHAR,
//   @        A        B        C        D        E        F        G        H        I        J        K        L        M        N        O
     CHAR,     CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,   CHAR,   CHAR,     CHAR,    CHAR,
//   P        Q        R        S        T        U        V        W        X        Y        Z        [        \        ]        ^        _
     CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    SEP,     SEP,     SEP,     SEP,     CHAR,
//   `        a        b        c        d        e        f        g        h        i        j        k        l        m        n        o
     SEP,     CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,     CHAR,
//   p        q        r        s        t        u        v        w        x        y        z        {        |        }        ~        <NBSP>
     CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    CHAR,    SEP,     SEP,     SEP,     SEP,     CHAR,
//   ...all the high-bit characters are escaped
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,
     ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR,  ERROR};

inline bool AttrIsSpace(unsigned char c) {
  return  (kCharLookup[c] & SPACE) != 0;
}
inline bool AttrIsQuote(unsigned char c) {
  return  (kCharLookup[c] & QUOTE) != 0;
}
inline bool AttrIsError(unsigned char c) {
  return  (kCharLookup[c] & ERROR) != 0;
}
inline bool AttrIsSep(unsigned char c) {
  return  (kCharLookup[c] & SEP) != 0;
}
inline bool AttrIsChar(unsigned char c) {
  return  (kCharLookup[c] & CHAR) != 0;
}
}

namespace io {
TokenReadError DataBlockPointer::ReadNextAsciiToken(string* s,
                                                    int* len_covered) {
  *len_covered = 0;
  BlockSize len = 0;
  DataBlockPointer saved(*this);
  DataBlockPointer begin(*this);
  BlockDqueue::const_iterator it = block_it();
  bool last_was_escape = false;
  char quote_char = '\0';

  int skipped = 0;
  while ( true ) {
    if ( pos_ == (*it)->size() ) {
      if ( !AdvanceToNextBlock(&it) ) {
        break;
      }
    } else {
      const char* p = (*it)->buffer() + pos_;
      while ( pos_ < (*it)->size() ) {
        if ( !quote_char ) {
          //
          // Outside quoted stream
          //
          if ( AttrIsError(*p) ) {
            // We accept these in quotes.. somehow..
            *this = saved;
            return TOKEN_ERROR_CHAR;
          } else if ( len ) {
            if ( AttrIsChar(*p) ) {
              ++len;
            } else {
              *this = begin;
              ReadStringData(s, len);
              *len_covered = len + skipped;
              return TOKEN_OK;
            }
          } else if ( AttrIsQuote(*p) ) {
            begin = *this;
            quote_char = *p;
            ++len;
          } else if ( AttrIsSep(*p) ) {
            s->assign(1, *p);
            Advance(1);
            *len_covered = 1 + skipped;
            return TOKEN_SEP_OK;
          } else if ( AttrIsSpace(*p) ) {
            ++skipped;
          } else {
            begin = *this;
            ++len;
          }
        } else {
          //
          // In a quoted stream
          //
          if ( quote_char == *p && !last_was_escape ) {
            *this = begin;
            ReadStringData(s, len + 1);  // include the last quote
            *len_covered = len + skipped + 1;
            return TOKEN_QUOTED_OK;
          } else if ( !last_was_escape && *p == '\\' ) {
            last_was_escape = true;
          } else {
            last_was_escape = false;
          }
          ++len;
        }
        ++pos_;
        ++p;
      }
    }
  }
  *this = saved;
  *len_covered = 0;
  return TOKEN_NO_DATA;
}

bool DataBlockPointer::ReadToChars(char fin, char prev, string* s) {
  BlockSize len = 0;
  char last = '\0';
  DataBlockPointer saved(*this);
  BlockDqueue::const_iterator it = block_it();
  while ( true ) {
    if ( pos_ == (*it)->size() ) {
      if ( !AdvanceToNextBlock(&it) ) {
        break;
      }
    } else {
      const char* p = (*it)->buffer() + pos_;
      while ( pos_ < (*it)->size() ) {
        ++pos_;
        ++len;
        if ( *p == fin && (!prev || last == prev) ) {
          *this = saved;
          ReadStringData(s, len);
          return true;
        }
        last = *p++;
      }
    }
  }
  *this = saved;
  return false;
}

BlockSize DataBlockPointer::ReadStringData(string* s, BlockSize len) {
  string tmp;
  tmp.reserve(len);
  const BlockSize cb = ReadData(&tmp[0], len);
  s->assign(tmp.c_str(), cb);
  return cb;
}

void DataBlockPointer::ReadToString(string* s) {
  const BlockSize len = ReadableSize();
  CHECK_EQ(ReadStringData(s, len), len);
}
//////////////////////////////////////////////////////////////////////

}  // namespace io
}  // namespace whisper
