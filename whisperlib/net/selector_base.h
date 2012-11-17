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
// Authors: Cosmin Tudorache & Catalin Popescu
//
// Base for Selector - we have different implementations if we use
// poll - most portable, epoll - great for linux, kevents - great for bsd
//

#ifndef __NET_BASE_SELECTOR_BASE_H__
#define __NET_BASE_SELECTOR_BASE_H__

#include <vector>
#include <whisperlib/base/types.h>

//////////////////////////////////////////////////////////////////////
//
// EPOLL version
//
#if defined(HAVE_SYS_EPOLL_H)

#include <sys/epoll.h>

// We need this definde to a safe value anyway
#ifndef EPOLLRDHUP
// #define EPOLLRDHUP 0x2000
#define EPOLLRDHUP 0
#endif

#elif defined(HAVE_POLL_H) || defined(HAVE_SYS_POLL_H)

#include WHISPER_HASH_MAP_HEADER

#ifdef HAVE_POLL_H
#include "poll.h"
#else
#include <sys/poll.h>
#endif

// We need this definde to a safe value anyway
#ifndef POLLRDHUP
// #define POLLRDHUP 0x2000
#define POLLRDHUP 0
#endif

#else

#error "Need poll or epoll available"

#endif

//////////////////////////////////////////////////////////////////////

// COMMON part

namespace net {

class Selectable;

struct SelectorEventData {
  void* data_;
  int32 desires_;
  int internal_event_;
  SelectorEventData(void* data,
                    int32 desires,
                    int internal_event)
      : data_(data),
        desires_(desires),
        internal_event_(internal_event) {
  }
};

class SelectorBase {
 public:

  SelectorBase(int pipe_fd, int max_events_per_step);
  ~SelectorBase();

  //  add a file descriptor in the epoll, and link to the given data pointer.
  bool Add(int fd, void* user_data, int desires);
  //  updates the desires for the file descriptor in the epoll.
  bool Update(int fd, void* user_data, int desires);
  //  remove a file descriptor from the epoll
  bool Delete(int fd);

  // Run a selector loop step. It fills in events two things:
  //  -- the user data associted with the fd that was triggered
  //  -- the event that happended (an or of Selector desires)
  bool LoopStep(int32 timeout_in_ms,
                vector<SelectorEventData>*  events);

 private:
  const int max_events_per_step_;

  //////////////////////////////////////////////////////////////////////
  //
  // EPOLL version
  //

#if defined(HAVE_SYS_EPOLL_H)
  // Converts a Selector desire in some epoll flags.
  int DesiresToEpollEvents(int32 desires);

  // epoll file descriptor
  const int epfd_;

  // here we get events that we poll
  struct epoll_event* const events_;

#else

  //////////////////////////////////////////////////////////////////////
  //
  // POLL version
  //

  // Converts a Selector desire in some poll flags.
  int DesiresToPollEvents(int32 desires);
  // Compacts the fds table at the end of a loop step
  void Compact();

  static const int kMaxFds = 4096;
  // epoll file descriptor
  struct pollfd fds_[kMaxFds];
  // how many in fds are used
  size_t fds_size_;
  // maps from fd to index in fds_ and user data
  typedef hash_map< int, pair<size_t, void*> > DataMap;
  DataMap fd_data_;
  // indices that we need to compact at the end of the step
  vector<size_t> indices_to_compact_;
#endif

  //////////////////////////////////////////////////////////////////////

  DISALLOW_EVIL_CONSTRUCTORS(SelectorBase);
};

}

#endif  // __NET_BASE_SELECTOR_BASE_H__
