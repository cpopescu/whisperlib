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

#include "whisperlib/base/log.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/base/system.h"


int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  {
    string s("',P6=-7E|c\\\\\\\\$'+@kz[\\\"\\\"3l]jtDkyt?$!o8>zOax4@U8F`W4^54^H!>5vb\\\"pWArX2,t\\/[Y");
    CHECK_EQ(strutil::JsonStrEscape(strutil::JsonStrUnescape(s)), s);
  }

  LOG_INFO << " Testing strutil::StrEql";
  const char* test = "12345";
  CHECK(strutil::StrEql(test, test));
  CHECK(strutil::StrEql("", ""));
  CHECK(!strutil::StrEql("abcd", "abcde"));
  CHECK(strutil::StrEql("abcd", "abcd"));
  CHECK(!strutil::StrEql("fdsakhfsalkjh abcd", "abcde"));

  LOG_INFO << " Testing strutil::StrIEql";
  CHECK(strutil::StrIEql("", ""));
  CHECK(strutil::StrIEql(test, test));
  CHECK(!strutil::StrIEql("abcd", "abcde"));
  CHECK(strutil::StrIEql("abcd", "aBcD"));
  CHECK(strutil::StrIEql("Abcd", "aBcD"));
  CHECK(!strutil::StrIEql("Abcd", "aBcDe"));
  CHECK(!strutil::StrEql("abcd", "falkjgyabcde"));

  LOG_INFO << " Testing strutil::ToHex";
  const unsigned char buf[] = { 0xff, 0x12, 0xe9, 0x0c, 0xad, 0xb6, 0x75 };
  CHECK(strutil::StrEql(strutil::ToHex(buf, 7), "ff12e90cadb675"));
  CHECK(strutil::StrEql(strutil::ToHex("abcxyz"), "61626378797a"));
  CHECK(strutil::StrEql(strutil::ToHex("hello"), "68656c6c6f"));
  CHECK(strutil::StrEql(strutil::ToHex("9876543210"), "39383736353433323130"));

  LOG_INFO << " Testing strutil::StrPrefix";
  CHECK(strutil::StrPrefix(test, test));
  CHECK(strutil::StrPrefix("abcedf", "abc"));
  CHECK(strutil::StrPrefix("abcedf", ""));
  CHECK(strutil::StrPrefix("", ""));
  CHECK(!strutil::StrPrefix("abc", "abcedf"));
  CHECK(!strutil::StrPrefix("adfabc", "adfb"));

  LOG_INFO << " Testing strutil::ShiftLeftBuffer";
  char test_shift[100];
  strcpy(test_shift, "abcdefghijkl");
  strutil::ShiftLeftBuffer(test_shift, sizeof(test_shift - 1), 0, 0);
  CHECK(!strcmp(test_shift, "abcdefghijkl")) << "[" << test_shift << "]";
  strutil::ShiftLeftBuffer(test_shift, 5, 3, 'x');
  CHECK(!strcmp(test_shift, "dexxxfghijkl")) << "[" << test_shift << "]";
  strutil::ShiftLeftBuffer(test_shift, 1, 1, 'y');
  CHECK(!strcmp(test_shift, "yexxxfghijkl")) << "[" << test_shift << "]";
  strutil::ShiftLeftBuffer(test_shift, 10, 0, 'y');
  CHECK(!strcmp(test_shift, "yexxxfghijkl")) << "[" << test_shift << "]";
  strutil::ShiftLeftBuffer(test_shift, strlen(test_shift),
                           strlen(test_shift), 'z');
  CHECK(!strcmp(test_shift, "zzzzzzzzzzzz")) << "[" << test_shift << "]";

  LOG_INFO << " Testing strutil::JoinStrings";
  const char* test_join[] = {"aaaaa", "bbb", "cccccc", "dd" };
  CHECK_EQ(strutil::JoinStrings(test_join, NUMBEROF(test_join), " "),
           string("aaaaa bbb cccccc dd"));
  vector<string> test_join2;
  CHECK_EQ(strutil::JoinStrings(test_join2, " "), string(""));
  test_join2.push_back(string("AAA"));
  test_join2.push_back(string("BB"));
  test_join2.push_back(string("CCCC"));
  test_join2.push_back(string("D"));
  CHECK_EQ(strutil::JoinStrings(test_join2, "x"), string("AAAxBBxCCCCxD"));

  LOG_INFO << " Testing strutil::Basename";
  CHECK_STREQ(strutil::Basename("a"), "a");
  CHECK_STREQ(strutil::Basename("/a"), "a");
  CHECK_STREQ(strutil::Basename("/a/b"), "b");
  CHECK_STREQ(strutil::Basename("/a/b/c"), "c");
  CHECK_STREQ(strutil::Basename("/alda hk/biyf fi/8740%#%/c123456"), "c123456");
  CHECK_STREQ(strutil::Basename("/a^%#2/tg/ftrdd09[/blksau  834/c123456%^%"),
              "c123456%^%");

  LOG_INFO << " Testing strutil::Dirname";
  CHECK_EQ(strutil::Dirname("/a"), string(""));
  CHECK_EQ(strutil::Dirname("/a/b"), string("/a"));
  CHECK_EQ(strutil::Dirname("/a/b/c"), string("/a/b"));
  CHECK_EQ(strutil::Dirname("/alda hk/biyf fi/8740%#%/c123456"),
           string("/alda hk/biyf fi/8740%#%"));

  LOG_INFO << " Testing strutil::StringPrintf";
  CHECK_EQ(strutil::StringPrintf("%d", static_cast<int32>(1234)),
           string("1234"));
  CHECK_EQ(strutil::StringPrintf("%08d", static_cast<int32>(1234)),
           string("00001234"));
  CHECK_EQ(strutil::StringPrintf("%s %d", "abcd", static_cast<int32>(1234)),
           string("abcd 1234"));
  CHECK_EQ(strutil::StringPrintf("%s %d %.4f", "abcd",
                                 static_cast<int32>(1234), 3.456),
           string("abcd 1234 3.4560"));

  LOG_INFO << " Testing strutil::SplitString";
