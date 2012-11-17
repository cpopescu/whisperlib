// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
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

#ifdef WIN32
#include <windows.h>
#endif

#include <algorithm>

#include "whisperlib/url/google-url/gurl.h"

#include "whisperlib/base/log.h"
#include "whisperlib/url/google-url/url_canon_stdstring.h"
#include "whisperlib/url/google-url/url_util.h"
#include "whisperlib/url/google-url/url_canon_internal.h"

namespace {

// External template that can handle initialization of either character type.
// The input spec is given, and the canonical version will be placed in
// |*canonical|, along with the parsing of the canonical spec in |*parsed|.
template<typename CHAR>
bool InitCanonical(const std::basic_string<CHAR>& input_spec,
                   std::string* canonical,
                   url_parse::Parsed* parsed) {
  // Reserve enough room in the output for the input, plus some extra so that
  // we have room if we have to escape a few things without reallocating.
  canonical->reserve(input_spec.size() + 32);
  url_canon::StdStringCanonOutput output(canonical);
  bool success = url_util::Canonicalize(
      input_spec.data(), static_cast<int>(input_spec.length()),
      &output, parsed);

  output.Complete();  // Must be done before using string.
  return success;
}

// Returns a static reference to an empty string for returning a reference
// when there is no underlying string.
const std::string& EmptyStringForURL() {
#ifdef WIN32
  // Avoid static object construction/destruction on startup/shutdown.
  static std::string* empty_string = NULL;
  if (!empty_string) {
    // Create the string. Be careful that we don't break in the case that this
    // is being called from multiple threads. Statics are not threadsafe.
    std::string* new_empty_string = new std::string;
    if (InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID*>(&empty_string), new_empty_string, NULL)) {
      // The old value was non-NULL, so no replacement was done. Another
      // thread did the initialization out from under us.
      delete new_empty_string;
    }
  }
  return *empty_string;
#else
  // TODO(brettw) Write a threadsafe Unix version!
  static std::string empty_string;
  return empty_string;
#endif
}

} // namespace

URL::URL() : is_valid_(false) {
}

URL::URL(const URL& other)
    : spec_(other.spec_),
      is_valid_(other.is_valid_),
      parsed_(other.parsed_) {
}

URL::URL(const std::string& url_string) {
  is_valid_ = InitCanonical(url_string, &spec_, &parsed_);
}

URL::URL(const UTF16String& url_string) {
  is_valid_ = InitCanonical(url_string, &spec_, &parsed_);
}

URL::URL(const char* canonical_spec, int canonical_spec_len,
           const url_parse::Parsed& parsed, bool is_valid)
    : spec_(canonical_spec, canonical_spec_len),
      is_valid_(is_valid),
      parsed_(parsed) {
#ifndef NDEBUG
  // For testing purposes, check that the parsed canonical URL is identical to
  // what we would have produced. Skip checking for invalid URLs have no meaning
  // and we can't always canonicalize then reproducabely.
  if (is_valid_) {
    URL test_url(spec_);

    DCHECK(test_url.is_valid_ == is_valid_);
    DCHECK(test_url.spec_ == spec_);

    DCHECK(test_url.parsed_.scheme == parsed_.scheme);
    DCHECK(test_url.parsed_.username == parsed_.username);
    DCHECK(test_url.parsed_.password == parsed_.password);
    DCHECK(test_url.parsed_.host == parsed_.host);
    DCHECK(test_url.parsed_.port == parsed_.port);
    DCHECK(test_url.parsed_.path == parsed_.path);
    DCHECK(test_url.parsed_.query == parsed_.query);
    DCHECK(test_url.parsed_.ref == parsed_.ref);
  }
#endif
}

const std::string& URL::spec() const {
  if (is_valid_ || spec_.empty())
    return spec_;

  DCHECK(false) << "Trying to get the spec of an invalid URL!";
  return EmptyStringForURL();
}

// Note: code duplicated below (it's inconvenient to use a template here).
URL URL::Resolve(const std::string& relative) const {
  // Not allowed for invalid URLs.
  if (!is_valid_)
    return URL();

  URL result;

  // Reserve enough room in the output for the input, plus some extra so that
  // we have room if we have to escape a few things without reallocating.
  result.spec_.reserve(spec_.size() + 32);
  url_canon::StdStringCanonOutput output(&result.spec_);

  if (!url_util::ResolveRelative(spec_.data(), parsed_, relative.data(),
                                 static_cast<int>(relative.length()),
                                 &output, &result.parsed_)) {
    // Error resolving, return an empty URL.
    return URL();
  }

  output.Complete();
  result.is_valid_ = true;
  return result;
}

