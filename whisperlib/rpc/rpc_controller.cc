// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
//
// (c) Copyright 2011, 1618labs
// All rights reserved.
// Author: Catalin Popescu (cp@1618labs.com)
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
  timeout_ms_ = 0;
  is_urgent_ = false;
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
