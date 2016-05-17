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

#ifndef __NET_RPC_LIB_CODEC_RPC_ENCODER_H__
#define __NET_RPC_LIB_CODEC_RPC_ENCODER_H__

#include <map>
#include "whisperlib/io/num_streaming.h"
#include "whisperlib/io/buffer/memory_stream.h"

namespace whisper {
namespace codec {

class Encoder {
 public:
  // Construct an encoder that writes data to the given output stream.
  explicit Encoder(io::MemoryStream& out)
      : out_(&out),
        tmp_() {
  }
  virtual ~Encoder() {
  }

 public:
  //////////////////////////////////////////////////////////////////////
  //
  //                    Encoding methods
  //

  // Methods for serializing complex types: structure, array, map.
  virtual void EncodeStructStart(uint32 nAttribs) = 0;
  virtual void EncodeStructContinue() = 0;
  virtual void EncodeStructEnd() = 0;
  virtual void EncodeStructAttribStart() = 0;
  virtual void EncodeStructAttribMiddle() = 0;
  virtual void EncodeStructAttribEnd() = 0;
  virtual void EncodeArrayStart(uint32 nElements) = 0;
  virtual void EncodeArrayContinue() = 0;
  virtual void EncodeArrayEnd() = 0;
  virtual void EncodeArrayElementStart() = 0;
  virtual void EncodeArrayElementEnd() = 0;
  virtual void EncodeMapStart(uint32 nPairs) = 0;
  virtual void EncodeMapContinue() = 0;
  virtual void EncodeMapEnd() = 0;
  virtual void EncodeMapPairStart() = 0;
  virtual void EncodeMapPairMiddle() = 0;
  virtual void EncodeMapPairEnd() = 0;

 protected:
  // Encode the value of a base type object in the output stream.
  // The reverse of these methods is codec::Decoder::DecodeBody(..).
  virtual void EncodeBody(const bool& obj) = 0;
  virtual void EncodeBody(const int32& obj) = 0;
  virtual void EncodeBody(const uint32& obj) = 0;
  virtual void EncodeBody(const int64& obj) = 0;
  virtual void EncodeBody(const uint64& obj) = 0;
  virtual void EncodeBody(const double& obj) = 0;
  virtual void EncodeBody(const char* obj) = 0;
  virtual void EncodeBody(const std::string& obj) = 0;

  template<typename T>
  void EncodeBody(const std::vector<T>& obj) {
    uint32 count = obj.size();
    EncodeArrayStart(count);            // encode the items count
    for ( uint32 i = 0; i < count; i++ ) {
      EncodeArrayElementStart();
      Encode(obj[i]);               // encode the items themselves
      EncodeArrayElementEnd();
      if ( i + 1 < count ) {
        EncodeArrayContinue();
      }
    }
    EncodeArrayEnd();
  }

  template<typename K, typename V>
      void EncodeBody(const std::map<K, V>& obj) {
    uint32 count = obj.size();          // count
    EncodeMapStart(count);              // encode the items count
    uint32 i = 0;
    for ( typename std::map<K, V>::const_iterator it = obj.begin();
          it != obj.end(); ++it, ++i ) {
      EncodeMapPairStart();
      Encode(it->first);                // encode the key for an item
      EncodeMapPairMiddle();
      Encode(it->second);               // encode the value for an item
      EncodeMapPairEnd();
      if ( i + 1 < count ) {
        EncodeMapContinue();
      }
    }
    EncodeMapEnd();
  }

 public:
  // Generic method//
  template <typename T>
  void Encode(const T& obj) {
    EncodeBody(obj);
  }

  template <typename T>
  uint32 EstimateEncodingSize(const T& obj) {
    tmp_.Clear();

    // switch to this output ..
    io::MemoryStream* originalOut = out_;
    out_ = &tmp_;

    // encode the object (using the tmp buffer);
    Encode(obj);

    // switch back to original output
    out_ = originalOut;

    return tmp_.Size();
  }

 protected:
  // the original output stream (as given in constructor)
  io::MemoryStream* out_;
  // an additional buffer used to temporary encode
  // an object for size measurement.
  io::MemoryStream tmp_;

  DISALLOW_EVIL_CONSTRUCTORS(Encoder);
};
}
}
#endif   // __NET_RPC_LIB_CODEC_RPC_ENCODER_H__
