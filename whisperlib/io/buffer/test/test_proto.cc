// Copyright (c) 2012, Urban Engines
// All rights reserved.
//
// Author: Catalin Popescu
//

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
