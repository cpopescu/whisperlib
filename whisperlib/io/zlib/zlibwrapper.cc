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
// Authors: Catalin Popescu

#include "whisperlib/io/zlib/zlibwrapper.h"

namespace io {

//////////////////////////////////////////////////////////////////////
//
// ZlibDeflateWrapper
//
ZlibDeflateWrapper::ZlibDeflateWrapper(int compress_level)
  : compress_level_(compress_level),
    initialized_(false) {
  Clear();
}
ZlibDeflateWrapper::~ZlibDeflateWrapper() {
  Clear();
}

void ZlibDeflateWrapper::Clear() {
  if ( initialized_ ) {
    deflateEnd(&strm_);
  }
  strm_.zalloc = Z_NULL;
  strm_.zfree = Z_NULL;
  strm_.opaque = Z_NULL;
  strm_.avail_in = 0;
  strm_.next_in = Z_NULL;
  initialized_ = false;
}
bool ZlibDeflateWrapper::Initialize() {
  Clear();
  if ( Z_OK !=  deflateInit2(&strm_, compress_level_, Z_DEFLATED,
                             -MAX_WBITS,       // supress zlib-header
                             8, Z_DEFAULT_STRATEGY) ) {
    return false;
  }
  initialized_ = true;
  return true;
}

bool ZlibDeflateWrapper::Deflate(const char* in, int in_size,
                                 io::MemoryStream* out) {
  if ( !initialized_ && !Initialize() ) {
    return false;
  }
  int32 out_size = 0;
  strm_.avail_in = in_size;
  strm_.next_in =
      reinterpret_cast<Bytef*>(const_cast<char*>(in));

  // run deflate() on input until output buffer not full, finish
  // compression if all of source has been read in
  do {
    void* const pout = &strm_.next_out;
    out->GetScratchSpace(reinterpret_cast<char**>(pout), &out_size);
    strm_.avail_out = out_size;
    deflate(&strm_, Z_NO_FLUSH);
    out->ConfirmScratch(out_size - strm_.avail_out);
  } while ( strm_.avail_in > 0 );

  // Flush the zlib stuff..
  do {
    void* const pout = &strm_.next_out;
    out->GetScratchSpace(reinterpret_cast<char**>(pout), &out_size);
    strm_.avail_out = out_size;
    deflate(&strm_, Z_FINISH);
    out->ConfirmScratch(out_size - strm_.avail_out);
  } while ( out_size > strm_.avail_out );
  return true;
}

bool ZlibDeflateWrapper::DeflateSize(io::MemoryStream* in,
                                     io::MemoryStream* out,
                                     int32* size) {
  if ( !initialized_ && !Initialize() ) {
    return false;
  }
  int32 out_size = 0;
  while ( *size > 0 ) {
    int32 crt_size = *size;
    const char* crt_buf = NULL;
    if ( !in->ReadNext(&crt_buf, &crt_size) ) {
      break;
    }
    strm_.avail_in = crt_size;
    strm_.next_in =
      reinterpret_cast<Bytef*>(const_cast<char*>(crt_buf));
    // run deflate() on input until output buffer not full, finish
    // compression if all of source has been read in
    do {
      void* const pout = &strm_.next_out;
      out->GetScratchSpace(reinterpret_cast<char**>(pout), &out_size);
      strm_.avail_out = out_size;
      deflate(&strm_, Z_NO_FLUSH);
      out->ConfirmScratch(out_size - strm_.avail_out);
    } while ( strm_.avail_in > 0 );
    *size -= crt_size;
  }
  if ( *size <= 0 ) {
    // Flush the zlib stuff..
    do {
      void* const pout = &strm_.next_out;
      out->GetScratchSpace(reinterpret_cast<char**>(pout), &out_size);
      strm_.avail_out = out_size;
      deflate(&strm_, Z_FINISH);
      out->ConfirmScratch(out_size - strm_.avail_out);
    } while ( out_size > strm_.avail_out );
    Clear();
  }
  return true;
}

//////////////////////////////////////////////////////////////////////
//
// ZlibInflateWrapper
//
ZlibInflateWrapper::ZlibInflateWrapper()
  : initialized_(false) {
  Clear();
}
ZlibInflateWrapper::~ZlibInflateWrapper() {
  Clear();
}

void ZlibInflateWrapper::Clear() {
  if ( initialized_ ) {
    inflateEnd(&strm_);
  }
  strm_.zalloc = Z_NULL;
  strm_.zfree = Z_NULL;
  strm_.opaque = Z_NULL;
  strm_.avail_in = 0;
  strm_.next_in = Z_NULL;
  initialized_ = false;
}


int ZlibInflateWrapper::InflateSize(io::MemoryStream* in,
                                    io::MemoryStream* out,
                                    int32* size) {
  int zlib_err = Z_OK;
  if ( !initialized_ ) {
    Clear();
    zlib_err = inflateInit2(&strm_, -MAX_WBITS);
    if ( Z_OK != zlib_err ) {
      return zlib_err;
    }
    initialized_ = true;
  }
  while ( size == NULL || *size > 0 ) {
    int32 crt_size = size == NULL ? 0 : *size;
    const char* crt_buf = NULL;
    in->MarkerSet();
    if ( !in->ReadNext(&crt_buf, &crt_size) ) {
      return zlib_err;   // no error actually ..
    }
    strm_.avail_in = crt_size;
    strm_.next_in =
      reinterpret_cast<Bytef*>(const_cast<char*>(crt_buf));
    do {
      int32 out_size = 0;
      void* const pout = &strm_.next_out;
      out->GetScratchSpace(reinterpret_cast<char**>(pout), &out_size);
      strm_.avail_out = out_size;
      zlib_err = inflate(&strm_, Z_NO_FLUSH);
      DCHECK_NE(zlib_err, Z_STREAM_ERROR);   // state not clobbered
      if ( zlib_err == Z_NEED_DICT ||
           zlib_err == Z_DATA_ERROR ||
           zlib_err == Z_MEM_ERROR ) {
        out->ConfirmScratch(0);
        Clear();
        return zlib_err;
      }
      out->ConfirmScratch(out_size - strm_.avail_out);
    } while ( zlib_err == Z_OK && strm_.avail_in > 0 );
    if ( size ) {
      *size -= (crt_size - strm_.avail_in);
    }
    if ( strm_.avail_in > 0 ) {
      in->MarkerRestore();
      in->Skip(crt_size - strm_.avail_in);
    } else {
      in->MarkerClear();
    }
    if ( zlib_err == Z_STREAM_END ) {
      Clear();
      break;
    }
  }
  return zlib_err;
}

//////////////////////////////////////////////////////////////////////

//       +---+---+---+---+---+---+---+---+---+---+
//       |ID1|ID2|CM |FLG|     MTIME     |XFL|OS | (more-->)
//       +---+---+---+---+---+---+---+---+---+---+

static const uint8 kGzipHeader[] = {
  0x1f, 0x8b, 0x08, 0, 0, 0, 0, 0, 0, 0xff,
};

ZlibGzipEncodeWrapper::ZlibGzipEncodeWrapper(int compress_level)
  : compress_level_(compress_level) {
  InitStream();
}

ZlibGzipEncodeWrapper::~ZlibGzipEncodeWrapper() {
  deflateEnd(&strm_);
}
void ZlibGzipEncodeWrapper::InitStream() {
  strm_.zalloc = Z_NULL;
  strm_.zfree = Z_NULL;
  strm_.opaque = Z_NULL;
  strm_.avail_in = 0;
  strm_.next_in = Z_NULL;
  // NB - this params are the KEY for good http compression
  deflateInit2(&strm_, compress_level_, Z_DEFLATED,
               -MAX_WBITS,       // supress zlib-header
               8,
               Z_DEFAULT_STRATEGY);
  crc_ = crc32(0L, Z_NULL, 0);
  input_size_ = 0;
}

void ZlibGzipEncodeWrapper::BeginEncoding(io::MemoryStream* out) {
  CHECK_EQ(Z_OK, deflateReset(&strm_));
  crc_ = crc32(0L, Z_NULL, 0);
  input_size_ = 0;
  out->Write(kGzipHeader, sizeof(kGzipHeader));
}

void ZlibGzipEncodeWrapper::ContinueEncoding(io::MemoryStream* in,
                                             io::MemoryStream* out) {
  int32 size = in->Size();
  input_size_ += size;
  int32 out_size = 0;
  io::MemoryStream compressed;

  while ( size > 0 ) {
    int32 crt_size = size;
    const char* crt_buf = NULL;
    CHECK(in->ReadNext(&crt_buf, &crt_size));

    strm_.avail_in = crt_size;
    strm_.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(crt_buf));
    // update the CRC
    crc_ = crc32(crc_, strm_.next_in, strm_.avail_in);

    // run deflate() on input until output buffer not full, finish
    // compression if all of source has been read in
    do {
      void* const pout = &strm_.next_out;
      compressed.GetScratchSpace(reinterpret_cast<char**>(pout), &out_size);
      CHECK_GT(out_size, 0);
      strm_.avail_out = out_size;
      int z_err = deflate(&strm_, Z_NO_FLUSH);
      CHECK(z_err == Z_OK)   // some progress was made
          << " Bad z_err = " << z_err;
      compressed.ConfirmScratch(out_size - strm_.avail_out);
    } while ( strm_.avail_in > 0 );
    size -= crt_size;
  }
  CHECK_EQ(size, 0);

