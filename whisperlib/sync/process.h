// Copyright (c) 2013, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache

#ifndef __COMMON_SYNC_PROCESS2_H__
#define __COMMON_SYNC_PROCESS2_H__

#include <string>
#include <vector>

#include "whisperlib/base/types.h"
#include "whisperlib/base/callback.h"
// #include <whisperlib/base/common.h>
#include "whisperlib/sync/thread.h"
#include "whisperlib/sync/event.h"
#include "whisperlib/net/selector.h"

// A smarter process.
// - The child's stdout & stderr are piped back to the parent, and message
//    lines are transmited to the application through asynchronous callbacks.
// - The child's stdin although not used now, it could be implemented
//    to transmit messages to child.
// - easy synchronization: If you provide a selector, than all callbacks are
//    executed in selector context.
//    If the selector is NULL then callbacks are executed on a separate thread.
//
// PROBLEM: with stdout buffering
// - the C runtime detects if stdout is a terminal. If it's a terminal then it
//    enables line-buffer, otherwise (pipe,file,socket) enables block-buffer.
//    The call to execve() reinitializes the C runtime for the child,
//    so whatever buffer settings made before execve() are useless.
// - fork() creates a child with no terminal, so it's stdout is block-buffered
//    meaning the parent receives no msgs until the buf is full or child exits
// - solution: use forkpty() to create a pseudo-terminal which makes the stdout
//    line-buffered
// - workaround: use fflush() calls in child process to force stdout output

namespace whisper {
namespace process {
class Process {
 private:
  static const pid_t kInvalidPid;
 public:
  // exe: path to the executable
  // argv: program arguments. arg[0] should be your first argument,
  //       and NOT the program name.
  // envp: can be NULL. Specifies additional environment variables
  //       (additional to environ)
  Process(const std::string& exe, const std::vector<std::string>& argv,
          const vector<std::string>* envp = NULL);
  virtual ~Process();

  const std::string& path() const { return exe_; }

  // Returns success status. On failure, call GetLastSystemErrorDescription().
  // stdout_reader: can be NULL. Must be a permanent callback.
  //                Called with lines from child's stdout.
  // auto_delete_stdout_reader: stdout_reader closure is deleted on termination
  // stderr_reader: can be NULL. Must be a permanent callback.
  //                Called with lines from child's stderr.
  // auto_delete_stderr_reader: stderr_reader closure is deleted on termination
  // exit_callback: can be NULL. May be temporary / permanent.
  //                Called just once upon termination with child's exit code.
  // selector: can be NULL. If not null then all callbacks are run in selector.
  //           Otherwise, callbacks are run on a separate random thread.
  bool Start(Callback1<const std::string*>* stdout_reader,
             bool auto_delete_stdout_reader,
             Callback1<const std::string*>* stderr_reader,
             bool auto_delete_stderr_reader,
             Callback1<int>* exit_callback,
             net::Selector* selector);

  // Sends a signal to the running process.
  // Returns success status. On failure, call GetLastSystemError().
  bool Signal(int signum);

  // Kills the running process.
  // Does nothing if the process is not running.
  void Kill();

  // Detaches from the executing process. So you can delete this
  // object and the process keeps running.
  void Detach();

  //  Wait for the running process to terminate.
  // Returns:
  //  true: the process terminated.
  //        exit_status (if not null) contains the process exit status.
  //  false: timeout or the process was not started. Call GetLastSystemError().
  bool Wait(uint32 timeout_ms, int* exit_status = NULL);

  bool IsRunning() const;

  std::string ToString() const;

 private:
  void Runner();
  void RunExitCallback(int exit_code);
  void RunStdoutReader(const std::string* line);
  void RunStderrReader(const std::string* line);
  void RunReaderCallback(Callback1<const std::string*>* reader,
                         const std::string* line);
 private:
  const std::string exe_;
  const std::vector<std::string> argv_;
  std::vector<std::string> envp_;

  pid_t pid_;
  int exit_status_;

  thread::Thread* runner_;
  synch::Event runner_start_;
  bool runner_error_;

  Callback1<const std::string*>* stdout_reader_;
  bool auto_delete_stdout_reader_;
  Callback1<const std::string*>* stderr_reader_;
  bool auto_delete_stderr_reader_;
  Callback1<int>* exit_callback_;
  net::Selector* selector_;
};
}  // namespace process
}  // namespace whisper

#endif  //  __COMMON_SYNC_PROCESS2_H__
