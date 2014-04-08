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

#include <config.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <signal.h>

#include <whisperlib/base/app.h>
#include <whisperlib/base/strutil.h>
#include <whisperlib/base/system.h>
#include <whisperlib/sync/mutex.h>

#include <whisperlib/base/signal_handlers.h>

namespace app {

static App* g_app = NULL;

App::App(int argc, char** argv)
    : argc_(argc),
      result_(0),
      thread_(NewCallback(this, &App::RunInternal)),
      stop_event_(false, true) {
  argv_ = new char*[argc + 1];
  for ( int i = 0; i < argc; ++i ) {
    argv_[i] = strdup(argv[i]);
  }
  argv_[argc] = NULL;

  CHECK_NULL(g_app);
  g_app = this;

  name_ = strutil::Basename(argv[0]);

  common::Init(argc_, argv_);

  SignalSetup();
}

App::~App() {
  SignalRestore();
  for ( int i = 0; i < argc_; ++i ) {
    // free(argv_[i]);  //  -- well .. we leak these - whatever, but
    //                  //     they may got mixed..
  }
  // delete argv_;  // we leak this too..

  CHECK_NOT_NULL(g_app);
  g_app = NULL;
}

int App::Main() {
  result_ = Initialize();
  if ( result_ == 0 ) {
    CHECK(thread_.Start()) << " Couldn't start the main thread.";
    CHECK(thread_.SetJoinable()) << " Couldn't make the main thread joinable.";

    stop_event_.Wait();

    Stop();
    thread_.Join();
  }

  Shutdown();
  return result_;
}

void App::SignalSetup() {
#ifdef NACL
  LOG_ERROR << " Skipping SignalSetup on native client";
#else
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = &App::SignalHandler;
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  // we ignore PIPE signals
  action.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &action, NULL);
#endif
}

void App::SignalRestore() {
#ifdef NACL
  LOG_ERROR << " Skipping SignalRestore on native client";
#else
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_DFL;
  sigaction(SIGPIPE, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGINT, &action, NULL);
#endif
}

void App::SignalHandler(int signal) {
  switch ( signal ) {
    case SIGINT:
      LOG_INFO << "SIGINT caught, terminating " << g_app->name_ << "...";
      break;
    case SIGTERM:
      LOG_INFO << "SIGTERM caught, terminating " << g_app->name_ << "...";
      break;
    default:
      LOG_INFO << "Unexpected signal " << signal
               << " caught, terminating " << g_app->name_ << "...";
      break;
  }

  if (common::IsApplicationHanging()) {
    LOG_INFO << "Forced exit.";
    ::_exit(-1);
  }
  g_app->stop_event_.Signal();
  common::SetApplicationHanging(true);
}
void App::RunInternal() {
  Run();
  stop_event_.Signal();
}
}
