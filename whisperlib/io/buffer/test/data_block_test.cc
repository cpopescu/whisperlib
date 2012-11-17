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
// Author: Catalin Popescu
//

#include <stdio.h>
#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/timer.h>
#include <whisperlib/base/system.h>

#include <whisperlib/io/buffer/data_block.h>

int main(int argc, char* argv[]) {
  common::Init(argc, argv);


#ifdef __USE_VARIANT_MEMORY_STREAM_IMPLEMENTATION__
  io::DataBlock* scratch = new io::DataBlock(1024);
  io::BlockDqueue q(scratch);
  q.pop_back();
#else
  io::BlockDqueue q;
#endif

  q.push_back(new io::DataBlock(1024));
  q.push_back(new io::DataBlock(2048));
  q.push_back(new io::DataBlock(4096));
  q.push_back(new io::DataBlock(9192));
#ifdef __USE_VARIANT_MEMORY_STREAM_IMPLEMENTATION__
  q.push_back(scratch);
#endif

  io::DataBlockPointer p0(&q, q.begin_id(), 0);
  io::DataBlockPointer p1(&q, q.begin_id(), 0);
  io::DataBlockPointer p2(&q, q.begin_id() + 1, 0);
  io::DataBlockPointer p3(&q, q.begin_id() + 2, 0);
  io::DataBlockPointer p4(&q, q.begin_id() + 3, 0);
#ifdef __USE_VARIANT_MEMORY_STREAM_IMPLEMENTATION__
  io::DataBlockPointer p5(&q, q.begin_id() + 4, 0);
#endif

  CHECK(p0 == p1) << " - " << p0 << " vs. " << p1;
  CHECK(p2 > p1);
  CHECK(p3 > p1);
  CHECK(p1 < p4);
  CHECK(p3 > p2);
  CHECK_EQ(p0.ReadableSize(), 0);
  CHECK_EQ(p1.ReadableSize(), 0);
  CHECK_EQ(p2.ReadableSize(), 0);

  CHECK_EQ(p0.Distance(p0), 0);
  CHECK_EQ(p0.Distance(p1), 0);
  CHECK_EQ(p1.Distance(p2), 0);
  CHECK_EQ(p1.Distance(p3), 0);
  CHECK_EQ(p3.Distance(p4), 0);

  // Advance / devance on empty stuff
  CHECK_EQ(p0.Advance(1), 0);
#ifdef __USE_VARIANT_MEMORY_STREAM_IMPLEMENTATION__
  CHECK(p5 == p0);
#else
  CHECK(p4 == p0);
#endif
  CHECK_EQ(p0.Advance(-1), 0);
  CHECK(p1 == p0);
  CHECK_EQ(p0.Devance(-1), 0);
#ifdef __USE_VARIANT_MEMORY_STREAM_IMPLEMENTATION__
  CHECK(p5 == p0);
#else
  CHECK(p4 == p0);
#endif
  CHECK_EQ(p0.Devance(1), 0);
  CHECK(p1 == p0);

  // Write stuff
  char buffer[16384];
  for ( int32 i = 0; i < NUMBEROF(buffer); ++i ) {
    buffer[i] = i % 256;
  }
  CHECK_EQ(p0.WriteData(buffer, 512), 512);
  CHECK_EQ(p0.Distance(p1), 512);
  CHECK_EQ(p0.Distance(p2), 0);

  CHECK_EQ(p0.WriteData(buffer, 1024), 1024);
  CHECK_EQ(p0.Distance(p1), 1024 + 512);
  CHECK_EQ(p0.Distance(p2), 512);
  CHECK_EQ(p0.Distance(p3), 0);

  CHECK_EQ(p0.WriteData(buffer, 2048), 2048);
  CHECK_EQ(p0.Distance(p1), 2048 + 1024 + 512);
  CHECK_EQ(p0.Distance(p2), 2048 + 512);
  CHECK_EQ(p0.Distance(p3), 512);
  CHECK_EQ(p0.Distance(p4), 0);

  // Start reading ...
  CHECK_EQ(p1.ReadData(buffer, 2048), 2048);
  for ( int i = 0; i < 2048; ++i ) {
    CHECK_EQ(static_cast<uint8>(buffer[i]),
             static_cast<int>(i % 256));
  }
  CHECK_EQ(p1.Distance(p2), 1024);
  CHECK_EQ(p1.Distance(p3), 1024);

  CHECK_EQ(p1.ReadData(buffer, 512),  512);
  for ( int i = 0; i < 512; ++i ) {
    CHECK_EQ(static_cast<uint8>(buffer[i]),
             static_cast<int>(i % 256));
  }
  CHECK_EQ(p1.Distance(p2), 1024 + 512);
  CHECK_EQ(p1.Distance(p3), 512);

  CHECK_EQ(p1.ReadData(buffer, 333), 333);
  CHECK_EQ(p1.Distance(p2), 1024 + 512 + 333);
  for ( int i = 0; i < 333; ++i ) {
    CHECK_EQ(static_cast<uint8>(buffer[i]),
             static_cast<int>(i % 256));
  }

  CHECK_EQ(p1.ReadData(buffer, 2048 - 333), 1024 - 333);
  for ( int i = 0; i < 333; ++i ) {
    CHECK_EQ(static_cast<uint8>(buffer[i]),
             static_cast<int>((333 + i) % 256));
  }
  CHECK(p0 == p1);
  CHECK_EQ(p1.Distance(p2), 2048 + 512);
  CHECK_EQ(p1.Distance(p3), 512);
  printf("PASS\n");
  common::Exit(0);
}
