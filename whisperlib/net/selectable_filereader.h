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

#ifndef __NET_BASE_SELECTABLE_FILEREADER_H__
#define __NET_BASE_SELECTABLE_FILEREADER_H__

#include <whisperlib/base/types.h>
#include <whisperlib/base/callback.h>
#include <whisperlib/io/buffer/memory_stream.h>
#include <whisperlib/net/selectable.h>

namespace net {

class SelectableFilereader : public Selectable {
 public:
  typedef Callback1<io::MemoryStream*> DataCallback;

  SelectableFilereader(Selector* selector,
                       int block_size = io::DataBlock::kDefaultBufferSize);
  virtual ~SelectableFilereader();

  bool InitializeFd(int fd,
                    DataCallback* data_callback,
                    Closure* close_callback);

  virtual bool HandleReadEvent(const SelectorEventData& event);
  virtual bool HandleWriteEvent(const SelectorEventData& event);
  virtual bool HandleErrorEvent(const SelectorEventData& event);

  virtual int GetFd() const { return fd_; }
  virtual void Close();

  // Enables or disables reading from the underneath file descriptor
  void EnableRead(bool enable);

  io::MemoryStream* inbuf() {
    return &inbuf_;
  }

 private:
  int fd_;
  io::MemoryStream inbuf_;
  DataCallback* data_callback_;
  Closure* close_callback_;
};
}

#endif  // __NET_BASE_SELECTABLE_FILEREADER_H__