#define TEST_SPLIT_BASE(str, sep, nExpectedTokens, strExpectedJoinedTokens,  \
                        fun) {                                          \
      vector<string> tokens;                                            \
      strutil::fun(str, sep, &tokens);                                  \
      string strJoinedTokens = strutil::JoinStrings(tokens, ",");       \
      LOG_INFO << #fun "(\"" << str << "\", \""                         \
               << sep << "\") => #" << tokens.size()                    \
               << " {" << strJoinedTokens << "}";                       \
      CHECK_EQ(tokens.size(), nExpectedTokens);                         \
      CHECK_EQ(strJoinedTokens, strExpectedJoinedTokens);               \
  }
#define TEST_SPLIT(str, sep, nExpectedTokens, strExpectedJoinedTokens) \
  TEST_SPLIT_BASE(str, sep, nExpectedTokens, strExpectedJoinedTokens, SplitString)

  TEST_SPLIT("abcbcd", "b", 3, (string() + "a" + "," + "c" + "," + "cd"));
  TEST_SPLIT("abc", "abc", 2, (string() + "" + "," + ""));
  TEST_SPLIT("abc", "a", 2, (string() + "" + "," + "bc"));
  TEST_SPLIT("abc", "c", 2, (string() + "ab" + "," + ""));
  TEST_SPLIT("abc", "", 3, (string() + "a" + "," + "b" + "," + "c"));
  TEST_SPLIT("", "a", 1, (string() + ""));
  TEST_SPLIT("", "", 0, (string()));

