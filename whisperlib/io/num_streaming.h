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
// Author: Cosmin Tudorache & Catalin Popescu
//
// This contains utility functions for straming numbers
//  (in your given endianess)
//
#ifndef __WHISPERLIB_IO_NUM_STREAMING_H__
#define __WHISPERLIB_IO_NUM_STREAMING_H__

#include <algorithm>
#include <whisperlib/io/output_stream.h>
#include <whisperlib/io/input_stream.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/log.h>

namespace io {

template<class R, class W>
class BaseNumStreamer {
 private:
  struct uint24 {
    uint8 data0_;
    uint8 data1_;
    uint8 data2_;
    uint24() : data0_(0), data1_(0), data2_(0) {
    }
#   if __BYTE_ORDER == __LITTLE_ENDIAN
    explicit uint24(int32 x)
        : data0_(x),
          data1_(x >> 8),
          data2_(x >> 16) {
    }
#   elif __BYTE_ORDER == __BIG_ENDIAN
    explicit uint24(int32 x)
        : data0_((x >> 16) & 0xFF),
          data1_((x >> 8) & 0xFF),
          data2_(x & 0xFF) {
    }
#   endif
    operator int32() const {
      return (data0_ +
              (static_cast<int32>(data1_) << 8) +
              (static_cast<int32>(data2_) << 16));
    }
    operator uint32() const {
      return (data0_ +
              (static_cast<uint32>(data1_) << 8) +
              (static_cast<uint32>(data2_) << 16));
    }
  };
 public:
  template<typename N> static void SwapBytes(N* data) {
    uint8* p1 = reinterpret_cast<uint8*>(data);
    uint8* p2 = p1 + sizeof(*data) - 1;
    while ( p1 < p2 ) {
      swap(*p1++, *p2--);
    }
  }

  // max size of numbers that we can read / write
  static const size_t kMaxSize = sizeof(uint64);

 public:
  //////////////////////////////////////////////////////////////////////
  //
  // Input streaming -- by default we use the stream default endianess
  //
  template<typename N> static N ReadNumber(R* is,
                                           common::ByteOrder order) {
    COMPILE_ASSERT(sizeof(N) <= kMaxSize, size_too_big_for_ReadNumber);
    N result;
    const int32 i = is->Read(&result, sizeof(N));
    CHECK_EQ(sizeof(N), i);
    if ( common::kByteOrder != order ) {
      SwapBytes(&result);
    }
    return result;
  }
  template<typename N> static N PeekNumber(const R* is,
                                           common::ByteOrder order) {
    COMPILE_ASSERT(sizeof(N) <= kMaxSize, size_too_big_for_ReadNumber);
    N result;
    const int32 i = is->Peek(&result, sizeof(N));
    CHECK_EQ(sizeof(N), i);
    if ( common::kByteOrder != order ) {
      SwapBytes(&result);
    }
    return result;
  }


  // Pops a single byte from the buffer. If the stream is empty, it returns 0.
  // The number of bytes read are written in "bytes_read" if not NULL.
  static uint8 ReadByte(R* is) {
    return ReadNumber<uint8>(is, common::kByteOrder);
  }
  // Functions for popping various number types from the stream (written in
  // the given byteorder)
  static int16 ReadInt16(R* is,
                         common::ByteOrder order) {
    return ReadNumber<int16>(is, order);
  }
  static uint16 ReadUInt16(R* is,
                           common::ByteOrder order) {
    return ReadNumber<uint16>(is, order);
  }
  static uint32 ReadUInt24(R* is,
                           common::ByteOrder order) {
    return static_cast<uint32>(ReadNumber<uint24>(is, order));
  }
  static int32 ReadInt32(R* is,
                         common::ByteOrder order) {
    return ReadNumber<int32>(is, order);
  }
  static uint32 ReadUInt32(R* is,
                           common::ByteOrder order) {
    return ReadNumber<uint32>(is, order);
  }
  static int64 ReadInt64(R* is,
                         common::ByteOrder order) {
    return ReadNumber<int64>(is, order);
  }
  static uint64 ReadUInt64(R* is,
                          common::ByteOrder order) {
    return ReadNumber<uint64>(is, order);
  }
  static float ReadFloat(R* is,
                         common::ByteOrder order) {
    return ReadNumber<float>(is, order);
  }
  static double ReadDouble(R* is,
                           common::ByteOrder order) {
    return ReadNumber<double>(is, order);
  }

  // Functions for peeking various number types from the stream (written in
  // the given byteorder)
  static uint8 PeekByte(const R* is) {
    return PeekNumber<uint8>(is, common::kByteOrder);
  }
  static int16 PeekInt16(const R* is,
                         common::ByteOrder order) {
    return PeekNumber<int16>(is, order);
  }
  static uint16 PeekUInt16(const R* is,
                           common::ByteOrder order) {
    return PeekNumber<uint16>(is, order);
  }
  static uint32 PeekUInt24(const R* is,
                           common::ByteOrder order) {
    return static_cast<uint32>(PeekNumber<uint24>(is, order));
  }
  static int32 PeekInt32(const R* is,
                         common::ByteOrder order) {
    return PeekNumber<int32>(is, order);
  }
  static uint32 PeekUInt32(const R* is,
                           common::ByteOrder order) {
    return PeekNumber<uint32>(is, order);
  }
  static int64 PeekInt64(const R* is,
                         common::ByteOrder order) {
    return PeekNumber<int64>(is, order);
  }
  static uint64 PeekUInt64(const R* is,
                          common::ByteOrder order) {
    return PeekNumber<uint64>(is, order);
  }
  static float PeekFloat(const R* is,
                         common::ByteOrder order) {
    return PeekNumber<float>(is, order);
  }
  static double PeekDouble(const R* is,
                           common::ByteOrder order) {
    return PeekNumber<double>(is, order);
  }


