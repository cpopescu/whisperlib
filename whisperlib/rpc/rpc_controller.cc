// -*- Mode:c++; c-basic-offset:2; indent-tabs-mode:nil; coding:utf-8 -*-
// (c) Copyright 2011, Urban Engines.  All rights reserved.
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
// * Neither the name of Urban Engines inc nor the names of its
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
// Author: Catalin Popescu (cp@urbanengines.com)
//

#include <google/protobuf/message.h>

#include "whisperlib/base/gflags.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/rpc/rpc_controller.h"
#include "whisperlib/rpc/RpcStats.pb.h"

DEFINE_string(rpc_statusz_bgcolor, "#FFFFFF",
              "Background color for statusz pages.");

namespace whisper {
namespace rpc {

static const char* kStatuszColLine = "style=\"border-top:1px solid black;\"";

std::string GetStatuszHeadHtml() {
  return strutil::StringPrintf(
    "<head><style>\n"
    "body {\n"
    "  background-color:%s; \n"
    "  font-family: Arial, sans-serif;\n"
    "  font-size: 10pt;\n"
    "}\n"
    "</style></head>",
    FLAGS_rpc_statusz_bgcolor.c_str());
}
std::string MachineStatsToHtml(const pb::MachineStats& stat) {
  std::string s;
  s += "\n<h2>Machine Requests:</h2><table cellpadding=\"3\" width=\"80%%\">\n";
  s += strutil::StringPrintf(
    "\n<tr><td width=\"30%\"><b>Name:</b> %s</td>"
    "\n<td \"70%\"><b>Path:</b> %s</td></tr>",
    strutil::XmlStrEscape(stat.server_name()).c_str(),
    strutil::XmlStrEscape(stat.path()).c_str());
  if (stat.has_auth_realm()) {
    s += strutil::StringPrintf(
      "\n<tr><td bgcolor=\"#f0f0c0\"><b>Auth:</b> %s</td></tr>",
      strutil::XmlStrEscape(stat.auth_realm()).c_str());
  }
  s += strutil::StringPrintf(
    "\n<tr><td %s bgcolor=\"#c0f0c0\"><b>Sys info:</b></td>"
    "\n<td %s bgcolor=\"#c0f0c0\"><font face=\"courier\">%s</font></td></tr>",
    kStatuszColLine, kStatuszColLine,
    strutil::XmlStrEscape(stat.system_status().ShortDebugString()).c_str());
  for (int i = 0; i < stat.disk_status_size(); ++i) {
    s += strutil::StringPrintf(
      "\n<tr><td %s bgcolor=\"#f0f0f8\"><b>Disk [%s]:</b></td>"
      "\n<td %s bgcolor=\"#f0f0f8\"><font face=\"courier\">%s</font></td></tr>",
      kStatuszColLine,
      strutil::XmlStrEscape(stat.disk_status(i).path()).c_str(),
      kStatuszColLine,
      strutil::XmlStrEscape(stat.disk_status(i).ShortDebugString()).c_str());
  }
  s+="</table>";
  return s;
}

std::string RequestStatsToHtml(const pb::RequestStats& stat, int64 now_ts, int64 now_sec) {
  const int64 start_ms = now_sec * 1000 - (now_ts - stat.start_time_ts()) * 1e-6;
  const int64 response_ms = stat.has_response_time_ts()
    ? now_sec * 1000 - (now_ts - stat.response_time_ts()) * 1e-6 : int64(0);
  std::string s = strutil::StringPrintf(
    "\n<tr bgcolor=\"#e8e0f8\">"
    "\n  <td %s width=\"10%%\"><b>%s</b></td>"    // peer address
    "\n  <td %s width=\"20%%\"><b>Start:</b> %s</td>"    // start time
    "\n  <td %s width=\"20%%\"><b>End:</b> %s</td>"      // start time
    "\n  <td %s width=\"10%%\">[%" PRId64 "ms]</td>"  // duration
    "\n  <td %s width=\"35%%\">%s</td>"           // method
    "\n  <td %s width=\"5%%\">%s</td>"           // stream
    "\n</tr>",
    kStatuszColLine, strutil::XmlStrEscape(stat.peer_address()).c_str(),
    kStatuszColLine, strutil::XmlStrEscape(timer::StrHumanTimeMs(start_ms)).c_str(),
    kStatuszColLine, response_ms ? strutil::XmlStrEscape(
      timer::StrHumanTimeMs(response_ms)).c_str() : "-",
    kStatuszColLine, response_ms ? response_ms - start_ms : int64(0),
    kStatuszColLine, strutil::XmlStrEscape(stat.method_txt()).c_str(),
    kStatuszColLine, stat.is_streaming() ? strutil::XmlStrEscape(
      strutil::StrHumanBytes(stat.streamed_size())).c_str() : "-");
  if (stat.has_error_txt()) {
    s += strutil::StringPrintf(
      "\n<tr>"
      "\n  <td bgcolor=\"#f8c0c0\" colspan=\"6\"><b>ERROR:</b> %s</td>"
      "\n</tr>",
      strutil::XmlStrEscape(stat.error_txt()).c_str());
  }
  s += strutil::StringPrintf(
    "\n<tr><td colspan=\"6\">Server Bytes: %" PRId64 " (raw: %" PRId64") "
    " Client Bytes: %" PRId64 " (raw: %" PRId64 ")</td></tr>",
    stat.server_size(), stat.server_raw_size(), stat.client_size(), stat.client_raw_size());
  s += strutil::StringPrintf(
    "\n<tr>"
    "\n  <td colspan=\"2\"><b>%s</b></td>"
    "\n  <td colspan=\"4\">Size: %s</td>"
    "\n</tr>",
    strutil::XmlStrEscape(stat.request_type_name()).c_str(),
    strutil::XmlStrEscape(strutil::StrHumanBytes(stat.request_size())).c_str());
  if (!stat.request_txt().empty()) {
    s += strutil::StringPrintf(
      "\n<tr><td colspan=\"6\"><font size=\"1\" face=\"courier\">%s</font></td></tr>",
      strutil::XmlStrEscape(stat.request_txt()).c_str());
  }
  if (stat.has_response_type_name()) {
    s += strutil::StringPrintf(
      "\n<tr>"
      "\n  <td colspan=\"2\"><b>%s</b></td>"
      "\n  <td colspan=\"4\">Size: %d</td>"
      "\n</tr>",
      strutil::XmlStrEscape(stat.response_type_name()).c_str(),
      stat.response_size());
    // strutil::XmlStrEscape(strutil::StrHumanBytes(stat.response_size())).c_str());
    if (!stat.response_txt().empty()) {
      s += strutil::StringPrintf(
        "\n<tr><td colspan=\"6\"><font size=\"1\" face=\"courier\">%s</font></td></tr>",
        strutil::XmlStrEscape(stat.response_txt()).c_str());
    }
  }
  return s;
}
std::string ServerStatsToHtml(const pb::ServerStats& stat) {
  std::string s;
  const int64 now = time(NULL);
  s += MachineStatsToHtml(stat.machine_stats());
  s += strutil::StringPrintf(
    "\n<h3>[%d] Live Requests:</h3><table cellpadding=\"3\" width=\"80%%\">\n",
    stat.live_req_size());
  for (int i = 0; i < stat.live_req_size(); ++i) {
    s += "\n";
    s += RequestStatsToHtml(stat.live_req(i), stat.now_ts(), now);
  }
  s += "\n</table>";
  s += "\n<h3>Completed Requests:</h3><table cellpadding=\"3\" width=\"80%%\">\n";
  for (int i = 0; i < stat.completed_req_size(); ++i) {
    s += "\n";
    s += RequestStatsToHtml(stat.completed_req(i), stat.now_ts(), now);
  }
  return s;
}
std::string ClientStatsToHtml(const pb::ClientStats& stat) {
  std::string s;
  const int64 now = time(NULL);
  s += strutil::StringPrintf("\n<h2>Client: %s</h2>",
                             strutil::XmlStrEscape(stat.client_name()).c_str());
  s += "\n<h3>Live Requests:</h3>\n";
  if (stat.live_req_size()) {
    s += "<table cellpadding=\"3\" width=\"80%%\">\n";
    for (int i = 0; i < stat.live_req_size(); ++i) {
      s += "\n";
      s += RequestStatsToHtml(stat.live_req(i), stat.now_ts(), now);
    }
    s += "\n</table>";
  }
  s += "\n<h3>Completed Requests:</h3>";
  if (stat.completed_req_size()) {
    s += "\n<table cellpadding=\"3\" width=\"80%%\">\n";
    for (int i = 0; i < stat.completed_req_size(); ++i) {
      s += "\n";
      s += RequestStatsToHtml(stat.completed_req(i), stat.now_ts(), now);
    }
    s += "\n</table>";
  }
  return s;
}

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
    mutex_(true),
    transport_(NULL), server_streaming_callback_(NULL) {
  Reset();
}

Controller::Controller(Transport* transport)
  : google::protobuf::RpcController(),
    mutex_(true),
    transport_(NULL), server_streaming_callback_(NULL)  {
  Reset();
  transport_ = transport;
}

Controller::~Controller() {
  CHECK(cancel_callback_ == NULL);
  Reset();
}

void Controller::Reset() {
  while (!streamed_messages_.empty()) {
    delete streamed_messages_.front();
    streamed_messages_.pop_front();
  }
  custom_content_type_.clear();
  error_code_ = rpc::ERROR_NONE;
  error_reason_.clear();
  cancel_callback_ = NULL;
  delete transport_;
  transport_ = NULL;
  timeout_ms_ = 0;
  is_urgent_ = false;
  is_finalized_ = false;
  compress_transfer_ = true;
  is_streaming_ = false;
  delete server_streaming_callback_;
  server_streaming_callback_ = NULL;
}

bool Controller::Failed() const {
  synch::MutexLocker l(&mutex_);
  return error_code_ != rpc::ERROR_NONE;
}

std::string Controller::ErrorText() const {
  synch::MutexLocker l(&mutex_);
  return strutil::StringPrintf("%d %s [%s]", error_code_,
                               rpc::GetErrorCodeString(error_code_),
                               error_reason_.c_str());
}

void Controller::StartCancel() {
  CallCancelCallback(true);
}

void Controller::SetFailed(const std::string& reason) {
  synch::MutexLocker l(&mutex_);
  is_finalized_ = true;
  if (error_code_ == rpc::ERROR_NONE) {
    error_code_ = rpc::ERROR_USER;
  }
  CHECK(!reason.empty());
  error_reason_ = reason;
}

rpc::ErrorCode Controller::GetErrorCode() {
  synch::MutexLocker l(&mutex_);
  return error_code_;
}

void Controller::SetErrorCode(rpc::ErrorCode code) {
  DCHECK_NE(code, rpc::ERROR_NONE);
  DCHECK_NE(code, rpc::ERROR_CANCELLED);

  synch::MutexLocker l(&mutex_);
  error_code_ = code;
  is_finalized_ = true;
}

bool Controller::IsCanceled() const {
  synch::MutexLocker l(&mutex_);
  return error_code_ == ERROR_CANCELLED;
}

bool Controller::IsFinalized() const {
  synch::MutexLocker l(&mutex_);
  return error_code_ != rpc::ERROR_NONE || is_finalized_;
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
    is_finalized_ = true;
  }
  cancel_callback_ = NULL;
  mutex_.Unlock();
  if (to_call != NULL) {
    to_call->Run();
  }
}
}  // namespace rpc
}  // namespace whisper
