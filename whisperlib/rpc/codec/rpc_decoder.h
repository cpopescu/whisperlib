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

#ifndef __NET_RPC_LIB_CODEC_RPC_DECODER_H__
#define __NET_RPC_LIB_CODEC_RPC_DECODER_H__

#include <list>
#include <vector>

#include <whisperlib/base/types.h>
#include <whisperlib/io/buffer/memory_stream.h>

#include <whisperlib/rpc/codec/rpc_decode_result.h>

namespace codec {

class Decoder {
 public:
  // Construct a decoder that reads data from the given input stream.
  explicit Decoder(io::MemoryStream& in)
      : in_(in) {
  }
  virtual ~Decoder() {
  }

 public:
  //  Methods for unwinding complex types: structure, array, map.
  // returns:
  //  success status.
  // NOTE: in case of error or not-enough-data, the read data is not restored.
  virtual DECODE_RESULT DecodeStructStart(uint32& nAttribs) = 0;
  virtual DECODE_RESULT DecodeStructContinue(bool& bMoreAttribs) = 0;
  virtual DECODE_RESULT DecodeStructAttribStart() = 0;
  virtual DECODE_RESULT DecodeStructAttribMiddle() = 0;
  virtual DECODE_RESULT DecodeStructAttribEnd() = 0;
  virtual DECODE_RESULT DecodeArrayStart(uint32& nElements) = 0;
  virtual DECODE_RESULT DecodeArrayContinue(bool& bMoreElements) = 0;
  virtual DECODE_RESULT DecodeMapStart(uint32& nPairs) = 0;
  virtual DECODE_RESULT DecodeMapContinue(bool& bMorePairs) = 0;
  virtual DECODE_RESULT DecodeMapPairStart() = 0;
  virtual DECODE_RESULT DecodeMapPairMiddle() = 0;
  virtual DECODE_RESULT DecodeMapPairEnd() = 0;

 protected:
  //  Decode a basic type value from the input stream. Usually the object type
  //  is not stored in stream, so the decoder cannot check data correctness.
  //  You must know somehow the type of data you're expecting.
  //  These methods are the oposite of codec::Encoder::EncodeBody(..).
  // returns:
  //   -   1  if the object succeessfully loads from stream
  //   -   0  if there is not enough data in stream
  //   - (-1) error loading object. Bad data in stream.
  // NOTE: in case of error or not-enough-data, the read data is not restored.
  virtual DECODE_RESULT DecodeBody(bool& out) = 0;
  virtual DECODE_RESULT DecodeBody(int32& out) = 0;
  virtual DECODE_RESULT DecodeBody(uint32& out) = 0;
  virtual DECODE_RESULT DecodeBody(int64& out) = 0;
  virtual DECODE_RESULT DecodeBody(uint64& out) = 0;
  virtual DECODE_RESULT DecodeBody(double& out) = 0;
  virtual DECODE_RESULT DecodeBody(string& out) = 0;

#define DECODE_VERIFY(dec_op)                           \
  do {                                                  \
    const codec::DECODE_RESULT result = dec_op;           \
    if ( result != codec::DECODE_RESULT_SUCCESS ) {       \
      return result;                                    \
    }                                                   \
  } while ( false )

  template <typename T>
  DECODE_RESULT DecodeBody(vector<T>& out) {
    // decode array items count
    uint32 count;
    DECODE_VERIFY(DecodeArrayStart(count));
    if ( count != kMaxUInt32 ) {
      // TODO(cpopescu): well, this may not be that smart - to check
      out.resize(count);

      // decode elements
      bool has_more = false;
      for ( int i = 0; i < count; i++ ) {
        DECODE_VERIFY(DecodeArrayContinue(has_more));
        CHECK(has_more);
        DECODE_VERIFY(Decode(out.at(i)));
      }

      // bug-trap
      has_more = false;
      DECODE_VERIFY(DecodeArrayContinue(has_more));
      CHECK(!has_more);
      return DECODE_RESULT_SUCCESS;
    }
    list<T> tmp;

    // decode elements in a temporary list
    bool has_more = false;
    while ( true ) {
      DECODE_VERIFY(DecodeArrayContinue(has_more));
      if ( !has_more ) {
        break;
      }
      tmp.push_back(T());
      T& item = tmp.back();
      DECODE_VERIFY(Decode(item));
    }

    // copy elements from temporary list to output array
    //
    out.resize(tmp.size());
    uint32 i = 0;
    for ( typename list<T>::iterator it = tmp.begin();
          it != tmp.end(); ++it, ++i ) {
      out[i] = *it;
    }
    return DECODE_RESULT_SUCCESS;
  }