  //////////////////////////////////////////////////////////////////////
  //
  // Output streaming -- by default we use the stream default endianess
  //
  template<typename N> static int32 WriteNumber(W* os, N data,
                                                common::ByteOrder order) {
    // COMPILE_CHECK(sizeof(data) <= kMaxSize);
    if ( common::kByteOrder != order ) {
      SwapBytes(&data);
    }
    return os->Write(&data, sizeof(data));
  }

  // Functions for writing various number types to the stream.
  static int32 WriteByte(W* os, uint8 byte) {
    return WriteNumber(os, byte, common::kByteOrder);
  }
  static int32 WriteInt16(W* os, int16 data,
                          common::ByteOrder order) {
    return WriteNumber<int16>(os, data, order);
  }
  static int32 WriteUInt16(W* os, uint16 data,
                           common::ByteOrder order) {
    return WriteNumber<uint16>(os, data, order);
  }
  static int32 WriteUInt24(W* os, int32 data,
                           common::ByteOrder order) {
    return WriteNumber<uint24>(os, uint24(data), order);
  }
  static int32 WriteInt32(W* os, int32 data,
                          common::ByteOrder order) {
    return WriteNumber<int32>(os, data, order);
  }
  static int32 WriteUInt32(W* os, uint32 data,
                           common::ByteOrder order) {
    return WriteNumber<uint32>(os, data, order);
  }
  static int32 WriteInt64(W* os, int64 data,
                          common::ByteOrder order) {
    return WriteNumber<int64>(os, data, order);
  }
  static int32 WriteUInt64(W* os, uint64 data,
                           common::ByteOrder order) {
    return WriteNumber<uint64>(os, data, order);
  }
  static int32 WriteFloat(W* os, float data,
                          common::ByteOrder order) {
    return WriteNumber<float>(os, data, order);
  }
  static int32 WriteDouble(W* os, double data,
                           common::ByteOrder order) {
    return WriteNumber<double>(os, data, order);
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(BaseNumStreamer);
};
typedef BaseNumStreamer<InputStream, OutputStream> IONumStreamer;

class BitArray {
public:
  BitArray();
  virtual ~BitArray();

  // Use an external data buffer.
  void Wrap(const void* data, uint32 size);
  // Make an internal copy of the data buffer.
  void Put(const void* data, uint32 size);

  // Read next 'bit_count' bits, and assemble them as a T number.
  // e.g. If current data_ contains: "01010000.."
  //      and bit_count = 4, and T = uint8 => returns: 0x05
  // e.g. If current data_ contains: "0101000110.."
  //      and bit_count = 10, and T = uint16 => returns: 0x0146
  template <typename T>
  T Peek(uint32 bit_count) {
    T t = 0;

    CHECK( bit_count <= sizeof(T) * 8 )
      << "bit_count: " << bit_count << " is bigger than return"
         " type size: " << sizeof(T) << " bytes";
    CHECK( bit_count <= Size() ) << "Not enough data, bit_count: " << bit_count
                                 << ", available bits: " << Size();

    uint32 bit_index = sizeof(T) * 8 - bit_count;
    bit_index = (bit_index / 8) * 8 + (7 - (bit_index % 8));

    BitCopy(data_ + head_, head_bit_, &t, bit_index, bit_count);

    if ( common::kByteOrder == common::LILENDIAN ) {
      BaseNumStreamer<void, void>::SwapBytes(&t);
    }

    return t;
  }
  template <typename T>
  T Read(uint32 bit_count) {
    T t = Peek<T>(bit_count);
    Skip(bit_count);
    return t;
  }

  void Skip(uint32 bit_count);

  // returns the number of available bits
  uint32 Size();

  // src_bit_index: 7 - is the first bit in '*src' (MSB)
  //                0 - is the last bit in '*src'  (LSB)
  // If you want to copy src right from the begining, use src_bit_index: 7.
  static void BitCopy(const void* void_src, uint32 src_bit_index,
                      void* void_dst, uint32 dst_bit_index,
                      uint32 bit_count);

private:
  const uint8* data_;
  uint32 data_size_;

  // true if we own the "data_" buffer
  bool data_owned_;

  // current byte index inside "data_"
  uint32 head_;
  // bit index inside current byte (i.e. inside data_[head_])
  uint32 head_bit_; // [7..0] . 7 is MSB, 0 is LSB
};
} // namespace io

#endif  // __WHISPERLIB_IO_NUM_STREAMING_H__
