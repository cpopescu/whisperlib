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
// Author: Catalin Popescu

#ifndef __NET_BASE_USER_AUTHENTICATOR_H__
#define __NET_BASE_USER_AUTHENTICATOR_H__

// This is more of an interface for authenticating users..

#include <string>
#include <map>
#include "whisperlib/base/types.h"
#include "whisperlib/base/callback.h"

#include "whisperlib/base/hash.h"
#include WHISPER_HASH_MAP_HEADER

namespace whisper {
namespace net {

class UserAuthenticator {
 public:
  explicit UserAuthenticator(const std::string& realm)
      : realm_(realm) {
  }
  virtual ~UserAuthenticator() {
  }

  enum Answer {
    Authenticated = 0,
    BadUser,
    BadPassword,
    MissingCredentials,
  };
  static const char* AnswerName(Answer answer) {
    switch (answer) {
      CONSIDER(Authenticated);
      CONSIDER(BadUser);
      CONSIDER(BadPassword);
      CONSIDER(MissingCredentials);
    }
    return "=UNKNOWN=";
  }
  typedef Callback1<Answer> AnswerCallback;
  // The main authentication function - synchronous version
  virtual Answer Authenticate(const std::string& user,
                              const std::string& passwd) const = 0;

  // The main authentication function - asynchronous version
  virtual void Authenticate(const std::string& user,
                            const std::string& passwd,
                            AnswerCallback* answer_callback) const = 0;
  const std::string& realm() const {
    return realm_;
  }
 private:
  const std::string realm_;

  DISALLOW_EVIL_CONSTRUCTORS(UserAuthenticator);
};

// A simple authenticator - not the smartest thing you can do, and definitely
// not the most secure, as the passwords are kept in clear, in memory, but
// for something not very demanding, on a secure machine, this should be
// probably fine..
class SimpleUserAuthenticator : public UserAuthenticator {
 public:
  explicit SimpleUserAuthenticator(const std::string& realm)
      : UserAuthenticator(realm) {
  }
  virtual ~SimpleUserAuthenticator() {
  }

  virtual Answer Authenticate(const std::string& user,
                              const std::string& passwd) const {
    hash_map<std::string, std::string>::const_iterator it =
        user_passwords_.find(user);
    if ( it == user_passwords_.end() ) {
      return BadUser;
    }
    if ( it->second != passwd ) {
      return BadPassword;
    }
    return Authenticated;
  }
  virtual void Authenticate(const std::string& user,
                            const std::string& passwd,
                            Callback1<Answer>* result_callback) const {
    result_callback->Run(Authenticate(user, passwd));
  }

  void set_user_password(const std::string& user,
                         const std::string& passwd) {
    user_passwords_[user] = passwd;
  }
  void remove_user(const std::string& user) {
    user_passwords_.erase(user);
  }
  const hash_map<std::string, std::string>& user_passwords() const {
    return user_passwords_;
  }
 private:
  hash_map<std::string, std::string> user_passwords_;

  DISALLOW_EVIL_CONSTRUCTORS(SimpleUserAuthenticator);
};
}   // namespace net
}   // namespace whisper

#endif  // __NET_BASE_USER_AUTHENTICATOR_H__
