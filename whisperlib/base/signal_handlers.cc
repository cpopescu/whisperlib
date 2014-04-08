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

#include <string.h>
#include <unistd.h>
#include <signal.h>
#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#ifdef HAVE_GOOGLE_PERFTOOLS

#include <google/heap-profiler.h>
#endif
#include <whisperlib/base/log.h>
#include <whisperlib/base/errno.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/date.h>
#include <whisperlib/base/callback.h>

namespace common {

bool g_hang_on_signal_stack_trace = false;
// true if the application is already hanging
bool g_application_is_hanging = false;

#ifndef NACL
void HandleSignal(int signum, siginfo_t* info, void* context) {
  // [COSMIN] Using LOG functions here is unwise. If the initial exception
  //          happened inside a LOG statement, then using LOG here would
  //          recursively generate a new exception.
  cerr << "\033[31m On [" << timer::Date() << " ("
       << timer::Date(true) << ")]\n"
       << " Signal cought " << signum
       << " - " << strsignal(signum) << "\033[0m\n";
  cerr.flush();

  if ( g_hang_on_signal_stack_trace ) {
    g_application_is_hanging = true;
    while ( true ) {
      cerr << "Program pid=" << getpid() << " tid="
#if defined(MACOSX) || defined(IOS)
           << pthread_self()
#else
           << static_cast<uint32>(pthread_self())
#endif
           << " is hanging now. You can Debug it or Kill (Ctrl+C) it."
           << endl;
      ::sleep(30);
    }
  }

  // if you want to continue, do not call default handler
  if ( signum == SIGUSR1 ) {
#if defined(HAVE_GOOGLE_PERFTOOLS) && defined(USE_GOOGLE_PERFTOOLS)
    HeapProfilerDump("on user command");
#endif
    return;
  }

  // call default handler
  ::signal(signum, SIG_DFL);
  ::raise(signum);
}
#endif

bool InstallDefaultSignalHandlers(bool hang_on_bad_signals) {
#ifndef NACL
  g_hang_on_signal_stack_trace = hang_on_bad_signals;

  // install signal handler routines
  struct sigaction sa;
  sa.sa_handler = NULL;
  sa.sa_sigaction = HandleSignal;
  sigemptyset(&sa.sa_mask);
  //  Restart functions if interrupted by handler
  //  We want to process RT signals..
  sa.sa_flags = SA_RESTART | SA_SIGINFO;

  if ( -1 == ::sigaction(SIGABRT, &sa, NULL) ||   // assert failed
       -1 == ::sigaction(SIGBUS, &sa, NULL) ||
       -1 == ::sigaction(SIGHUP, &sa, NULL) ||
       -1 == ::sigaction(SIGFPE, &sa, NULL) ||
       -1 == ::sigaction(SIGILL, &sa, NULL) ||
       -1 == ::sigaction(SIGUSR1, &sa, NULL) ||
       -1 == ::sigaction(SIGSEGV, &sa, NULL) ) {
    LOG_ERROR << "cannot install SIGNAL-handler: "
              << GetLastSystemErrorDescription();
    return false;
  }
  // TODO(cpopescu): disable this for now
  //     CHECK(!::sigaction(SIGRTMIN, &sa, NULL));

  // TODO(cosmin): ignore SIGPIPE.
  //               Writing to a disconnected socket causes SIGPIPE.
  //               All system calls that would cause SIGPIPE to be sent will
  //               return -1 and set errno to EPIPE.
  if ( SIG_ERR == ::signal(SIGPIPE, SIG_IGN) ) {
    LOG_ERROR << "cannot install SIGNAL-handler: "
              << GetLastSystemErrorDescription();
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool IsApplicationHanging() {
  return g_application_is_hanging;
}
void SetApplicationHanging(bool hanging) {
  g_application_is_hanging = hanging;
}
}
