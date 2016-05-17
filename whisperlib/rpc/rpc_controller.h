// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
//
// (c) Copyright 2011, Urban Engines
// All rights reserved.
// Author: Catalin Popescu (cp@urbanengines.com)
//

#ifndef __WHISPERLIB_RPC_RPC_CONTROLLER_H__
#define __WHISPERLIB_RPC_RPC_CONTROLLER_H__

#include "whisperlib/base/types.h"
#include "whisperlib/base/callback.h"
#include "whisperlib/net/address.h"
#include <google/protobuf/service.h>
#include "whisperlib/sync/mutex.h"
#include <deque>

namespace whisper {
namespace net {
class Selector;
}
namespace rpc {
namespace pb {
class RequestStats;
class ServerStats;
class MachineStats;
class ClientStats;
}

std::string MachineStatsToHtml(const pb::MachineStats& stat);
std::string RequestStatsToHtml(const pb::RequestStats& stat, int64 now_ts, int64 now_sec);
std::string ServerStatsToHtml(const pb::ServerStats& stat);
std::string ClientStatsToHtml(const pb::ClientStats& stat);
std::string GetStatuszHeadHtml();
enum ErrorCode {
  /** No error - all smooth */
  ERROR_NONE = 0,
  /** The request is cancelled (probably at client)
   */
  ERROR_CANCELLED = 1,
  /** Error set by user at server side for some usage based reason (bad request / 4xx) ?
   *  ==> Do not retry.
   */
  ERROR_USER = 2,
  /** Error set at the client side some internal errors
   *  ==> Can Retry.
   */
  ERROR_CLIENT = 3,
  /** Error set at the client side for bad network error
   *  ==> Can Retry
   */
  ERROR_NETWORK = 4,
  /** Error set at the server side for some transient server error
   *  ==> Can Retry
   */
  ERROR_SERVER = 5,
  /** Error parsing the returned protobuf - this may mean that you are
   *  talking some bad proto.
   *  ==> Do not retry.
   */
  ERROR_PARSE = 6,
};

/** Returns if an error should be retried. In general you can return */
inline bool IsErrorRetriable(ErrorCode error_code) {
  return error_code != ERROR_USER && error_code != ERROR_PARSE;
}
/** Name of the error */
const char* GetErrorCodeString(ErrorCode code);

// Class that contains the networking parameters for a
// server request to be processed.
class Transport {
 public:
  enum Protocol {
    TCP,
    HTTP,
  };
  explicit Transport(const Transport& transport)
    : selector_(transport.selector()),
      protocol_(transport.protocol()),
      local_address_(transport.local_address()),
      peer_address_(transport.peer_address()),
      user_(transport.user()),
      passwd_(transport.passwd()) {
  }

  Transport(net::Selector* selector,
            Protocol protocol,
            const net::HostPort& local_address,
            const net::HostPort& peer_address)
      : selector_(selector),
        protocol_(protocol),
        local_address_(local_address),
        peer_address_(peer_address) {
  }
  virtual ~Transport() {
  }

  net::Selector* selector() const            { return selector_;       }
  Protocol protocol() const                  { return protocol_;       }
  const net::HostPort& local_address() const { return local_address_;  }
  const net::HostPort& peer_address() const  { return peer_address_;   }
  const string& user() const                 { return user_;           }
  const string& passwd() const               { return passwd_;         }
  string* mutable_user()                     { return &user_;          }
  string* mutable_passwd()                   { return &passwd_;        }

  void set_user_passwd(const string& user,
                       const string& passwd) { user_ = user;
                                               passwd_ = passwd;       }

 protected:
  net::Selector* const selector_;
  const Protocol protocol_;
  const net::HostPort local_address_;
  const net::HostPort peer_address_;
  string user_;
  string passwd_;
};


/**
 * Implementation of the google protobuf RpcController.
 */
class Controller : public google::protobuf::RpcController {
public:
  // This version is used normally on client
  Controller();
  // This version is used on the server
  Controller(Transport* transport);
  virtual ~Controller();

  // Resets the rpc::Controller to its initial state so that it may be reused in
  // a new call.  Must not be called while an RPC is in progress.
  virtual void Reset();

  // After a call has finished, returns true if the call failed.  The possible
  // reasons for failure depend on the RPC implementation.  Failed() must not
  // be called before a call has finished.  If Failed() returns true, the
  // contents of the response message are undefined.
  virtual bool Failed() const;

  // If Failed() is true, returns a human-readable description of the error.
  virtual string ErrorText() const;

  // Advises the RPC system that the caller desires that the RPC call be
  // canceled.  The RPC system may cancel it immediately, may wait awhile and
  // then cancel it, or may not even cancel the call at all.  If the call is
  // canceled, the "done" callback will still be called and the rpc::Controller
  // will indicate that the call failed at that time.
  //
  // NOTE(cp): not fully tested - may crash occasionally
  //
  virtual void StartCancel();

  // Server-side methods ---------------------------------------------
  // These calls may be made from the server side only.  Their results
  // are undefined on the client side (may crash).

  // Causes Failed() to return true on the client side.  "reason" will be
  // incorporated into the message returned by ErrorText().  If you find
  // you need to return machine-readable information about failures, you
  // should incorporate it into your response protocol buffer and should
  // NOT call SetFailed().
  virtual void SetFailed(const string& reason);