  // Some data was output in compressed buffer.
  // There is still more data in zlib to be flushed.
  out->AppendStream(&compressed);
}

void ZlibGzipEncodeWrapper::EndEncoding(io::MemoryStream* out) {
  int32 out_size = 0;
  io::MemoryStream compressed;
  // Flush the zlib stuff..
  int z_err;
  do {
    void* const pout = &strm_.next_out;
    compressed.GetScratchSpace(reinterpret_cast<char**>(pout), &out_size);
    CHECK_GT(out_size, 0);
    strm_.avail_out = out_size;
    z_err = deflate(&strm_, Z_FINISH);
    // some data has been written, but there is still more data to output
    // ||
    // all data has been written, there is no more data to flush in zlib
    CHECK(z_err == Z_OK || z_err == Z_STREAM_END)
          << " Bad z_err = " << z_err;
    compressed.ConfirmScratch(out_size - strm_.avail_out);
  } while ( z_err == Z_OK );

  // Write the data - finish CRC and input_size_
  out->AppendStream(&compressed);
  NumStreamer::WriteInt32(out, static_cast<int32>(crc_), common::LILENDIAN);
  NumStreamer::WriteInt32(out, input_size_, common::LILENDIAN);
}


void ZlibGzipEncodeWrapper::Encode(io::MemoryStream* in,
                                   io::MemoryStream* out) {
  BeginEncoding(out);
  ContinueEncoding(in, out);
  EndEncoding(out);
}

