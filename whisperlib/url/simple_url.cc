// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
//
// (c) Copyright 2011, 1618labs
// All rights reserved.
// Author: Catalin Popescu (cp@1618labs.com)
//

#include "whisperlib/url/simple_url.h"
#include "whisperlib/base/scoped_ptr.h"

int URL::IntPort() const {
  if (!has_port()) {
    return -1;
  }
  const int port = strtol(port_.c_str(), NULL, 10);
  if ( port <= 0 ) {
    return -1;
  }
  return port;
}

string URL::PathForRequest() const {
  if (!is_valid()) {
    return "";
  }
  if (!has_query() && !has_ref()) {
    return path_;
  }
  string path;
  if (path_.empty()) {
    path.append("/");
  } else {
    path.append(path_);
  }
  if (has_query()) {
    path.append("?");
    path.append(query_);
  }
  if (has_ref()) {
    path.append("#");
    path.append(ref_);
  }
  return path;
}

void URL::Invalidate() {
  is_valid_ = false;

  spec_.clear();
  scheme_.clear();
  user_.clear();
  host_.clear();
  port_.clear();
  path_.clear();
  query_.clear();
  ref_.clear();
}

inline string space2plus(const string& s) {
  string ret;
  ret.reserve(s.size());
  for (int ndx = 0; ndx < s.size(); ++ndx) {
    if (s[ndx] == ' ') {
      ret += "%20";
    } else {
      ret += s[ndx];
    }
  }
  return ret;
}

const string& URL::Reassemble() {
  spec_ = scheme_ + "://";
  if (!user_.empty()) {
    spec_ += user_;
    spec_ += '@';
  }
  spec_ += host_;
  if (!port_.empty()) {
    spec_ += ':';
    spec_ += port_;
  }
  spec_ += path_;
  if (!query_.empty()) {
    spec_ += '?';
    spec_ += query_;
  }
  if (!ref_.empty()) {
    spec_ += '#';
    spec_ += ref_;
  }
  return spec_;
}

inline uint8 hexval(char c) {
  if ( c >= '0' && c <= '9' ) return static_cast<uint8>(c - '0');
  if ( c >= 'a' && c <= 'f' ) return static_cast<uint8>(10 + c - 'a');
  if ( c >= 'A' && c <= 'F' ) return static_cast<uint8>(10 + c - 'A');
  return 0x10;
}

void URL::ParseSpec() {
  is_valid_ = true;
  const size_t scheme_pos = spec_.find("://");
  size_t next_pos = 0;
  if (scheme_pos == string::npos) {
    Invalidate();
    return;
  }
  next_pos = scheme_pos + 3;
  scheme_ = spec_.substr(0, scheme_pos);
  if (scheme_.empty()) {
    Invalidate();
    return;
  }
  const size_t host_port_pos = spec_.find("/", next_pos);
  if (host_port_pos != string::npos) {
    ParseHostPort(spec_.substr(next_pos, host_port_pos - next_pos));
  } else {
    Invalidate();
    return;
  }

  const size_t query_pos = spec_.find("?", host_port_pos);
  if (query_pos == string::npos) {
    const size_t ref_pos = spec_.find("#", host_port_pos);
    if (ref_pos == string::npos) {
      path_ = UrlEscape(UrlUnescape(spec_.substr(host_port_pos)));
      return;
    }
    path_ = UrlEscape(UrlUnescape(
                        spec_.substr(host_port_pos, ref_pos - host_port_pos)));
    ref_ = spec_.substr(ref_pos + 1);
    return;
  }
  path_ = UrlEscape(UrlUnescape(
                      spec_.substr(host_port_pos, query_pos - host_port_pos)));
  const size_t ref_pos = spec_.find("#", query_pos);
  if (ref_pos == string::npos) {
    query_ = space2plus(spec_.substr(query_pos + 1));
    return;
  }
  query_ = space2plus(spec_.substr(query_pos + 1, ref_pos - query_pos - 1));
  ref_ = spec_.substr(ref_pos + 1);
}

void URL::ParseHostPort(const string& host_port) {
  const size_t user_pos = host_port.find('@');
  const size_t port_pos = host_port.rfind(':');
  if (user_pos != string::npos) {
    user_ = host_port.substr(0, user_pos);
    host_ = host_port.substr(user_pos + 1, port_pos);
  } else {
    host_ = host_port.substr(0, port_pos);
  }
  if (port_pos != string::npos) {
    port_ = host_port.substr(port_pos + 1);
  }
}

enum CharacterFlags {
  PASS = 0,
  ESCAPE = 1,
};

const unsigned char kCharLookup[0x100] = {
//   NULL     control chars...
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
//   control chars...
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
//   ' '      !        "        #        $        %        &        '        (        )        *        +        ,        -        .        /
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  PASS,    ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  PASS,    PASS,    PASS,    PASS,
//   0        1        2        3        4        5        6        7        8        9        :        ;        <        =        >        ?
     PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
//   @        A        B        C        D        E        F        G        H        I        J        K        L        M        N        O
     PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,
//   P        Q        R        S        T        U        V        W        X        Y        Z        [        \        ]        ^        _
     PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    ESCAPE,  PASS,    ESCAPE,  PASS,
//   `        a        b        c        d        e        f        g        h        i        j        k        l        m        n        o
     ESCAPE,  PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,
//   p        q        r        s        t        u        v        w        x        y        z        {        |        }        ~        <NBSP>
     PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    ESCAPE,  ESCAPE,  ESCAPE,  PASS,    ESCAPE,
//   ...all the high-bit characters are escaped
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,
     ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE,  ESCAPE};


string URL::UrlEscape(const char* spec, int len) {
  int ndx = 0;
  string s;
  while ( ndx < len ) {
    const char c = spec[ndx];
    if ( kCharLookup[uint8(c)] == ESCAPE ) {

      s += strutil::StrToUpper(
        strutil::StringPrintf("%c%02x", '%', static_cast<int32>(c & 0xff)));
    } else {
      s += c;
    }
    ++ndx;
  }
  return s;
}

string URL::UrlUnescape(const char* spec, int len) {
  scoped_array<char> dest(new char[len + 1]);
  char* d = dest.get();
  const char* p = spec;
  const char* const end = spec + len;
  const char* esc = NULL;
  uint8 esc_val = 0;

  while ( *p && p < end ) {
    if ( *p == '%' ) {
      esc = p;
      esc_val = 0;
    } else if ( esc ) {
      uint8 val = hexval(*p);
      if ( val > 0xf ) {
        while (esc <= p) {
          *d++ = *esc++;
        }
        esc = NULL;
      } else {
        esc_val = (esc_val * 16 + val);
        if (p - esc == 2) {
          *d++ = static_cast<char>(esc_val);
          esc = NULL;
        }
      }
    } else if (*p == '+') {
      *d++ = ' ';
    } else {
      *d++ = *p;
    }
    ++p;
  }
  return string(dest.get(), d - dest.get());
}
