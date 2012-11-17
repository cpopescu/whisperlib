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

#ifndef __WHISPERLIB_IO_INPUT_STREAM_H__
#define __WHISPERLIB_IO_INPUT_STREAM_H__

#include <string>

#include <whisperlib/base/types.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/strutil.h>

#include <whisperlib/io/stream_base.h>
#include <whisperlib/io/iomarker.h>
#include <whisperlib/io/seeker.h>

namespace io {

class InputStream : public StreamBase {
 public:
  InputStream() : StreamBase() {
  }
  InputStream(IoMarker* marker, Seeker* seeker)
    : StreamBase(marker, seeker) {
  }
  virtual ~InputStream() {
  }

  // Reads "len" bytes of data into the given "buffer".
  // Returns the number of bytes read. Can be less than "len" if less bytes
  // are available.
  virtual int32 Read(void* buffer, int32 len) = 0;

  // Same as read, but w/ strings
  // If len == -1 read to the end of input, else read len bytes. Returns
  // anyway the number of read bytes
  inline int32 ReadString(string* s, int32 len = -1) {
    if ( len == -1 )
      len = Readable();
    string tmp;
    tmp.reserve(len);
    const int32 cb = Read(&tmp[0], len);
    s->assign(tmp.c_str(), cb);
    return cb;
  }
  // Reads a LF or a CRLF line. In fact CR characters preceding LF are ignored.
  // The CR LF characters are not returned.
  // The last piece of data before EOS is considered a line.
  inline bool ReadLine(string* s) {
    s->clear();
    while ( true ) {
      MarkerSet();
      char tmp[512] = {0};
      int32 r = Read(tmp, sizeof(tmp));
      if ( r == 0 ) {
        MarkerClear();
        return false;
      }
      char* end = ::strchr(tmp, 0x0A);
      if ( end != NULL ) {
        MarkerRestore();
        Skip(end-tmp+1);
        while ( (*end == 0x0D || *end == 0x0A) && end > tmp ) {
          end--;
        }
        s->append(tmp, end-tmp+1);
        break;
      }
      s->append(tmp, r);
      MarkerClear();
    }
    return true;
  }

  // 'Peeks' len bytes from the stream at the current position. On return the
  // read pointer is left unchanged.
  // Returns the number of bytes read or 0 in not available or operation not
  // supported.
  virtual int32 Peek(void* buffer, int32 len) = 0;

  // Passes over "len" bytes. (Advances the read head by the "len" number of
  // bytes).
  // Returns the number of bytes skipped. Can be less than "len" if less
  // bytes are available.
  virtual int64 Skip(int64 len) = 0;

  // Returns how many bytes are readable in the stream until EOS
  virtual int64 Readable() const = 0;

  // Returns (Readable() == 0) (Usually much faster)
  virtual bool IsEos() const = 0;

  // Sets the read marker in the current stream possition.
  virtual void MarkerSet() = 0;

  // Restores the read position of the associated stream to the position
  // previously marked by set.
  virtual void MarkerRestore() = 0;

  // Removes the last read mark for the underlying stream
  virtual void MarkerClear() = 0;

  // !! NOTE !!
  //  -- This is amazingly expensive !! Use it only for debugging --
  virtual string ToString() const {

// On android cannot convince it to have this defined in inttypes.h
// So may need to define it here.
#ifndef PRId64
#define PRId64 "ld"
#define __INTERNAL_PRId64_DEFINED
#endif

#ifdef _DEBUG
    InputStream& in = const_cast<InputStream&>(*this);
    const int64 size = in.Readable();
    in.MarkerSet();
    string buffer;
    in.ReadString(&buffer);
    in.MarkerRestore();
    DCHECK_EQ(in.Readable(), size);

    return (strutil::StringPrintf("InputStream [#%" PRId64 " bytes]:\n", size) +
            strutil::PrintableDataBuffer(buffer.data(), buffer.size()));
#else
    return (strutil::StringPrintf("InputStream [#%" PRId64 " bytes]:\n",
                                  Readable()));
#endif

#ifdef __INTERNAL_PRId64_DEFINED
#undef __INTERNAL_PRId64_DEFINED
#undef PRId64
#endif
  }
  virtual string PeekString() const {
    string s;
    InputStream& in = const_cast<InputStream&>(*this);
    const int64 size = in.Readable();
    in.MarkerSet();
    in.ReadString(&s);
    in.MarkerRestore();
    CHECK_EQ(in.Readable(), size);
    return s;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(InputStream);
};
inline std::ostream& operator<<(std::ostream& os,
                                const InputStream& in) {
  return os << in.ToString();
}
}
#endif  // __WHISPERLIB_IO_INPUT_STREAM_H__