  template <typename K, typename V>
  DECODE_RESULT DecodeBody(map<K, V>& out) {
    // decode map pairs count
    //
    uint32 count;
    DECODE_RESULT result = DecodeMapStart(count);
    if ( result != DECODE_RESULT_SUCCESS ) {
      return result;
    }

    out.clear();

    // decode [key, value] pairs
    //
    bool has_more = true;
    K key;
    V value;
    while ( (result = DecodeMapContinue(has_more)) == DECODE_RESULT_SUCCESS &&
            has_more ) {
      DECODE_VERIFY(DecodeMapPairStart());
      DECODE_VERIFY(Decode(key));
      DECODE_VERIFY(DecodeMapPairMiddle());
      DECODE_VERIFY(Decode(value));
      DECODE_VERIFY(DecodeMapPairEnd());
      out.insert(make_pair(key, value));
    }
    if ( result != DECODE_RESULT_SUCCESS ) {
      return result;
    }

    // TODO(cpopescu): wtf is w/ this check here ?
    if ( count != kMaxUInt32 ) {
      CHECK_EQ(count, out.size());
    }
    return DECODE_RESULT_SUCCESS;
  }
  // TODO(cpopescu): shall we leave this around or redefine it (is needed
  //   in rpc_json_decoder.cc
  // #undef DECODE_VERIFY

 public:
  //  Reset internal state. Should be called before Decode.
  //  If a previous try to Decode failed for insufficient data,
  //  the internal state may be mangled.
  virtual void Reset() = 0;
/*
  DECODE_RESULT Decode(std::_Bit_reference out) {
    bool b = false;
    DECODE_RESULT res = DecodeBody(b);
    out = b;
    return res;
  }
*/
  //  Decode generic rpc::Object.
  // Returns:
  //   DEODE_RESULT_SUCCESS: if the object was successfully read.
  //   DECODE_RESULT_NOT_ENOUGH_DATA: if there is not enough data
  //                                  in the input buffer.
  //                                  The read data is not restored.
  //   DECODE_RESULT_ERROR: if the data does not look like the expected object.
  //          The read data is not restored.
  template<typename T>
  DECODE_RESULT Decode(T& out) {
    return DecodeBody(out);
  }

protected:
  io::MemoryStream& in_;

  DISALLOW_EVIL_CONSTRUCTORS(Decoder);
};
}


#define DECODE_EXPECTED_FIELD(dec, expected)                            \
  do {                                                                  \
    string out;                                                         \
    codec::DECODE_RESULT res = (dec)->Decode(out);                      \
    if ( res != codec::DECODE_RESULT_SUCCESS ) {                        \
      return res;                                                       \
    }                                                                   \
    if ( expected != out ) {                                            \
      DLOG_ERROR << " JsonDecoder Expecting " << expected << " got: "   \
                 << res << ": " << out;                                 \
      return codec::DECODE_RESULT_ERROR;                                \
    }                                                                   \
  } while (false)                                                       \

#define DECODE_EXPECTED_STRUCT_CONTINUE(dec)                            \
  do {                                                                  \
    bool more_attribs = false;                                          \
    DECODE_VERIFY((dec)->DecodeStructContinue(more_attribs));           \
    if ( !more_attribs ) {                                              \
      DLOG_ERROR << "Expected more attributes, but found struct end. in_"; \
      return codec::DECODE_RESULT_ERROR;                                       \
    }                                                                   \
  } while ( false )

