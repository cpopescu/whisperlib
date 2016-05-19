// Copyright (c) 2013, Whispersoft s.r.l.
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
// * Neither the name of Whispersoft s.r.l nor the names of its
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
//


#include "whisperlib/sync/process.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "whisperlib/base/core_errno.h"
#include "whisperlib/base/system.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/base/scoped_ptr.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/io/buffer/memory_stream.h"

// #declared in unistd.h
// Points to an array of strings called the 'environment'.
// By convention these strings have the form 'name=value'.

// """
// is initialized as a pointer to an array of character pointers to
// the environment strings. The argv and environ arrays are each
// terminated by a null pointer. The null pointer terminating  the
// argv array is not counted in argc.
// """

#ifdef MACOSX
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char** environ;
#endif
#include <stdarg.h>

namespace whisper {
namespace process {

namespace {
void MakeStringVector(const char* const argv[], std::vector<std::string>* out) {
  if ( argv ) {
    for ( int i = 0; argv[i] != NULL; i++ ) {
      out->push_back(std::string(argv[i]));
    }
  }
}
// The result is dynamically allocated, but the elements are referenced;
// just call: delete[] result;
char** MakeCharArray(const char* first, const std::vector<std::string>& v) {
  char** out = new char*[v.size() + 2];
  uint32 out_index = 0;
  if ( first != NULL ) {
    out[out_index++] = const_cast<char*>(first);
  }
  for ( uint32 i = 0; i < v.size(); i++ ) {
    out[out_index++] = const_cast<char*>(v[i].c_str());
  }
  out[out_index] = NULL;
  return out;
}
template <typename T>
void RunCallback(Callback1<T>* callback, T result) {
  callback->Run(result);
}
template <typename T>
void RunPtrCallback(Callback1<T>* callback, T result) {
  callback->Run(result);
  delete result;
}
}

const pid_t Process::kInvalidPid = pid_t(-1);

Process::Process(const std::string& exe, const std::vector<std::string>& argv,
                 const std::vector<std::string>* envp)
  : exe_(exe),
    argv_(argv),
    pid_(kInvalidPid),
    exit_status_(0),
    runner_(NULL),
    runner_start_(false, true),
    runner_error_(false),
    stdout_reader_(NULL),
    auto_delete_stdout_reader_(false),
    stderr_reader_(NULL),
    auto_delete_stderr_reader_(false),
    exit_callback_(NULL) {
  MakeStringVector(environ, &envp_);
  if ( envp != NULL ) {
    std::copy(envp->begin(), envp->end(), std::back_inserter(envp_));
  }
}
Process::~Process() {
  Kill();
  delete runner_;
  runner_ = NULL;
  if ( auto_delete_stdout_reader_ ) {
    delete stdout_reader_;
    stdout_reader_ = NULL;
  }
  if ( auto_delete_stderr_reader_ ) {
    delete stderr_reader_;
    stderr_reader_ = NULL;
  }
}

std::string Process::ToString() const {
    return exe_ + " " + strutil::JoinStrings(argv_, " ");
}


bool Process::Start(Callback1<const std::string*>* stdout_reader,
                    bool auto_delete_stdout_reader,
                    Callback1<const std::string*>* stderr_reader,
                    bool auto_delete_stderr_reader,
                    Callback1<int>* exit_callback,
                    net::Selector* selector) {
  CHECK_NULL(runner_) << " Duplicate Start()";

  stdout_reader_ = stdout_reader;
  if ( stdout_reader_ != NULL ) {
    CHECK(stdout_reader_->is_permanent());
    auto_delete_stdout_reader_ = auto_delete_stdout_reader;
  }

  stderr_reader_ = stderr_reader;
  if ( stderr_reader_ != NULL ) {
    CHECK(stderr_reader_->is_permanent());
    auto_delete_stderr_reader_ = auto_delete_stderr_reader;
  }

  exit_callback_ = exit_callback;
  selector_ = selector;

  runner_ = new thread::Thread(NewCallback(this, &Process::Runner));
  runner_->Start();
  if ( !runner_start_.Wait(5000) ) {
    LOG_ERROR << "Runner() thread failed to start";
    return false;
  }
  return !runner_error_;
}

bool Process::Signal(int signum) {
  LOG_ERROR << "TODO(cosmin): implement";
  return false;
}

// Kills the process.
void Process::Kill() {
  if ( !IsRunning() ) { return; }
  if ( ::kill(-pid_, SIGKILL) != 0 ) {
    LOG_ERROR << "::kill(" << pid_ << ", SIGKILL) failed"
              << ", error=" << GetLastSystemErrorDescription();
  } else {
    LOG_WARNING << "Process [pid=" << pid_ << "] killed.";
  }
  pid_ = kInvalidPid;
}

// Detaches from the executing process. So you can delete this
// object and the process keeps running.
void Process::Detach() {
  LOG_ERROR << "TODO(cosmin): implement";
}

bool Process::Wait(uint32 timeout_ms, int* exit_status) {
  if ( !IsRunning() ) {
    return false;
  }
  CHECK_NOT_NULL(runner_);
  runner_->Join();
  if ( exit_status != NULL ) {
    *exit_status = exit_status_;
  }
  return true;
}

bool Process::IsRunning() const {
  return pid_ != kInvalidPid;
}

namespace {
struct Pipe {
  Pipe() : read_fd_(INVALID_FD_VALUE), write_fd_(INVALID_FD_VALUE) {}
  ~Pipe() { Close(); }
  bool Open() {
    int p[2] = {0,};
    if ( 0 != ::pipe(p) ) {
      LOG_ERROR << "::pipe() failed: " << GetLastSystemErrorDescription();
      return false;
    }
    read_fd_ = p[0];
    write_fd_ = p[1];
    return true;
  }
  void Close() {
    CloseRead();
    CloseWrite();
  }
  void CloseRead() {
    if ( read_fd_ != INVALID_FD_VALUE ) {
      if ( -1 == ::close(read_fd_) ) {
        LOG_ERROR << "::close failed: " << GetLastSystemErrorDescription();
      }
      read_fd_ = INVALID_FD_VALUE;
    }
  }
  void CloseWrite() {
    if ( write_fd_ != INVALID_FD_VALUE ) {
      if ( -1 == ::close(write_fd_) ) {
        LOG_ERROR << "::close failed: " << GetLastSystemErrorDescription();
      }
      write_fd_ = INVALID_FD_VALUE;
    }
  }
  bool IsReadOpen() const { return read_fd_ != INVALID_FD_VALUE; }
  bool IsWriteOpen() const { return write_fd_ != INVALID_FD_VALUE; }
  bool IsOpen() const { return IsReadOpen() || IsWriteOpen(); }
  int read_fd_;
  int write_fd_;
} __attribute__((__packed__));
}

void Process::Runner() {
  // Create simple pipes for stdout & stderr.
  // The child is going to write to them, the parent is going to read them.
  Pipe stdout_pipe, stderr_pipe;
  if ( (stdout_reader_ != NULL && !stdout_pipe.Open()) ||
       (stderr_reader_ != NULL && !stderr_pipe.Open()) ) {
    LOG_ERROR << " Cannot create process pipes.";
    runner_error_ = true;
    runner_start_.Signal();
    return;
  }

  LOG_INFO << "Executing process: " << exe_
           << ", argv: " << strutil::ToString(argv_);

  pid_ = ::fork();
  if ( pid_ == 0 ) {
    ////////////////////////////////
    // child process
    ::setsid(); // set us as a process group leader
                // (kill us -> kill all under us)
    // close pipes read end
    stdout_pipe.CloseRead();
    stderr_pipe.CloseRead();
    // redirect stdout & stderr to pipe's write end
    if ( (stdout_pipe.IsWriteOpen() && -1 == ::dup2(stdout_pipe.write_fd_, 1)) ||
         (stderr_pipe.IsWriteOpen() && -1 == ::dup2(stderr_pipe.write_fd_, 2)) ) {
      LOG_ERROR << "::dup2 failed: " << GetLastSystemErrorDescription();
      common::Exit(1);
    }

    // execute the external program
    scoped_array<char*> argv(MakeCharArray(exe_.c_str(), argv_));
    scoped_array<char*> envp(MakeCharArray(NULL, envp_));
    int result = ::execve(exe_.c_str(), argv.get(), envp.get());
    LOG_ERROR << "::execve(), exe: [" << exe_ << "]"
                 ", failed: " << GetLastSystemErrorDescription();
    _exit(result);
  }
  ////////////////////////////////
  // parent process
  runner_error_ = false;
  runner_start_.Signal();

  // close pipes write end
  stdout_pipe.CloseWrite();
  stderr_pipe.CloseWrite();

  // read pipes until EOF
  io::MemoryStream stdout_ms, stderr_ms;
  Pipe* pipes[] = { &stdout_pipe, &stderr_pipe };
  io::MemoryStream* ouput[] = { &stdout_ms, &stderr_ms };

  while ( true ) {
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set input;
    FD_ZERO(&input);
    int maxfd = 0;
    for (size_t npipe = 0; npipe < NUMBEROF(pipes); ++npipe) {
        Pipe* p = pipes[npipe];
        if (p && p->IsOpen()) {
            FD_SET(p->read_fd_, &input);
            maxfd = std::max(maxfd, p->read_fd_);
        }
    }
    if ( maxfd == 0 ) {
      // both pipes are closed
      break;
    }

    const int n = ::select(1 + maxfd, &input, NULL, NULL, &timeout);
    if ( n < 0 ) {
      LOG_ERROR << "::select failed: " << GetLastSystemErrorDescription();
      Kill();
      RunExitCallback(1);
      return;
    }
    if ( n == 0 ) {
      // timeout
      continue;
    }
    char read_buf[256];
    for (size_t npipe = 0; npipe < NUMBEROF(pipes); ++npipe) {
        Pipe* p = pipes[npipe];
        if (!p) continue;
        if ( p->IsReadOpen() && FD_ISSET(p->read_fd_, &input) ) {
            const ssize_t size = ::read(p->read_fd_, read_buf, sizeof(read_buf));
            if ( size == -1 ) {
                LOG_ERROR << "::read failed: " << GetLastSystemErrorDescription();
                Kill();
                RunExitCallback(1);
                return;
            }
            if ( size == 0 ) {
                // EOF
                p->Close();
                pipes[npipe] = NULL;
            }
            ouput[npipe]->Write(read_buf, size);
        }
    }
    // report lines
    std::string line;
    while ( stdout_ms.ReadLine(&line) ) { RunStdoutReader(&line); }
    while ( stderr_ms.ReadLine(&line) ) { RunStderrReader(&line); }
  }

  // wait for child to terminate (since we received EOF on both pipes,
  //  it should have already ended)
  int status = 0;
  const pid_t result = ::waitpid(pid_, &status, 0);
  if ( result == -1 ) {
    LOG_ERROR << "::waitpid [pid=" << pid_ << "] failed: "
              << GetLastSystemErrorDescription();
    Kill();
    RunExitCallback(1);
    return;
  }
  CHECK_EQ(result, pid_);
  exit_status_ = WEXITSTATUS(status);
  LOG_WARNING << "Process [pid=" << pid_ << "] terminated: exe_='"
              << exe_ << "', exit_status_=" << exit_status_;
  pid_ = kInvalidPid;

  RunExitCallback(exit_status_);
}
void Process::RunExitCallback(int exit_code) {
  if ( exit_callback_ ) {
      if ( selector_ != NULL ) {
          selector_->RunInSelectLoop(
              NewCallback(&RunCallback, exit_callback_, exit_code));
      } else {
          exit_callback_->Run(exit_code);
      }
  }
}

void Process::RunStdoutReader(const std::string* line) {
  if ( stdout_reader_ ) RunReaderCallback(stderr_reader_, line);
}
void Process::RunStderrReader(const std::string* line) {
  if ( stderr_reader_ ) RunReaderCallback(stderr_reader_, line);
}

void Process::RunReaderCallback(Callback1<const std::string*>* reader,
                                const std::string* line) {
    if ( selector_ ) {
        // Need to allocate a new temp string - this may be gone / changed
        // by the time of selector run
        selector_->RunInSelectLoop(
            whisper::NewCallback(&RunPtrCallback, reader,
                          static_cast<const std::string*>(
                              new std::string(*line))));
    } else {
        reader->Run(line);
    }
}

}  // namespace process
}  // namespace whisper