//////////////////////////////////////////////////////////////////////

ZlibGzipDecodeWrapper::ZlibGzipDecodeWrapper(
  bool strict_trailer_checking)
  : strict_trailer_checking_(strict_trailer_checking) {
  InitStream();
}

ZlibGzipDecodeWrapper::~ZlibGzipDecodeWrapper() {
  inflateEnd(&strm_);
}

void ZlibGzipDecodeWrapper::InitStream() {
  state_ = INITIALIZED;
  strm_.zalloc = Z_NULL;
  strm_.zfree = Z_NULL;
  strm_.opaque = Z_NULL;
  strm_.avail_in = 0;
  strm_.next_in = Z_NULL;

  inflateInit2(&strm_, -MAX_WBITS);
  running_crc_ = crc32(0L, Z_NULL, 0);
  running_size_ = 0;
}

int ZlibGzipDecodeWrapper::Decode(io::MemoryStream* in,
                                  io::MemoryStream* out) {
  if ( state_ == FINALIZED ) {
    CHECK_EQ(Z_OK, inflateReset(&strm_));
    running_crc_ = crc32(0L, Z_NULL, 0);
    running_size_ = 0;
    state_ = INITIALIZED;
  }
  if ( state_ == INITIALIZED ) {
    if ( in->Size() < sizeof(kGzipHeader) + 2 * sizeof(int32) ) {
      return Z_OK;  // not an error - waiting for more data though..
    }
    // Consume the header, crc and size..
    uint8 header[sizeof(kGzipHeader)];
    CHECK_EQ(in->Read(header, sizeof(header)), sizeof(header));
    if ( header[0] != kGzipHeader[0] || header[1] != kGzipHeader[1] ) {
      // ERROR - invalid magic:
      return Z_DATA_ERROR;
    }
    if ( (header[3] & 0xfe) != 0 ) {
      // ERROR - we don't know these things
      LOG_ERROR << " Error - we don't know to decode complicated gzip headers";
      return Z_VERSION_ERROR;
    }
    state_ = DECODING;
  }
  int zlib_err = Z_OK;
  while ( state_ == DECODING && zlib_err != Z_STREAM_END ) {
    int32 crt_size = 0;
    const char* crt_buf = NULL;
    in->MarkerSet();
    if ( !in->ReadNext(&crt_buf, &crt_size) ) {
      in->MarkerClear();
      return zlib_err;
    }

    strm_.avail_in = crt_size;
    strm_.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(crt_buf));
    do {
      int32 out_size = 0;
      void* const pout = &strm_.next_out;
      out->GetScratchSpace(reinterpret_cast<char**>(pout), &out_size);
      const Bytef* out_buf = strm_.next_out;
      strm_.avail_out = out_size;
      zlib_err = inflate(&strm_, Z_NO_FLUSH);
      DCHECK_NE(zlib_err, Z_STREAM_ERROR);   // state not clobbered
      if ( zlib_err == Z_NEED_DICT ||
           zlib_err == Z_DATA_ERROR ||
           zlib_err == Z_MEM_ERROR ) {
        out->ConfirmScratch(0);
        state_ = FINALIZED;
        return zlib_err;
      }
      running_size_ += out_size - strm_.avail_out;
      running_crc_ = crc32(running_crc_, out_buf, out_size - strm_.avail_out);
      out->ConfirmScratch(out_size - strm_.avail_out);
    } while ( zlib_err == Z_OK && strm_.avail_in > 0 );
    if ( strm_.avail_in > 0 ) {
      in->MarkerRestore();
      in->Skip(crt_size - strm_.avail_in);
    } else {
      in->MarkerClear();
    }
  }
  if ( zlib_err == Z_STREAM_END && state_ == DECODING ) {
    state_ = CHECKING;
  }
  if ( state_ == CHECKING ) {
    if ( in->Size() < 2 * sizeof(int32) ) {
      return Z_OK;
    }
    // everything should be OK
    state_ = FINALIZED;
    zlib_err = Z_STREAM_END;
    const int32 expected_crc = NumStreamer::ReadInt32(in, common::LILENDIAN);
    const int32 expected_size = NumStreamer::ReadInt32(in, common::LILENDIAN);
    if ( expected_crc != running_crc_ && strict_trailer_checking_ ) {
      LOG_WARNING << " Invalid CRC - got: 0x" << hex << running_crc_
                  << " excpected: 0x" << hex << expected_crc << dec;
      zlib_err = Z_DATA_ERROR;
    }
    if ( expected_size != running_size_ && strict_trailer_checking_ ) {
      LOG_ERROR << "Got a stream end before declared size: "
                << " expected: " << expected_size << " got: " << running_size_;
      zlib_err = Z_DATA_ERROR;
    }
  }
  return zlib_err;
}
}
