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

#ifndef __WHISPERLIB_RPC_RPC_CONTROLLER_H__
#define __WHISPERLIB_RPC_RPC_CONTROLLER_H__

#include <whisperlib/base/types.h>
#include <whisperlib/net/address.h>
#include <google/protobuf/service.h>
#include <whisperlib/sync/mutex.h>

namespace net {
class Selector;
}
namespace rpc {

enum ErrorCode {
  ERROR_NONE = 0,         // No error - all smooth
  ERROR_CANCELLED = 1,    // The request is cancelled
  ERROR_USER = 2,         // Error set by user at server side
  ERROR_CLIENT = 3,       // Error in the client stack
  ERROR_NETWORK = 4,      // Error on the network
  ERROR_SERVER = 5,       // HTTP error from the server
  ERROR_PARSE = 6,        // Error parsing the protobuf
};
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

  int64 timeout_ms() const { return timeout_ms_; }
  void set_timeout_ms(int64 timeout_ms) { timeout_ms_ = timeout_ms; }

private:
  mutable synch::Mutex mutex_;
  Transport* transport_;
  google::protobuf::Closure* cancel_callback_;
  ErrorCode error_code_;
  string error_reason_;
  int64 timeout_ms_;

  DISALLOW_EVIL_CONSTRUCTORS(Controller);
};

}

#endif
