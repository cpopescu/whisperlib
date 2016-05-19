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


// This is just a simple test .. nothing special ...

#include "whisperlib/base/log.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/base/system.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/sync/thread.h"
#include "whisperlib/sync/producer_consumer_queue.h"

DEFINE_int32(num_elements, 100, "Run a test on these many elements");

using namespace whisper;

void ThreadFun(int num,
               synch::ProducerConsumerQueue<int>* p1,
               synch::ProducerConsumerQueue<int>* p2) {
  for ( int i = 0; i < num; i++ ) {
    p1->Put(i);
    CHECK_EQ(p2->Get(), i);
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  synch::ProducerConsumerQueue<int> p1(5);
  synch::ProducerConsumerQueue<int> p2(5);
  thread::Thread t1(NewCallback(&ThreadFun, FLAGS_num_elements, &p1, &p2));
  thread::Thread t2(NewCallback(&ThreadFun, FLAGS_num_elements, &p2, &p1));
  t1.SetJoinable();
  t2.SetJoinable();
  t1.Start();
  t2.Start();
  t1.Join();
  t2.Join();
  LOG_INFO << " PASS !";
}
