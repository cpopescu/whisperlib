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

#ifndef __NET_RPC_LIB_CODEC_JSON_RPC_JSON_ENCODER_H__
#define __NET_RPC_LIB_CODEC_JSON_RPC_JSON_ENCODER_H__

#include <string>
#include <whisperlib/base/types.h>
#include <whisperlib/rpc/codec/rpc_encoder.h>

namespace codec {

class JsonEncoder : public codec::Encoder {
 public:
  explicit JsonEncoder(io::MemoryStream& out)
      : codec::Encoder(out),
        encoding_map_key_(false) {
  }
  virtual ~JsonEncoder() {
  }

  template<class C>
  static void EncodeToString(const C& obj, string* s) {
    io::MemoryStream mos;
    codec::JsonEncoder encoder(mos);
    encoder.Encode(obj);
    mos.ReadString(s);
  }

  template<class C>
  static string EncodeObject(const C& obj) {
    io::MemoryStream iostream;
    JsonEncoder encoder(iostream);
    encoder.Encode(obj);
    return iostream.ToString();
  }

  //////////////////////////////////////////////////////////////////////
  //
  //                   codec::Encoder interface methods
  //
  void EncodeStructStart(uint32)          { out_->Write("{");   }
  void EncodeStructContinue()             { out_->Write(", ");  }
  void EncodeStructEnd()                  { out_->Write("}");   }
  void EncodeStructAttribStart()          {  }
  void EncodeStructAttribMiddle()         { out_->Write(": ");  }
  void EncodeStructAttribEnd()            {  }
  void EncodeArrayStart(uint32)           { out_->Write("[");   }
  void EncodeArrayContinue()              { out_->Write(", ");  }
  void EncodeArrayEnd()                   { out_->Write("]");   }
  void EncodeArrayElementStart()          {  }
  void EncodeArrayElementEnd()            {  }
  void EncodeMapStart(uint32)             { out_->Write("{");   }
  void EncodeMapContinue()                { out_->Write(", ");  }
  void EncodeMapEnd()                     { out_->Write("}");   }
  void EncodeMapPairStart()               { encoding_map_key_ = true;    }
  void EncodeMapPairMiddle()              { out_->Write(": ");
                                            encoding_map_key_ = false;   }
  void EncodeMapPairEnd()                 {  }

 private:

#define PrintBody(format, object)                                       \
  do {                                                                  \
      if ( !encoding_map_key_ ) {                                       \
        out_->Write(strutil::StringPrintf(                              \
                        format" ", object));                            \
      } else {                                                          \
        out_->Write(strutil::StringPrintf(                              \
                        "\"" format "\"", object));                       \
      }                                                                 \
    } while(false)

 protected:
  void EncodeBody(const bool& obj);
  void EncodeBody(const int32& obj) {
    PrintBody("%d", obj);
  }
  void EncodeBody(const uint32& obj) {
    PrintBody("%u", obj);
  }
  void EncodeBody(const int64& obj) {
    PrintBody("%lld", (long long int)(obj));
  }
  void EncodeBody(const uint64& obj) {
    PrintBody("%llu", (long long unsigned int)(obj));
  }
  void EncodeBody(const double& obj) {
    PrintBody("%.60e", obj);
  }
  void EncodeBody(const string& obj);
  void EncodeBody(const char* obj);

 private:
  // if true we're encoding the key of a map
  // usefull in encoding map<int, ... > as map<string, ... >
  // because javascript does not support map<int..>
  bool encoding_map_key_;
  DISALLOW_EVIL_CONSTRUCTORS(JsonEncoder);
};
}
#endif  //  __NET_RPC_LIB_CODEC_JSON_RPC_JSON_CODEC_H__