#define DECODE_EXPECTED_STRUCT_END(dec)                                 \
  do {                                                                  \
    bool more_attribs = false;                                          \
    DECODE_VERIFY((dec)->DecodeStructContinue(more_attribs));           \
    if ( more_attribs ) {                                               \
      DLOG_ERROR << "Expected structure end, but found more attributes. "; \
      return codec::DECODE_RESULT_ERROR;                              \
    }                                                                   \
  } while ( false )

#define DECODE_EXPECTED_ARRAY_CONTINUE(dec)                             \
  do {                                                                  \
    bool more_attribs = false;                                          \
    DECODE_VERIFY((dec)->DecodeArrayContinue(more_attribs));            \
    if ( !more_attribs ) {                                              \
      DLOG_ERROR << "Expected more attributes, but found array end. in_"; \
      return codec::DECODE_RESULT_ERROR;                                       \
    }                                                                   \
  } while ( false )

#define DECODE_EXPECTED_ARRAY_END(dec)                                 \
  do {                                                                  \
    bool more_attribs = false;                                          \
    DECODE_VERIFY((dec)->DecodeArrayContinue(more_attribs));            \
    if ( more_attribs ) {                                               \
      DLOG_ERROR << "Expected array end, but found more attributes. "; \
      return codec::DECODE_RESULT_ERROR;                              \
    }                                                                   \
  } while ( false )

#define DECODE_VALUE_ATTRIB(dec, name, val)                           \
  do {                                                                \
    DECODE_VERIFY((dec)->DecodeStructAttribStart());                  \
    DECODE_EXPECTED_FIELD(dec, name);                                 \
    DECODE_VERIFY((dec)->DecodeStructAttribMiddle());                 \
    DECODE_VERIFY((dec)->Decode((val)));                              \
    DECODE_VERIFY((dec)->DecodeStructAttribEnd());                    \
  } while ( false )

#define DECODE_STREAM_ATTRIB(dec, name, val)                      \
  do {                                                            \
    DECODE_VERIFY((dec)->DecodeStructAttribStart());              \
    DECODE_EXPECTED_FIELD(dec, name);                             \
    DECODE_VERIFY((dec)->DecodeStructAttribMiddle());             \
    (val).Clear();                                                \
    DECODE_VERIFY((dec)->DecodeRaw(&(val)));                      \
    DECODE_VERIFY((dec)->DecodeStructAttribEnd());                \
  } while ( false )



#define DECODE_BEGIN_OB_ARRAY(dec, name)                 \
    do {                                              \
    bool more_elements;                               \
    DECODE_VERIFY((dec)->DecodeStructAttribStart());  \
    DECODE_EXPECTED_FIELD(dec, name);                 \
    DECODE_VERIFY((dec)->DecodeStructAttribMiddle()); \
    DECODE_VERIFY((dec)->DecodeArrayStart(count));    \
    do {                                                                \
    if ((dec)->DecodeArrayStart(count) == codec::DECODE_RESULT_SUCCESS) {

#define DECODE_END_OB_ARRAY(dec)                                           \
    DECODE_EXPECTED_ARRAY_END(dec);                                     \
    }                                                                   \
    DECODE_VERIFY((dec)->DecodeArrayContinue(more_elements)); \
    } while (more_elements);\
    } while (false)

#define DECODE_BEGIN_ARRAY(dec, name)                 \
    do {                                              \
      bool more_elements;                               \
      DECODE_VERIFY((dec)->DecodeStructAttribStart());  \
      DECODE_EXPECTED_FIELD(dec, name);                 \
      DECODE_VERIFY((dec)->DecodeStructAttribMiddle()); \
      DECODE_VERIFY((dec)->DecodeArrayStart(count));    \
      do {                                                              \
        more_elements = false;                                            \
        DECODE_VERIFY((dec)->DecodeArrayContinue(more_elements));       \
        if (!more_elements) {                                           \
          break;                                                          \
        }

#define DECODE_END_ARRAY(dec)                                           \
      } while(true);                                                      \
    } while (false)



#endif   // __NET_RPC_LIB_CODEC_RPC_DECODER_H__