  // In our implementation we use error codes for server specialized errors,
  // w/ failure reasons used for extra user detail.
  rpc::ErrorCode GetErrorCode();

  const string& GetErrorReason();

  // Sets the error reason for an RPC (cannot be ERROR_NONE or ERROR_CANCELLED)
  void SetErrorCode(rpc::ErrorCode code);

  // If true, indicates that the client canceled the RPC, so the server may
  // as well give up on replying to it.  The server should still call the
  // final "done" callback.
  virtual bool IsCanceled() const;

  // If the request is done.
  virtual bool IsFinalized() const;

  // Asks that the given callback be called when the RPC is canceled.  The
  // callback will always be called exactly once.  If the RPC completes without
  // being canceled, the callback will be called after completion.  If the RPC
  // has already been canceled when NotifyOnCancel() is called, the callback
  // will be called immediately.
  //
  // NotifyOnCancel() must be called no more than once per request.
  //
  virtual void NotifyOnCancel(google::protobuf::Closure* callback);

  // Calls the cancel callback, possibly setting the cancel flag.
  void CallCancelCallback(bool set_cancel);

  // Available *only* at server side:
  const Transport* transport() const { return transport_; }

  // TODO(cp) : timeout not implemented at this level
  int64 timeout_ms() const { return timeout_ms_; }
  void set_timeout_ms(int64 timeout_ms) { timeout_ms_ = timeout_ms; }

  bool is_urgent() const { return is_urgent_; }
  void set_is_urgent(bool is_urgent) { is_urgent_ = is_urgent; }

  bool compress_transfer() const { return compress_transfer_; }
  void set_compress_transfer(bool val) { compress_transfer_ = val; }

  // If the request is streaming / not streaming
  // For non streaming requests the done callback will be called once,
  // when the request is done.
  // For streaming the done callback is supposed to be permanent, and will be called for each
  // new response decoded from the pipe.
  bool is_streaming() const { return is_streaming_; }
  void set_is_streaming(bool is_streaming) { is_streaming_ = is_streaming; }

  void set_is_finalized() {
      synch::MutexLocker l(&mutex_);
      is_finalized_ = true;
  }

  // When streaming, new messages parsed from the server are appended to this vector.
  // Note that a NULL message can be appended when a message was sent but some parsing
  // error happened (like parsing). You can decide what to do in those cases.
  // The bool is false when no message is in the stream. You are responsible (new owner)
  // of any returned messages.
  std::pair<google::protobuf::Message*, bool> PopStreamedMessage() {
    synch::MutexLocker l(&mutex_);
    std::pair<google::protobuf::Message*, bool> ret =
        std::make_pair((google::protobuf::Message*)(NULL),
                       !streamed_messages_.empty());
    if (ret.second) {
      ret.first = streamed_messages_.front();
      streamed_messages_.pop_front();
    }
    return ret;
  }

  // Used by the framework to push a new message to the message stream. (on clients used
  // by the rpc subsystem to push messages received from the server; on servers used by the
  // server implementation to push more messages to be sent to the client).
  // VERY IMPORTANT: on servers always
  void PushStreamedMessage(google::protobuf::Message* msg) {
    synch::MutexLocker l(&mutex_);
    streamed_messages_.push_back(msg);
  }

  // Any messages to be streamed.
  bool HasStreamedMessage() const {
    synch::MutexLocker l(&mutex_);
    return !streamed_messages_.empty();
  }

  // On servers the implementation can set this closure (permanent) to be used by the rpc
  // implementation for flow control (when the server can push more messages in the
  // PushStreamMessage).
  // If this is null the rpc subsystem takes the signal the implementation finished its
  // streaming after the current run of streamed_messages_.
  // We do not own this callback - is your job to delete it. Set a NULL callback when you
  // finished your data to be sent.
  void set_server_streaming_callback(whisper::Closure* callback) {
    synch::MutexLocker l(&mutex_);
    DCHECK(!callback || callback->is_permanent());
    server_streaming_callback_ = callback;
  }

  whisper::Closure* server_streaming_callback() const {
    synch::MutexLocker l(&mutex_);
    return server_streaming_callback_;
  }

  void FinalizeStreamingOnNetworkError() {
    synch::MutexLocker l(&mutex_);
    if (server_streaming_callback_) {
      if (!Failed() && !IsCanceled()) {
        SetErrorCode(rpc::ERROR_NETWORK);
      }
      server_streaming_callback_->Run();
    }
    is_finalized_ = true;
  }

  const string& custom_content_type() const {
    return custom_content_type_;
  }
  void set_custom_content_type(const string& custom_content_type) {
    custom_content_type_ = custom_content_type;
  }

private:
  // reentrant per potential calling loop FinalizeStreamingOnNetworkError ->
  //   set_server_streaming_callback_se
  mutable synch::Mutex mutex_;
  Transport* transport_;
  google::protobuf::Closure* cancel_callback_;
  ErrorCode error_code_;
  string error_reason_;
  int64 timeout_ms_;    // TODO(cp): actually implement this
  bool is_urgent_;
  bool is_streaming_;
  bool is_finalized_;
  bool compress_transfer_;
  std::deque<google::protobuf::Message*> streamed_messages_;
  whisper::Closure* server_streaming_callback_;
  std::string custom_content_type_;

  DISALLOW_EVIL_CONSTRUCTORS(Controller);
};

}  // namespace rpc
}  // namespace whisper

#endif
