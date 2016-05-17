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

#ifndef __COMMON_APP_APP_H__
#define __COMMON_APP_APP_H__

#include <vector>
#include <string>
#include "whisperlib/base/types.h"
#include "whisperlib/sync/thread.h"
#include "whisperlib/sync/event.h"

namespace whisper {
namespace app {

class App {
 public:
  App(int& argc, char**& argv);
  virtual ~App();

  // The main entry point, should be called ONLY from the main() function.
  int Main();

  int argc() const { return argc_; }
  const char* const* argv() const { return argv_; }

 protected:
  // This is called from the main thread, before the application is run,
  // if not 0, the result will be returned from the main() function.
  virtual int Initialize() = 0;

  // This is called from the secondary thread, this should run the actual
  // application logic.
  // If this function returns before a termination signal (Ctrl+c) appears,
  //  we call Stop(), then terminate the application.
  // If a termination signal (Ctrl+c) appears, we call Stop() first then wait
  //  for Run() to terminate, then terminate application.
  // Stop() is always called from main thread, while Run() executes in
  //  secondary thread.
  virtual void Run() = 0;
  // This is called from the main thread when a stop/terminate request
  // is received. After this, the Run() function should terminate as soon
  // as possible.
  virtual void Stop() = 0;

  // This is called from the main thread just before the application terminates.
  virtual void Shutdown() = 0;

 private:
  void SignalSetup();
  void SignalRestore();

  static void SignalHandler(int signal);

  void RunInternal();

 protected:
   // The input parameters.
   int argc_;
   char** argv_;

   // The run result.
   int result_;

   // The secondary thread object - which runs the application itself.
   whisper::thread::Thread thread_;

   // The executable name.
   std::string name_;
   // The stop event
   synch::Event stop_event_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(App);
};
}  // namespace app
}  // namespace whisper

#endif  // __COMMON_APP_H__
