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

#include <whisperlib/base/types.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/gflags.h>

#include <whisperlib/http/http_header.h>
#include <whisperlib/io/buffer/memory_stream.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(data_file,
              "",
              "We test w/ headers from this file");
DEFINE_int32(memstream_bufsize,
             128,
             "We dimmensionate the MemoryStream buffer to "
             "this value");

//////////////////////////////////////////////////////////////////////

struct TestData {
  bool is_strict;
  const char header[1024];
  http::Header::ParseError expected_error;
  const char test_field_name[256];
  const char test_field_content[256];
};

const TestData test[] = {
  { true,
    "GET /en-US/firefox/bookmarks/%0A HTTP/1.0\r\n"
    "User-Agent: Wget/1.10.2\r\n"
    "  GIGI MARGA\r\n"
    "Accept: */*\r\n"
    "Host: en-us.add-ons.mozilla.com\r\n"
    "Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==\r\n"
    "Connection: Keep-Alive\r\n"
    "\r\n",
    http::Header::READ_OK,
    "User-Agent", "Wget/1.10.2 GIGI MARGA"
  }, {
    false,
    "GeT /en-US/firefox/bookmarks/%0A HTTP/1.0\r\n"
    "Accept: */*\r\n"
    "Host: en-us\0300.add-ons.mozilla.com\r\n"
    "   xxx abcd\r\n"
    "Connection: Keep-Alive\r\n"
    "\r\n",
    http::Header::READ_BAD_FIELD_SPEC,
    "Host", "en-us\0300.add-ons.mozilla.com xxx abcd"
  }, {
    true,
    "GeT /en-US/firefox/bookmarks/%0A HTTP/1.0\r\n"
    "Accept: */*\r\n"
    "Host: en-us\0300.add-ons.mozilla.com\r\n"
    "   xxx abcd\r\n"
    "Connection: Keep-Alive\r\n"
    "\r\n",
    http::Header::READ_BAD_FIELD_SPEC,
    "Host", ""
  }, {
    false,
    "GeT /en-US/firefox/bookmarks/%0A\r\n"
    "Accept: */*\r\n"
    "Host: en-us\0300.add-ons.mozilla.com\r\n"
    "Connection: Keep-Alive\r\n"
    "\r\n",
    http::Header::READ_NO_REQUEST_VERSION,
    "Host", "en-us\0300.add-ons.mozilla.com"
  }, {
    true,
    "GET /en-US/firefox/bookmarks/%0A HTTP/1.0\r\n"
    "User-Agent: Wget/1.10.2\r\n"
    "Accept: */*\r\n"
    "Host: en-us.add-ons.mozilla.com\r\n"
    "Connection: Keep-Alive\r\n"
    "User-Agent: GIGI MARGA\r\n"
    "\r\n",
    http::Header::READ_OK,
    "User-Agent", "Wget/1.10.2, GIGI MARGA"
  },
};

