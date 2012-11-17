// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
//
// (c) Copyright 2012, 1618labs
// All rights reserved.
// Author: Catalin Popescu (cp@1618labs.com)
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
// * Neither the name of 1618labs nor the names of its
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

#include <whisperlib/base/strutil.h>
#include <whisperlib/rpc/rpc_controller.h>

namespace rpc {

const char* GetErrorCodeString(rpc::ErrorCode error) {
  switch (error) {
    CONSIDER(ERROR_NONE);
    CONSIDER(ERROR_CANCELLED);
    CONSIDER(ERROR_USER);
    CONSIDER(ERROR_CLIENT);
    CONSIDER(ERROR_NETWORK);
    CONSIDER(ERROR_SERVER);
    CONSIDER(ERROR_PARSE);
  }
  return "=UNKNOWN=";
}

Controller::Controller()
  : google::protobuf::RpcController(),
    transport_(NULL) {
  Reset();
}

Controller::Controller(Transport* transport)
  : google::protobuf::RpcController(),
    transport_(NULL) {
  Reset();
  transport_ = transport;
}

Controller::~Controller() {
  CHECK(cancel_callback_ == NULL);
  delete transport_;
}

void Controller::Reset() {
  error_code_ = rpc::ERROR_NONE;
  error_reason_.clear();
  cancel_callback_ = NULL;
  if (transport_ != NULL) {
    delete transport_;
  }
  transport_ = NULL;
}

bool Controller::Failed() const {
  synch::MutexLocker l(&mutex_);
  return error_code_ != rpc::ERROR_NONE;
}

string Controller::ErrorText() const {
  synch::MutexLocker l(&mutex_);
  return strutil::StringPrintf("%d %s [%s]", error_code_,
                               rpc::GetErrorCodeString(error_code_),
                               error_reason_.c_str());
}

void Controller::StartCancel() {
  CallCancelCallback(true);
}

void Controller::SetFailed(const string& reason) {
  synch::MutexLocker l(&mutex_);
  error_code_ = rpc::ERROR_USER;
  error_reason_ = reason;
}

rpc::ErrorCode Controller::GetErrorCode() {
  synch::MutexLocker l(&mutex_);
  return error_code_;
}

void Controller::SetErrorCode(rpc::ErrorCode code) {
  CHECK_NE(code, rpc::ERROR_NONE);
  CHECK_NE(code, rpc::ERROR_CANCELLED);

  synch::MutexLocker l(&mutex_);
  error_code_ = code;
}

bool Controller::IsCanceled() const {
  synch::MutexLocker l(&mutex_);
  return error_code_ == ERROR_CANCELLED;
}

void Controller::NotifyOnCancel(google::protobuf::Closure* callback) {
  bool call_now = false;
  mutex_.Lock();
  if (error_code_ == ERROR_CANCELLED) {
    call_now = true;
    cancel_callback_ = NULL;
  } else {
    cancel_callback_ = callback;
  }
  mutex_.Unlock();
  if (call_now && callback != NULL) {
    callback->Run();
  }
}

void Controller::CallCancelCallback(bool set_cancel) {
  google::protobuf::Closure* to_call;
  mutex_.Lock();
  to_call = cancel_callback_;
  if (set_cancel) {
    error_code_ = rpc::ERROR_CANCELLED;
  }
  cancel_callback_ = NULL;
  mutex_.Unlock();
  if (to_call != NULL) {
    to_call->Run();
  }
}
}
