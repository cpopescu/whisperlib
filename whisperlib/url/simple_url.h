// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
//
// Simple URL implementation -for a system w/o utf8 libraries (as android).
// Good enough for whisperlib http usage. If utf8 is available, use the
// open source gurl, which deals w/ utf8 / unicode.  This library will not do
// very well for that, but is good enough for client libraries that know
// to prepare ascii only urls.
//
#ifndef __WHISPERLIB_URL_SIMPLE_URL_H__
#define __WHISPERLIB_URL_SIMPLE_URL_H__

#include <string>
#include <vector>
#include <ostream>

#include <whisperlib/base/strutil.h>
#include <whisperlib/net/address.h>

class URL {
public:
  // Creates an empty, invalid URL.
  URL()
    : is_valid_(false) {
  }

  // Copy construction is relatively inexpensive, with most of the time going
  // to reallocating the string. It does not re-parse.
  URL(const URL& other)
    : spec_(other.spec_) {
    ParseSpec();
  }

  // Requires the url to be Ascii
  explicit URL(const string& url_string)
    : spec_(url_string) {
    ParseSpec();
  }

  bool is_valid() const {
    return is_valid_;
  }
  bool is_empty() const {
    return spec_.empty();
  }
  const string& spec() const {
    return spec_;
  }

  // Defiant equality operator!
  const URL& operator=(const URL& other) {
    spec_ = spec_;
    ParseSpec();
    return *this;
  }
  bool operator==(const URL& other) const {
    return spec_ == other.spec_;
  }
  bool operator!=(const URL& other) const {
    return spec_ != other.spec_;
  }

  // Allows URL to used as a key in STL (for example, a set or map).
  bool operator<(const URL& other) const {
    return spec_ < other.spec_;
  }

  // Resolves a URL that's possibly relative to this object's URL, and returns
  // it. Absolute URLs are also handled according to the rules of URLs on web
  // pages.
  //
  // It is an error to resolve a URL relative to an invalid URL. The result
  // will be the empty URL.
  URL Resolve(const string& relative_path) const {
    URL url(*this);
    url.path_ = strutil::NormalizeUrlPath(
      strutil::JoinPaths(path_, relative_path));
    url.Reassemble();
    return url;
  }

  // Returns true if the given parameter (should be lower-case ASCII to match
  // the canonicalized scheme) is the scheme for this URL. This call is more
  // efficient than getting the scheme and comparing it because no copies or
  // object constructions are done.
  bool SchemeIs(const char* lower_ascii_scheme) const {
    return scheme_ == lower_ascii_scheme;
  }

  // We often need to know if this is a file URL. File URLs are "standard", but
  // are often treated separately by some programs.
  bool SchemeIsFile() const {
    return SchemeIs("file");
  }

  // If the scheme indicates a secure connection
  bool SchemeIsSecure() const {
    return SchemeIs("https");
  }

  // Returns true if the hostname is an IP address. Note: this function isn't
  // as cheap as a simple getter because it re-parses the hostname to verify.
  // This currently identifies only IPv4 addresses (bug 822685).
  bool HostIsIPAddress() const {
    if (!has_host()) {
      return false;
    }
    net::IpAddress addr(host_);
    return !addr.IsInvalid();
  }

  // Getters for various components of the URL. The returned string will be
  // empty if the component is empty or is not present.
  const string& scheme() const {
    return scheme_;
  }
  const string& user() const {
    return user_;
  }
  const string& host() const {
    return host_;
  }
  const string& port() const {
    return port_;
  }
  const string& path() const {
    return path_;
  }
  const string& query() const {
    return query_;
  }
  const string& ref() const {
    return ref_;
  }

  bool has_scheme() const {
    return !scheme_.empty();
  }
  bool has_host() const {
    return !host_.empty();
  }
  bool has_user() const {
    return !user_.empty();
  }
  bool has_port() const {
    return !port_.empty();
  }
  bool has_path() const {
    return !path_.empty();
  }
  bool has_query() const {
    return !query_.empty();
  }
  bool has_ref() const {
    return !ref_.empty();
  }

  // Returns a parsed version of the port. Can also be any of the special
  // values defined in Parsed for ExtractPort.
  int IntPort() const;

  // Returns the path that should be sent to the server. This is the path,
  // parameter, and query portions of the URL. It is guaranteed to be ASCII.
  string PathForRequest() const;

  // Utility function that unescapes a % / + encoded url piece to the binary
  // unescaped underneath string.
  static string UrlUnescape(const char* spec, int len);
  static string UrlUnescape(const string& s) {
    return UrlUnescape(s.c_str(), s.size());
  }

  // Utility function that escapes a string to be included in urls
  static string UrlEscape(const char* spec, int len);
  static string UrlEscape(const string& s) {
    return UrlEscape(s.c_str(), s.size());
  }

  const string& Reassemble();

private:
  void ParseSpec();
  void ParseHostPort(const string& host_port);
  void Invalidate();

  string spec_;

  bool is_valid_;

  string scheme_;
  string user_;
  string host_;
  string port_;
  string path_;
  string query_;
  string ref_;
};

// Stream operator so URL can be used in assertion statements.
inline ostream& operator<<(ostream& out, const URL& url) {
    return out << url.spec();
}

#endif  // __WHISPERLIB_URL_SIMPLE_URL_H__
