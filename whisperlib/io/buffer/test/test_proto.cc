/** Copyright (c) 2012, Urban Engines inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Urban Engines inc nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Catalin Popescu
 */

#include <stdio.h>
#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/base/system.h"

#include "whisperlib/io/buffer/protobuf_stream.h"
#include "whisperlib/io/buffer/test/test_proto.pb.h"

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  whisper::io::TestData data;
  data.set_i(12345);
  data.set_s("ali baba");

  whisper::io::MemoryStream ms;
  CHECK(whisper::io::SerializeProto(&data, &ms));
  const int data_size = ms.Size();
  LOG_INFO << " Serialized data size: " << ms.Size();
  whisper::io::TestData proof;
  CHECK(whisper::io::ParseProto(&proof, &ms));
  CHECK_EQ(ms.Size(), 0);
  CHECK_EQ(proof.i(), 12345);
  CHECK_EQ(proof.s(), "ali baba");

  for (int i = 0; i < 10; ++i) {
      CHECK(whisper::io::SerializeProto(&data, &ms));
  }
  LOG_INFO << " Serialized data size: " << ms.Size();
  for (int i = 0; i < 10; ++i) {
      whisper::io::TestData proof2;
      LOG_INFO << "Parsing from: " << ms.Size() << " / " << data_size;
      CHECK(whisper::io::ParseProto(&proof2, &ms, data_size));
      CHECK_EQ(proof2.i(), 12345);
  }
  CHECK_EQ(ms.Size(), 0);


  data.clear_i();
  CHECK(whisper::io::SerializePartialProto(&data, &ms));
  LOG_INFO << " Serialized data size: " << ms.Size();
  proof.Clear();
  CHECK(whisper::io::ParsePartialProto(&proof, &ms));
  CHECK_EQ(ms.Size(), 0);
  CHECK(!proof.has_i());
  CHECK_EQ(proof.s(), "ali baba");


  LOG_INFO << "PASS";
}
