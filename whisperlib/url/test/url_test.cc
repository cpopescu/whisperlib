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

#include "whisperlib/url/url.h"

#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/system.h"

using namespace std;
using namespace whisper;

void LogUrl(const char* surl) {
  URL url(surl);
  vector< pair<string, string> > qp;
  int num = url.GetQueryParameters(&qp, true);
  LOG_INFO << "\n==================================================="
           << "\nSTRING: " << surl
           << "\nURL: " << url.is_valid()
           << "\n scheme: [" << url.scheme() << "]"
           << "\n spec: [" << url.spec() << "]"
           << "\n host: [" << url.host() << "]"
           << "\n port: [" << url.port() << "] / ["
           << url.IntPort() << "]"
           << "\n path: [" << url.path() << "] / ["
           << (url.path().size() > 0
               ? url.PathForRequest().c_str() : "--?--") << "]"
           << "\n query: [" << url.query() << "]"
           << "\n ref: [" << url.ref()  << "]";
  for ( int i = 0; i < num; i++ ) {
    LOG_INFO << " Param " << i << " [" << qp[i].first << "] = ["
             << qp[i].second << "]";
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  LogUrl("http://www.google.com/");
  LogUrl("http://www.google.com/gigi/marga?xyz=abc%5c&zuzu=aba+mucu");
  LogUrl("http://www.google.com/gigi/marga?x yz=ab[c%5c&zuzu=aba mucu");
  LogUrl("http://www.google.com/gigi/marga?lulu=~!@#$%^&*()_+");
  LogUrl("http://www.google.com/gigi/marga?lulu=`1234567890-=");
  LogUrl("http://www.google.com/gigi/%7E%21%40%23%24%25%5E%26"
         "%2A%28%29%5F%2B?%601234567890%2D%3D=%5B%5D%5C%3B%27"
         "%2C%2E%2F&%7B%7D%7C%3A%22%3C%3E%3F=");
  LogUrl("@#$http://www.*&google.com/gigi/%7E%21%40%23%24%25%5E"
         "%26%2A%28%29%5F%2B?%601234567890%2D%3D=%5B%5D%5C%3B%27"
         "%2C%2E%2F&%7B%7D%7C%3A%22%3C%3E%3F=");
  LogUrl("http://www.google.com/gigi/marga?xyz=abc%A7%BC");
  LogUrl("http://h/a/b/c/d?x=10&y=20");

  LOG_INFO << URL::UrlEscape("abc$%-=");
  CHECK_EQ(URL::UrlUnescape(URL::UrlEscape("abc$%-=")), "abc$%-=");
}
