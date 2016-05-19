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

//
// A wrapper for zlib to be used with our own io::MemoryStream
//
#ifndef __WHISPERLIB_IO_ZLIB_ZLIBWRAPPER_H__
#define __WHISPERLIB_IO_ZLIB_ZLIBWRAPPER_H__

#include <zlib.h>
#include "whisperlib/io/buffer/memory_stream.h"

namespace whisper {
namespace re {
class RE;
}

namespace io {
class Compressor {
public:
    Compressor() {
    }
    virtual ~Compressor() {
    }
    virtual bool Compress(io::MemoryStream* in, io::MemoryStream* out) = 0;
};
class Decompressor {
public:
    Decompressor() {
    }
    virtual ~Decompressor() {
    }
    virtual bool Decompress(io::MemoryStream* in, io::MemoryStream* out) = 0;
};

//////////////////////////////////////////////////////////////////////

class ZlibDeflateWrapper : public Compressor {
 public:
  explicit ZlibDeflateWrapper(int compress_level = Z_DEFAULT_COMPRESSION);
  virtual ~ZlibDeflateWrapper();
  void Clear();
  // Compresses *at most* *size bytes from in and writes the result to out.
  // Updates *size to reflect the leftover bytes (*size -= in->Size())
  // If we are able to extract *size bytes from in we also flush the
  // compression trailer in out and end the compression process.
  // You can call multiple times Compress, as data becomes available in 'in'
  // Return false *iff* we got some Zlib errors.
  bool DeflateSize(io::MemoryStream* in, io::MemoryStream* out, size_t* size);

  // Compresses the entire content of in and appends it to out.
  // Returns true on success and false on some error.
  bool Deflate(io::MemoryStream* in, io::MemoryStream* out) {
    Clear();   // just in case..
    size_t size = in->Size();
    return DeflateSize(in, out, &size);
  }
  // Compresses the entire buffer of in and appends it to out.
  // Returns true on success and false on some error.
  bool Deflate(const char* in, size_t in_size, io::MemoryStream* out);

  virtual bool Compress(io::MemoryStream* in, io::MemoryStream* out) {
      return Deflate(in, out);
  }

 private:
  bool Initialize();
  const int compress_level_;
  bool initialized_;
  z_stream strm_;

  DISALLOW_EVIL_CONSTRUCTORS(ZlibDeflateWrapper);
};

//////////////////////////////////////////////////////////////////////

class ZlibInflateWrapper : public Decompressor {
 public:
  ZlibInflateWrapper();
  virtual ~ZlibInflateWrapper();
  void Clear();

  // Decompresses the entire content of in and appends it to out.
  // Return Zlib error (see bellow)
  int Inflate(io::MemoryStream* in, io::MemoryStream* out) {
    Clear();      // just in case..
    size_t size = in->Size();
    return InflateSize(in, out, &size);
  }
  // Decompresses *at most* *size bytes from in and writes the result to out.
  // Updates *size to reflect the leftover bytes (*size -= in->Size())
  // If we are able to extract *size bytes from in we also flush the
  // compression trailer in out and end the compression process.
  // You can call multiple times DecompressSize, as data becomes available
  // in 'in'.
  // A NULL size would decompress untill the stream end..
  // Returns the Zlib error:
  //   -- on Z_OK we decompressed ok up to size (or the available data in in)
  //   -- on Z_STREAM_END - the entire data begun to be decompressed
  //      was decompressed ok
  //   -- anything else - some sort of error..
  int InflateSize(io::MemoryStream* in, io::MemoryStream* out,
                  size_t* size = NULL);

  bool Decompress(io::MemoryStream* in, io::MemoryStream* out) {
      const int err = Inflate(in, out);
      return (err == Z_OK || err == Z_STREAM_END);
  }
 private:
  bool initialized_;
  z_stream strm_;