// Note: code duplicated above (it's inconvenient to use a template here).
URL URL::Resolve(const UTF16String& relative) const {
  // Not allowed for invalid URLs.
  if (!is_valid_)
    return URL();

  URL result;

  // Reserve enough room in the output for the input, plus some extra so that
  // we have room if we have to escape a few things without reallocating.
  result.spec_.reserve(spec_.size() + 32);
  url_canon::StdStringCanonOutput output(&result.spec_);

  if (!url_util::ResolveRelative(spec_.data(), parsed_, relative.data(),
                                 static_cast<int>(relative.length()),
                                 &output, &result.parsed_)) {
    // Error resolving, return an empty URL.
    return URL();
  }

  output.Complete();
  result.is_valid_ = true;
  return result;
}

// Note: code duplicated below (it's inconvenient to use a template here).
URL URL::ReplaceComponents(
    const url_canon::Replacements<char>& replacements) const {
  URL result;

  // Not allowed for invalid URLs.
  if (!is_valid_)
    return URL();

  // Reserve enough room in the output for the input, plus some extra so that
  // we have room if we have to escape a few things without reallocating.
  result.spec_.reserve(spec_.size() + 32);
  url_canon::StdStringCanonOutput output(&result.spec_);

  result.is_valid_ = url_util::ReplaceComponents(
      spec_.data(), parsed_, replacements,
      &output, &result.parsed_);

  output.Complete();
  return result;
}

// Note: code duplicated above (it's inconvenient to use a template here).
URL URL::ReplaceComponents(
    const url_canon::Replacements<UTF16Char>& replacements) const {
  URL result;

  // Not allowed for invalid URLs.
  if (!is_valid_)
    return URL();

  // Reserve enough room in the output for the input, plus some extra so that
  // we have room if we have to escape a few things without reallocating.
  result.spec_.reserve(spec_.size() + 32);
  url_canon::StdStringCanonOutput output(&result.spec_);

  result.is_valid_ = url_util::ReplaceComponents(
      spec_.data(), parsed_, replacements,
      &output, &result.parsed_);

  output.Complete();
  return result;
}

URL URL::GetWithEmptyPath() const {
  // This doesn't make sense for invalid or nonstandard URLs, so return
  // the empty URL.
  if (!is_valid_ || !SchemeIsStandard())
    return URL();

  // We could optimize this since we know that the URL is canonical, and we are
  // appending a canonical path, so avoiding re-parsing.
  URL other(*this);
  if (parsed_.path.len == 0)
    return other;

  // Clear everything after the path.
  other.parsed_.query.reset();
  other.parsed_.ref.reset();

  // Set the path, since the path is longer than one, we can just set the
  // first character and resize.
  other.spec_[other.parsed_.path.begin] = '/';
  other.parsed_.path.len = 1;
  other.spec_.resize(other.parsed_.path.begin + 1);
  return other;
}

bool URL::SchemeIs(const char* lower_ascii_scheme) const {
  if (parsed_.scheme.len <= 0)
    return lower_ascii_scheme == NULL;
  return url_util::LowerCaseEqualsASCII(spec_.data() + parsed_.scheme.begin,
                                        spec_.data() + parsed_.scheme.end(),
                                        lower_ascii_scheme);
}

bool URL::SchemeIsStandard() const {
  return url_util::IsStandardScheme(&spec_[parsed_.scheme.begin],
                                    parsed_.scheme.len);
}

int URL::IntPort() const {
  if (parsed_.port.is_nonempty())
    return url_parse::ParsePort(spec_.data(), parsed_.port);
  return url_parse::PORT_UNSPECIFIED;
}

std::string URL::ExtractFileName() const {
  url_parse::Component file_component;
  url_parse::ExtractFileName(spec_.data(), parsed_.path, &file_component);
  return ComponentString(file_component);
}

std::string URL::PathForRequest() const {
  DCHECK(parsed_.path.len > 0) << "Canonical path for requests should be non-empty";
  if (parsed_.ref.len >= 0) {
    // Clip off the reference when it exists. The reference starts after the #
    // sign, so we have to subtract one to also remove it.
    return std::string(spec_, parsed_.path.begin,
                       parsed_.ref.begin - parsed_.path.begin - 1);
  }

  // Use everything form the path to the end.
  return std::string(spec_, parsed_.path.begin);
}

