// Copyright (c) 2012, 1618labs
// All rights reserved.
//
// Author: Catalin Popescu
//

#include <stdio.h>
#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/timer.h>
#include <whisperlib/base/system.h>

#include <whisperlib/io/buffer/protobuf_stream.h>
#include <whisperlib/io/buffer/test/test_proto.pb.h>

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  io::TestData data;
  data.set_i(12345);
  data.set_s("ali baba");

  io::MemoryStream ms;
  CHECK(io::SerializeProto(&data, &ms));
  LOG_INFO << " Serialized data size: " << ms.Size();
  io::TestData proof;
  CHECK(io::ParseProto(&proof, &ms));
  CHECK_EQ(ms.Size(), 0);
  CHECK_EQ(proof.i(), 12345);
  CHECK_EQ(proof.s(), "ali baba");

  data.clear_i();
  CHECK(io::SerializePartialProto(&data, &ms));
  LOG_INFO << " Serialized data size: " << ms.Size();
  proof.Clear();
  CHECK(io::ParsePartialProto(&proof, &ms));
  CHECK_EQ(ms.Size(), 0);
  CHECK(!proof.has_i());
  CHECK_EQ(proof.s(), "ali baba");


  LOG_INFO << "PASS";
}