#define TEST_SPLIT_ANY(str, sep, nExpectedTokens, strExpectedJoinedTokens) \
  TEST_SPLIT_BASE(str, sep, nExpectedTokens,                            \
                  strExpectedJoinedTokens, SplitStringOnAny)

  TEST_SPLIT_ANY("abc 443 #$ 43 @#", " $#", 4, string("abc,443,43,@"));
  TEST_SPLIT_ANY("# abc 443 #x$ zw", " $#", 4, string("abc,443,x,zw"));
  TEST_SPLIT_ANY(" #$ #$$$   ", " $#", 0, string(""));
  TEST_SPLIT_ANY("", " $#", 0, string(""));

  LOG_INFO << " Testing strutil::StrEscape";
  CHECK_EQ(strutil::StrEscape("", '#', ","), "");
  CHECK_EQ(strutil::StrEscape("a", '#', ","), "a");
  CHECK_EQ(strutil::StrEscape("a,", '#', ","), "a#2c");
  CHECK_EQ(strutil::StrEscape(".", '#', "."), "#2e");
  CHECK_EQ(strutil::StrEscape("#", '#', "."), "#23");
  CHECK_EQ(strutil::StrEscape("##", '#', "."), "#23#23");
  CHECK_EQ(strutil::StrEscape("#a#", '#', "."), "#23a#23");
  CHECK_EQ(strutil::StrEscape("a, b, c", '#', ","), "a#2c b#2c c");
  CHECK_EQ(strutil::StrEscape("a,., b,c", '#', ",."), "a#2c#2e#2c b#2cc");
  CHECK_EQ(strutil::StrEscape("a,., b,##c", '#', ",."),
           "a#2c#2e#2c b#2c#23#23c");
  CHECK_EQ(strutil::StrEscape("a,b\r\na,b\n", '#', ",\r\n"),
           "a#2cb#0d#0aa#2cb#0a");
  CHECK_EQ(strutil::StrEscape("#a,b\r\na,b\n#", '#', ",\r\n"),
           "#23a#2cb#0d#0aa#2cb#0a#23");

  LOG_INFO << " Testing strutil::StrUnescape";
  CHECK_EQ(strutil::StrUnescape("", '#'), "");
  CHECK_EQ(strutil::StrUnescape("a", '#'), "a");
  CHECK_EQ(strutil::StrUnescape("a#2c", '#'), "a,");
  CHECK_EQ(strutil::StrUnescape("#2e", '#'), ".");
  CHECK_EQ(strutil::StrUnescape("#23", '#'), "#");
  CHECK_EQ(strutil::StrUnescape("#23#23", '#'), "##");
  CHECK_EQ(strutil::StrUnescape("#23a#23", '#'), "#a#");
  CHECK_EQ(strutil::StrUnescape("a#2c b#2c c", '#'), "a, b, c");
  CHECK_EQ(strutil::StrUnescape("a#2c#2e#2c b#2cc", '#'), "a,., b,c");
  CHECK_EQ(strutil::StrUnescape("a#2c#2e#2c b#2c#23#23c", '#'), "a,., b,##c");
  CHECK_EQ(strutil::StrUnescape("a#2cb#0d#0aa#2cb#0a", '#'), "a,b\r\na,b\n");
  CHECK_EQ(strutil::StrUnescape("#23a#2cb#0d#0aa#2cb#0a#23", '#'),
           "#a,b\r\na,b\n#");

  CHECK_STREQ(strutil::NormalizePath("").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("/").c_str(), "/");
  CHECK_STREQ(strutil::NormalizePath("//").c_str(), "//");
  CHECK_STREQ(strutil::NormalizePath("///").c_str(), "/");
  CHECK_STREQ(strutil::NormalizePath("f").c_str(), "f");
  CHECK_STREQ(strutil::NormalizePath("foo").c_str(), "foo");
  CHECK_STREQ(strutil::NormalizePath("foo/").c_str(), "foo/");
  CHECK_STREQ(strutil::NormalizePath("f/").c_str(), "f/");
  CHECK_STREQ(strutil::NormalizePath("/foo").c_str(), "/foo");
  CHECK_STREQ(strutil::NormalizePath("foo/bar").c_str(), "foo/bar");
  CHECK_STREQ(strutil::NormalizePath("..").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("../..").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("/..").c_str(), "/");
  CHECK_STREQ(strutil::NormalizePath("/../..").c_str(), "/");
  CHECK_STREQ(strutil::NormalizePath("../foo").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/..").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/../").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/...").c_str(), "foo/...");
  CHECK_STREQ(strutil::NormalizePath("foo/.../").c_str(), "foo/.../");
  CHECK_STREQ(strutil::NormalizePath("foo/..bar").c_str(), "foo/..bar");
  CHECK_STREQ(strutil::NormalizePath("../f").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("/../f").c_str(), "/");
  CHECK_STREQ(strutil::NormalizePath("f/..").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/../..").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/../../").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/../../..").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/../../../").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/../bar").c_str(), "bar");
  CHECK_STREQ(strutil::NormalizePath("foo/../bar/").c_str(), "bar/");
  CHECK_STREQ(strutil::NormalizePath("foo/bar/..").c_str(), "foo");
  CHECK_STREQ(strutil::NormalizePath("foo/bar/../").c_str(), "foo/");
  CHECK_STREQ(strutil::NormalizePath("foo/bar/../..").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/bar/../../").c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo/bar/../blah").c_str(), "foo/blah");
  CHECK_STREQ(strutil::NormalizePath("f/../b").c_str(), "b");
  CHECK_STREQ(strutil::NormalizePath("f/b/..").c_str(), "f");
  CHECK_STREQ(strutil::NormalizePath("f/b/../").c_str(), "f/");
  CHECK_STREQ(strutil::NormalizePath("f/b/../a").c_str(), "f/a");
  CHECK_STREQ(strutil::NormalizePath("foo/bar/blah/../..").c_str(), "foo");
  CHECK_STREQ(strutil::NormalizePath("foo/bar/blah/../../bletch").c_str(),
              "foo/bletch");
  CHECK_STREQ(strutil::NormalizePath("//net").c_str(), "//net");
  CHECK_STREQ(strutil::NormalizePath("//net/").c_str(), "//net/");
  CHECK_STREQ(strutil::NormalizePath("//..net").c_str(), "//..net");
  CHECK_STREQ(strutil::NormalizePath("//net/..").c_str(), "/");
  CHECK_STREQ(strutil::NormalizePath("//net/foo").c_str(), "//net/foo");
  CHECK_STREQ(strutil::NormalizePath("//net/foo/").c_str(), "//net/foo/");
  CHECK_STREQ(strutil::NormalizePath("//net/foo/..").c_str(), "//net");
  CHECK_STREQ(strutil::NormalizePath("//net/foo/../").c_str(), "//net/");

  CHECK_STREQ(strutil::NormalizePath("/net/foo/bar").c_str(),
              "/net/foo/bar");
  CHECK_STREQ(strutil::NormalizePath("/net/foo/bar/").c_str(),
              "/net/foo/bar/");
  CHECK_STREQ(strutil::NormalizePath("/net/foo/..").c_str(),
              "/net");
  CHECK_STREQ(strutil::NormalizePath("/net/foo/../").c_str(),
              "/net/");

  CHECK_STREQ(strutil::NormalizePath("//net//foo//bar").c_str(),
              "//net/foo/bar");
  CHECK_STREQ(strutil::NormalizePath("//net//foo//bar//").c_str(),
              "//net/foo/bar/");
  CHECK_STREQ(strutil::NormalizePath("//net//foo//..").c_str(),
              "//net");
  CHECK_STREQ(strutil::NormalizePath("//net//foo//..//").c_str(),
              "//net/");

  CHECK_STREQ(strutil::NormalizePath("///net///foo///bar").c_str(),
              "/net/foo/bar");
  CHECK_STREQ(strutil::NormalizePath("///net///foo///bar///").c_str(),
              "/net/foo/bar/");
  CHECK_STREQ(strutil::NormalizePath("///net///foo///..").c_str(),
              "/net");
  CHECK_STREQ(strutil::NormalizePath("///net///foo///..///").c_str(),
              "/net/");

  ///////////////////////////////////////////////////////////////////////

  CHECK_STREQ(strutil::NormalizePath("", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("#", '#').c_str(), "#");
  CHECK_STREQ(strutil::NormalizePath("##", '#').c_str(), "##");
  CHECK_STREQ(strutil::NormalizePath("###", '#').c_str(), "#");
  CHECK_STREQ(strutil::NormalizePath("f", '#').c_str(), "f");
  CHECK_STREQ(strutil::NormalizePath("foo", '#').c_str(), "foo");
  CHECK_STREQ(strutil::NormalizePath("foo#", '#').c_str(), "foo#");
  CHECK_STREQ(strutil::NormalizePath("f#", '#').c_str(), "f#");
  CHECK_STREQ(strutil::NormalizePath("#foo", '#').c_str(), "#foo");
  CHECK_STREQ(strutil::NormalizePath("foo#bar", '#').c_str(), "foo#bar");
  CHECK_STREQ(strutil::NormalizePath("..", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("..#..", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("#..", '#').c_str(), "#");
  CHECK_STREQ(strutil::NormalizePath("#..#..", '#').c_str(), "#");
  CHECK_STREQ(strutil::NormalizePath("..#foo", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#..", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#..#", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#...", '#').c_str(), "foo#...");
  CHECK_STREQ(strutil::NormalizePath("foo#...#", '#').c_str(), "foo#...#");
  CHECK_STREQ(strutil::NormalizePath("foo#..bar", '#').c_str(), "foo#..bar");
  CHECK_STREQ(strutil::NormalizePath("..#f", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("#..#f", '#').c_str(), "#");
  CHECK_STREQ(strutil::NormalizePath("f#..", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#..#..", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#..#..#", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#..#..#..", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#..#..#..#", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#..#bar", '#').c_str(), "bar");
  CHECK_STREQ(strutil::NormalizePath("foo#..#bar#", '#').c_str(), "bar#");
  CHECK_STREQ(strutil::NormalizePath("foo#bar#..", '#').c_str(), "foo");
  CHECK_STREQ(strutil::NormalizePath("foo#bar#..#", '#').c_str(), "foo#");
  CHECK_STREQ(strutil::NormalizePath("foo#bar#..#..", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#bar#..#..#", '#').c_str(), "");
  CHECK_STREQ(strutil::NormalizePath("foo#bar#..#blah", '#').c_str(), "foo#blah");
  CHECK_STREQ(strutil::NormalizePath("f#..#b", '#').c_str(), "b");
  CHECK_STREQ(strutil::NormalizePath("f#b#..", '#').c_str(), "f");
  CHECK_STREQ(strutil::NormalizePath("f#b#..#", '#').c_str(), "f#");
  CHECK_STREQ(strutil::NormalizePath("f#b#..#a", '#').c_str(), "f#a");
  CHECK_STREQ(strutil::NormalizePath("foo#bar#blah#..#..", '#').c_str(), "foo");
  CHECK_STREQ(strutil::NormalizePath("foo#bar#blah#..#..#bletch", '#').c_str(),
              "foo#bletch");
  CHECK_STREQ(strutil::NormalizePath("##net", '#').c_str(), "##net");
  CHECK_STREQ(strutil::NormalizePath("##net#", '#').c_str(), "##net#");
  CHECK_STREQ(strutil::NormalizePath("##..net", '#').c_str(), "##..net");
  CHECK_STREQ(strutil::NormalizePath("##net#..", '#').c_str(), "#");
  CHECK_STREQ(strutil::NormalizePath("##net#foo", '#').c_str(), "##net#foo");
  CHECK_STREQ(strutil::NormalizePath("##net#foo#", '#').c_str(), "##net#foo#");
  CHECK_STREQ(strutil::NormalizePath("##net#foo#..", '#').c_str(), "##net");
  CHECK_STREQ(strutil::NormalizePath("##net#foo#..#", '#').c_str(), "##net#");

  CHECK_STREQ(strutil::NormalizePath("#net#foo#bar", '#').c_str(),
              "#net#foo#bar");
  CHECK_STREQ(strutil::NormalizePath("#net#foo#bar#", '#').c_str(),
              "#net#foo#bar#");
  CHECK_STREQ(strutil::NormalizePath("#net#foo#..", '#').c_str(),
              "#net");
  CHECK_STREQ(strutil::NormalizePath("#net#foo#..#", '#').c_str(),
              "#net#");

  CHECK_STREQ(strutil::NormalizePath("##net##foo##bar", '#').c_str(),
              "##net#foo#bar");
  CHECK_STREQ(strutil::NormalizePath("##net##foo##bar##", '#').c_str(),
              "##net#foo#bar#");
  CHECK_STREQ(strutil::NormalizePath("##net##foo##..", '#').c_str(),
              "##net");
  CHECK_STREQ(strutil::NormalizePath("##net##foo##..##", '#').c_str(),
              "##net#");

  CHECK_STREQ(strutil::NormalizePath("###net###foo###bar", '#').c_str(),
              "#net#foo#bar");
  CHECK_STREQ(strutil::NormalizePath("###net###foo###bar###", '#').c_str(),
              "#net#foo#bar#");
  CHECK_STREQ(strutil::NormalizePath("###net###foo###..", '#').c_str(),
              "#net");
  CHECK_STREQ(strutil::NormalizePath("###net###foo###..###", '#').c_str(),
              "#net#");

  LOG_INFO << " Testing strutil::StrTrim";
  CHECK_EQ(strutil::StrTrimChars("", ""), "");
  CHECK_EQ(strutil::StrTrimChars("", " "), "");
  CHECK_EQ(strutil::StrTrimChars(" ", " "), "");
  CHECK_EQ(strutil::StrTrimChars("  ", " "), "");
  CHECK_EQ(strutil::StrTrimChars("   ", " "), "");
  CHECK_EQ(strutil::StrTrimChars(" a ", " "), "a");
  CHECK_EQ(strutil::StrTrimChars("a ", " "), "a");
  CHECK_EQ(strutil::StrTrimChars(" a", " "), "a");
  CHECK_EQ(strutil::StrTrimChars(" abcd ", " "), "abcd");
  CHECK_EQ(strutil::StrTrimChars("abcd ", " "), "abcd");
  CHECK_EQ(strutil::StrTrimChars(" abcd", " "), "abcd");
  CHECK_EQ(strutil::StrTrimChars("   a      a ", " "), "a      a");
  CHECK_EQ(strutil::StrTrimChars("      a  ", " "), "a");
  CHECK_EQ(strutil::StrTrimChars("a        a", " "), "a        a");
  CHECK_EQ(strutil::StrTrimChars("   a      a ", " "), "a      a");

  LOG_INFO << " Testing strutil::StrTrimChars";
  CHECK_EQ(strutil::StrTrimChars("", ""), "");
  CHECK_EQ(strutil::StrTrimChars("", "/"), "");
  CHECK_EQ(strutil::StrTrimChars("/", "/"), "");
  CHECK_EQ(strutil::StrTrimChars("//", "/"), "");
  CHECK_EQ(strutil::StrTrimChars("///", "/"), "");
  CHECK_EQ(strutil::StrTrimChars("/a/", "/"), "a");
  CHECK_EQ(strutil::StrTrimChars("a/", "/"), "a");
  CHECK_EQ(strutil::StrTrimChars("/a", "/"), "a");
  CHECK_EQ(strutil::StrTrimChars("/abcd/", "/"), "abcd");
  CHECK_EQ(strutil::StrTrimChars("abcd/", "/"), "abcd");
  CHECK_EQ(strutil::StrTrimChars("/abcd", "/"), "abcd");
  CHECK_EQ(strutil::StrTrimChars("///a//////a/", "/"), "a//////a");
  CHECK_EQ(strutil::StrTrimChars("//////a//", "/"), "a");
  CHECK_EQ(strutil::StrTrimChars("a////////a", "/"), "a////////a");
  CHECK_EQ(strutil::StrTrimChars("///a//////a/", "/"), "a//////a");

  LOG_INFO << " Testing strutil::StrToUpper";
  CHECK(strutil::StrEndsWith("", ""));
  CHECK(strutil::StrEndsWith("a", ""));
  CHECK(strutil::StrEndsWith("ala bala", ""));
  CHECK(strutil::StrEndsWith("abcdefgh", "h"));
  CHECK(strutil::StrEndsWith("abcdefgh", "gh"));
  CHECK(strutil::StrEndsWith("abcdefgh", "fgh"));
  CHECK(strutil::StrEndsWith("abcdefgh", "bcdefgh"));
  CHECK(strutil::StrEndsWith("abcdefgh", "abcdefgh"));
  CHECK(!strutil::StrEndsWith("", "a"));
  CHECK(!strutil::StrEndsWith("a", "b"));
  CHECK(!strutil::StrEndsWith("ala bala", "l"));
  CHECK(!strutil::StrEndsWith("abcdefgh", "ghi"));
  CHECK(!strutil::StrEndsWith("abcdefgh", "hi"));
  CHECK(!strutil::StrEndsWith("abcdefgh", "fg"));
  CHECK(!strutil::StrEndsWith("abcdefgh", "bbcdefgh"));
  CHECK(!strutil::StrEndsWith("abcdefgh", "abcdefghi"));

  LOG_INFO << " Testing strutil::JoinPaths";
  CHECK_EQ(strutil::JoinPaths("", ""), "");
  CHECK_EQ(strutil::JoinPaths("", "b"), "b");
  CHECK_EQ(strutil::JoinPaths("", "/b"), "/b");
  CHECK_EQ(strutil::JoinPaths("/", ""), "/");
  CHECK_EQ(strutil::JoinPaths("/", "b"), "/b");
  CHECK_EQ(strutil::JoinPaths("/", "/b"), "//b");
  CHECK_EQ(strutil::JoinPaths("/a", "b"), "/a/b");
  CHECK_EQ(strutil::JoinPaths("/a", "/b"), "/a/b");
  CHECK_EQ(strutil::JoinPaths("/a/b", "//c//d//"), "/a/b/c/d/");
  CHECK_EQ(strutil::JoinPaths("a", "b"), "a/b");
  CHECK_EQ(strutil::JoinPaths("a", "/b"), "a/b");
  CHECK_EQ(strutil::JoinPaths("a/", "b/"), "a/b/");

  LOG_INFO << " Testing strutil::StrToUpper";
  string s;
  CHECK_STREQ(strutil::StrToUpper(s).c_str(), "");
  s = "a";
  CHECK_STREQ(strutil::StrToUpper(s).c_str(), "A");
  s = "A";
  CHECK_STREQ(strutil::StrToUpper(s).c_str(), "A");
  s = "aA 12Xs";
  CHECK_STREQ(strutil::StrToUpper(s).c_str(), "AA 12XS");
  s = "Ac (0(12XD";
  CHECK_STREQ(strutil::StrToUpper(s).c_str(), "AC (0(12XD");

  LOG_INFO << " Testing strutil::StrToLower";
  s = "";
  CHECK_STREQ(strutil::StrToLower(s).c_str(), "");
  s = "a";
  CHECK_STREQ(strutil::StrToLower(s).c_str(), "a");
  s = "A";
  CHECK_STREQ(strutil::StrToLower(s).c_str(), "a");
  s = "aA 12Xs";
  CHECK_STREQ(strutil::StrToLower(s).c_str(), "aa 12xs");
  s = "Ac (0(12XD";
  CHECK_STREQ(strutil::StrToLower(s).c_str(), "ac (0(12xd");

  LOG_INFO << " Testing strutil::IsValidIdentifier";
  CHECK(!strutil::IsValidIdentifier(""));
  CHECK(!strutil::IsValidIdentifier("a@#"));
  CHECK(!strutil::IsValidIdentifier("_a"));
  CHECK(!strutil::IsValidIdentifier("!a"));
  CHECK(strutil::IsValidIdentifier("1a"));
  CHECK(!strutil::IsValidIdentifier("a_AZ!"));

  CHECK(strutil::IsValidIdentifier("a"));
  CHECK(strutil::IsValidIdentifier("A"));
  CHECK(strutil::IsValidIdentifier("a1"));
  CHECK(strutil::IsValidIdentifier("A1"));
  CHECK(strutil::IsValidIdentifier("azAZ_09"));
  CHECK(strutil::IsValidIdentifier("AazZ_09"));
  CHECK(strutil::IsValidIdentifier("AazZ_09_"));

  LOG_INFO << " Testing  strutil::StrMapFormat";
  {
  // string s("We found user ${User} who wants to \${10 access ${Resource}")
    map<string, string> m;
    m["User"] = "john";
    m["Resource"] = "disk";
    m["Bogus"] = "gugu";
    m["AHA}"] = "aha";
    CHECK_STREQ(strutil::StrMapFormat(
                     "We found user ${User} who wants to \\${10 "
                     "access ${Resource}.",
                     m, "${", "}", '\\').c_str(),
                 "We found user john who wants to ${10 access disk.");
    CHECK_STREQ(strutil::StrMapFormat(
                     "${User} \\${10 ${Resource} ${AHA\\}} $Vasile ${lili}.",
                     m, "${", "}", '\\').c_str(),
                "john ${10 disk aha $Vasile .");
    CHECK_STREQ(strutil::StrMapFormat(
                     "{{{User}}} \\${10 ${Resource} ${{{AHA\\}}}}",
                     m, "{{{", "}}}", '\\').c_str(),
                "john \\${10 ${Resource} $aha");
  }

  LOG_INFO << " Testing strutil::ToBinary";
  CHECK_STREQ(strutil::ToBinary(0x1234567890abcdefULL).c_str(),
              "0001001000110100010101100111100010010000101010111100110111101111");

  LOG_INFO << "PASS";
  common::Exit(0);
}