bool URL::HostIsIPAddress() const {
  if (!is_valid_ || spec_.empty())
     return false;

  url_canon::RawCanonOutputT<char, 128> ignored_output;
  url_parse::Component ignored_component;
  return url_canon::CanonicalizeIPAddress(spec_.c_str(), parsed_.host,
                                          &ignored_output, &ignored_component);
}

#ifdef WIN32

// static
const URL& URL::EmptyURL() {
  // Avoid static object construction/destruction on startup/shutdown.
  static URL* empty_gurl = NULL;
  if (!empty_gurl) {
    // Create the string. Be careful that we don't break in the case that this
    // is being called from multiple threads.
    URL* new_empty_gurl = new URL;
    if (InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID*>(&empty_gurl), new_empty_gurl, NULL)) {
      // The old value was non-NULL, so no replacement was done. Another
      // thread did the initialization out from under us.
      delete new_empty_gurl;
    }
  }
  return *empty_gurl;
}

#endif  // WIN32

bool URL::DomainIs(const char* lower_ascii_domain,
                    int domain_len) const {
  // Return false if this URL is not valid or domain is empty.
  if (!is_valid_ || !parsed_.host.is_nonempty() || !domain_len)
    return false;

  // Check whether the host name is end with a dot. If yes, treat it
  // the same as no-dot unless the input comparison domain is end
  // with dot.
  const char* last_pos = spec_.data() + parsed_.host.end() - 1;
  int host_len = parsed_.host.len;
  if ('.' == *last_pos && '.' != lower_ascii_domain[domain_len - 1]) {
    last_pos--;
    host_len--;
  }

  // Return false if host's length is less than domain's length.
  if (host_len < domain_len)
    return false;

  // Compare this url whether belong specific domain.
  const char* start_pos = spec_.data() + parsed_.host.begin +
                          host_len - domain_len;

  if (!url_util::LowerCaseEqualsASCII(start_pos,
                                      last_pos + 1,
                                      lower_ascii_domain,
                                      lower_ascii_domain + domain_len))
    return false;

  // Check whether host has right domain start with dot, make sure we got
  // right domain range. For example www.google.com has domain
  // "google.com" but www.iamnotgoogle.com does not.
  if ('.' != lower_ascii_domain[0] && host_len > domain_len &&
      '.' != *(start_pos - 1))
    return false;

  return true;
}

void URL::Swap(URL* other) {
  spec_.swap(other->spec_);
  std::swap(is_valid_, other->is_valid_);
  std::swap(parsed_, other->parsed_);
}

string URL::UrlUnescape(const char* spec, int len) {
  int ndx = 0;
  string s;
  char value;
  url_canon::StdStringCanonOutput str_canon(&s);
  while ( ndx < len ) {
    if ( spec[ndx] == '%' &&
         url_canon::DecodeEscaped<char>(spec, &ndx, len, &value) ) {
      str_canon.push_back(value);
    } else if ( spec[ndx] == '+' ) {
      str_canon.push_back(' ');
    } else {
      str_canon.push_back(spec[ndx]);
    }
    ++ndx;
  }
  return string(str_canon.data(), str_canon.length());
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
     ESCAPE,  PASS,    ESCAPE,  ESCAPE,  PASS,    ESCAPE,  ESCAPE,  PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,    PASS,
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
  url_canon::StdStringCanonOutput str_canon(&s);
  while ( ndx < len ) {
    if ( kCharLookup[uint8(spec[ndx])] == ESCAPE ) {
      url_canon::AppendEscapedChar(spec[ndx], &str_canon);
    } else {
      str_canon.push_back(spec[ndx]);
    }
    ++ndx;
  }
  return string(str_canon.data(), str_canon.length());
}


int URL::GetQueryParameters(
  std::vector< std::pair<string, string> >* components,
  bool unescape) const {
  url_parse::Component tmp_query(parsed_.query);
  url_parse::Component key, value;
  int num = 0;
  while ( url_parse::ExtractQueryKeyValue(
            spec_.c_str(), &tmp_query, &key, &value) ) {
    ++num;
    if ( unescape ) {
      components->push_back(
        std::make_pair(UrlUnescape(spec_.c_str() + key.begin, key.len),
                       UrlUnescape(spec_.c_str() + value.begin, value.len)));
    } else {
      components->push_back(std::make_pair(ComponentString(key),
                                           ComponentString(value)));
    }
  }
  return num;
}