void RunSimpleTests() {
  for ( int i = 0; i < NUMBEROF(test); ++i ) {
    http::Header headers(test[i].is_strict);
    LOG_INFO << " ======================================== \n"
             << " TEST " << i;
    io::MemoryStream ms;
    ms.Write(test[i].header);
    headers.Clear();
    CHECK(headers.ParseHttpRequest(&ms));
    CHECK_EQ(headers.parse_error(), test[i].expected_error);
    LOG_INFO << "==================\n"
             << headers.ToString();
    string s;
    if ( *test[i].test_field_content ) {
      CHECK(headers.FindField(
              string(test[i].test_field_name), &s));
      CHECK_EQ(s, string(test[i].test_field_content));
    } else {
      CHECK(!headers.FindField(
              string(test[i].test_field_name), &s));
    }
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  http::Header htest(true);
  io::MemoryStream mstest;
  mstest.Write("GET /en-US/firefox/bookmarks/%0A HTTP/1.0\r\n"
               "Date1: Sun, 06 Nov 1994 08:49:37 GMT\r\n"
               "Date2: Sunday, 06-Nov-94 08:49:37 GMT\r\n"
               "Date3:  Sun Nov  6 08:49:37 1994\r\n"
               "Transfer-Encoding:   \tchunked\r\n"
               "Accept-Charset: iso-8859-5, unicode-1-1;q=0.8\r\n"
               "Accept: text/plain; q=0.5, text/html, \r\n"
               "   text/x-dvi; q=0.8, text/x-c,\r\n"
               "   audio/*; q=0.3, audio/mp3; q = 0.7 , */*; q=0.01\r\n"
               "Accept-Encoding: gzip;q=1.0, identity; q=0.5, *;q=0\r\n"
               "Host: en-us.add-ons.mozilla.com\r\n"
               "Connection: Keep-Alive\r\n"
               "\r\n");
  CHECK(htest.ParseHttpRequest(&mstest));
  CHECK_NE(htest.GetDateField("Date1"), 0);
  CHECK_EQ(htest.GetDateField("Date1"), htest.GetDateField("Date2"));
  // This can be in different time zone - just make sure divides by 3600
  CHECK_EQ((htest.GetDateField("Date1") - htest.GetDateField("Date3")) % 3600, 0);
  CHECK_EQ(htest.method(), http::METHOD_GET);
  CHECK_EQ(htest.http_version(), http::VERSION_1_0);
  CHECK(htest.IsChunkedTransfer());
  float value;
  value = htest.GetHeaderAcceptance(
    http::kHeaderAccept, "text/plain", "text/*", "*/*");
  CHECK_EQ(value, 0.5f);
  value = htest.GetHeaderAcceptance(
    http::kHeaderAccept, "gigi/marga", "gigi/*", "*/*");
  CHECK_EQ(value, 0.01f);
  value = htest.GetHeaderAcceptance(
    http::kHeaderAccept, "text/x-dvi", "text/*", "*/*");
  CHECK_EQ(value, 0.8f);
  value = htest.GetHeaderAcceptance(
    http::kHeaderAccept, "audio/x-dvi", "audio/*", "*/*");
  CHECK_EQ(value, 0.3f);
  value = htest.GetHeaderAcceptance(
    http::kHeaderAccept, "audio/mp3", "audio/*", "*/*");
  CHECK_EQ(value, 0.7f);
  value = htest.GetHeaderAcceptance(
    http::kHeaderAcceptCharset, "iso-8859-5", "", "*");
  CHECK_EQ(value, 1.0f);
  value = htest.GetHeaderAcceptance(
    http::kHeaderAcceptCharset, "UNICODE-1-1", "", "*");
  CHECK_EQ(value, 0.8f);
  value = htest.GetHeaderAcceptance(
    http::kHeaderAcceptEncoding, "deflate", "", "*");
  CHECK_EQ(value, 0.0f);
  value = htest.GetHeaderAcceptance(
    http::kHeaderAcceptEncoding, "identity", "", "*");
  CHECK_EQ(value, 0.5f);

  CHECK(htest.IsGzipAcceptableEncoding());

  CHECK(!htest.SetAuthorizationField("gigi_marga:", "zuzub0#$A"));
  string user, passwd;
  CHECK(!htest.GetAuthorizationField(&user, &passwd));
  CHECK(htest.SetAuthorizationField("gigi_marga", "zuzub0#$A"));
  CHECK(htest.GetAuthorizationField(&user, &passwd));
  CHECK_EQ(user, string("gigi_marga"));
  CHECK_EQ(passwd, string("zuzub0#$A"));

  string ss = http::Header::NormalizeFieldName(
    string(" ABC DEF  \t "));
  CHECK_EQ(ss, "Abc-Def");
  ss = http::Header::NormalizeFieldName("GiGi--MarG A  \t ");
  CHECK_EQ(ss, "Gigi--Marg-A");

  CHECK(!http::Header::IsValidFieldName("  \t Gigi Marga "));
  CHECK(http::Header::IsValidFieldName("Gigi \t Marga "));
  CHECK(!http::Header::IsValidFieldName("Gigi \t Ma\0340rga "));
  CHECK(http::Header::IsValidFieldName("Content-Length"));
  CHECK(!http::Header::IsValidFieldName("\t  "));
  CHECK(!http::Header::IsValidFieldName(""));
  CHECK(!http::Header::IsValidFieldName("Content:Length"));
  CHECK(!http::Header::IsValidFieldName("Content<Length"));

  CHECK(http::Header::IsValidFieldContent(""));
  CHECK(http::Header::IsValidFieldContent("\t sds "));
  CHECK(!http::Header::IsValidFieldContent("\t sd\0300s "));
  CHECK(http::Header::IsValidFieldContent(
          "\t !@#$%^&*()_+{}[]|:;\"'\\<>,./?"));

  RunSimpleTests();

  if ( !FLAGS_data_file.empty() ) {
    io::MemoryStream ms(FLAGS_memstream_bufsize);
    FILE* f = fopen(FLAGS_data_file.c_str(), "r");
    CHECK_NOT_NULL(f);
    static const size_t max_buf_size = 1000000;
    char* const buffer = new char[max_buf_size];
    const size_t cb = fread(buffer, 1, max_buf_size, f);
    CHECK_GT(cb, 0);
    ms.AppendRaw(buffer, cb);
    const int32 size = ms.Size();

    http::Header headers(true);
    int64 parsed_bytes = 0;
    bool is_req = true;
    while ( !ms.IsEmpty() ) {
      int sz1 = ms.Size();
      CHECK_EQ(sz1, ms.Size());

      if ( is_req ) {
        CHECK(headers.ParseHttpRequest(&ms));
      } else {
        CHECK(headers.ParseHttpReply(&ms));
      }
      parsed_bytes += headers.bytes_parsed();
      CHECK_EQ(headers.parse_error(), http::Header::READ_OK);

      io::MemoryStream ss;
      string s;
      headers.AppendToStream(&ss);
      CHECK(ss.ReadString(&s));
      LOG_INFO << " ======================================== \n"
               << s;
      is_req = !is_req;
      headers.Clear();
    }
    CHECK_EQ(parsed_bytes, size);
  }
  LOG_INFO << "PASS!";
}