  DISALLOW_EVIL_CONSTRUCTORS(ZlibInflateWrapper);
};

////////////////////////////////////////////////////////////////////////////////

class ZlibGzipEncodeWrapper : public Compressor {
 public:
  explicit ZlibGzipEncodeWrapper(int compress_level = Z_DEFAULT_COMPRESSION);
  virtual ~ZlibGzipEncodeWrapper();

  // Compresses the entire content of in and appends it to out. It
  // writes a new gzip header, computes size and crc
  void Encode(io::MemoryStream* in, io::MemoryStream* out);

  // Begins encoding in the given buffer
  void BeginEncoding(io::MemoryStream* out);
  void ContinueEncoding(io::MemoryStream* in, io::MemoryStream* out);
  void EndEncoding(io::MemoryStream* out);

  virtual bool Compress(io::MemoryStream* in, io::MemoryStream* out) {
      Encode(in, out);
      return true;
  }

 private:
  void InitStream();
  const int compress_level_;
  z_stream strm_;
  int32 crc_;
  size_t input_size_;

  DISALLOW_EVIL_CONSTRUCTORS(ZlibGzipEncodeWrapper);
};

//////////////////////////////////////////////////////////////////////

class ZlibGzipDecodeWrapper : public Decompressor {
 public:
  explicit ZlibGzipDecodeWrapper(bool strict_trailer_checking = true);
  ~ZlibGzipDecodeWrapper();

  // Decompreses the entire content and appends the decoded content to out.
  // Returns Zlib error code.
  //   -- on Z_OK we decompressed ok up to size (or the available data in in)
  //   -- on Z_STREAM_END - the entire data begun to be decompressed
  //      was decompressed ok
  //   -- anything else - some sort of error..
  int Decode(io::MemoryStream* in, io::MemoryStream* out);

  bool Decompress(io::MemoryStream* in, io::MemoryStream* out) {
      return Decode(in, out) == Z_OK;
  }

  // Basically if we have less then this, there is no data..
  static const int kMinGzipDataSize = 18;

 private:
  void InitStream();

  enum State {
    INITIALIZED,
    DECODING,
    CHECKING,
    FINALIZED,
  };

  const bool strict_trailer_checking_;

  State state_;
  z_stream strm_;
  int last_zlib_err_;
  bool header_passed_;
  int32 running_crc_;
  size_t running_size_;

  DISALLOW_EVIL_CONSTRUCTORS(ZlibGzipDecodeWrapper);
};

static const size_t kCompressMaxChunkSize = 4 << 20;

/**
 * Compresses the files under a directory.
 * @param compressor - knowns how to compress buffers.
 * @param input_name - name of the file or directory to compress. Please note that
 *   the last '/' counts for directories: E.g
 *   If you specify a/b/ to compress, which has inside f1 and
 *   f2, when we @see InflateToDir to, say a directory c/d/ we will create
 *   c/d/b/f1 and c/d/b/f2. However, if you specify a/b for dir_name when deflating
 *   we will create c/d/f1 and c/d/f2
 * @param regex - when not null we filter the file names to compress using this.
 * @param append - when append to an already existing archive.
 * @param recursive - find files under dir_name recursively.
 * @param chunk_size - chunk the writes at this size (max 4Mb).
 * @return true on success.
 */
bool DeflateDir(Compressor* compressor,
                const std::string& dir_name,
                const re::RE* regex,
                const std::string& file_name,
                bool append, bool recursive,
                size_t chunk_size = kCompressMaxChunkSize);

/** Decompresses an archive created w/ DeflateDir.
 * @param decompressor - knows to decompress buffers compressed by its pairing
 *   compressor, that was used in DeflateDir.
 * @param file_name - compressed archive (created w/ DeflateDir)
 * @param dir_name - decompress to this directory.
 * @return true on success.
 */
bool InflateToDir(Decompressor* decompressor,
                  const std::string& file_name,
                  const std::string& dir_name);

}  // namespace io
}  // namespace whisper

#endif  // __WHISPERLIB_IO_ZLIB_ZLIBWRAPPER_H__
