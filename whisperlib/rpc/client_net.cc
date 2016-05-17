// -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
// Copyright: Urban Engines, Inc. 2012 onwards.
// All rights reserved.
// cp@urbanengines.com

#include "whisperlib/rpc/client_net.h"
#include "whisperlib/http/http_client_protocol.h"
#include "whisperlib/http/failsafe_http_client.h"

#ifdef USE_OPENSSL
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#endif

namespace {
whisper::http::BaseClientConnection* CreateConnection(
  whisper::net::Selector* selector,
  whisper::net::NetFactory* net_factory,
  whisper::net::PROTOCOL net_protocol) {
  return new whisper::http::SimpleClientConnection(selector, *net_factory, net_protocol);
}

whisper::net::NetFactory* CreateNetFactory(whisper::net::Selector* selector,
                                           SSL_CTX* ssl_context) {
  whisper::net::NetFactory* net_factory = new whisper::net::NetFactory(selector);
#ifdef USE_OPENSSL
  if (ssl_context) {
    whisper::net::SslConnectionParams ssl_params(ssl_context);
    net_factory->SetSslParams(whisper::net::SslAcceptorParams(), ssl_params);
  }
#endif
  return net_factory;
}
}

namespace whisper {
namespace rpc {

ClientNet::ClientNet(net::Selector* selector,
                     const http::ClientParams* params,
                     const std::vector<net::HostPort>& servers,
                     const char* host_header,
                     int num_retries,
                     int32 request_timeout_ms,
                     int32 reopen_connection_interval_ms,
                     SSL_CTX* ssl_context)
    : selector_(selector),
      ssl_context_(ssl_context),
      net_factory_(CreateNetFactory(selector, ssl_context)),
      fsc_(new http::FailSafeClient(
               selector_, params, servers,
               whisper::NewPermanentCallback(
                 &CreateConnection, selector_, net_factory_,
#ifdef USE_OPENSSL
                 ssl_context == NULL
                 ? net::PROTOCOL_TCP : net::PROTOCOL_SSL
#else
                 net::PROTOCOL_TCP
#endif
                 ),
               true, num_retries, request_timeout_ms,
               reopen_connection_interval_ms,
               host_header == NULL ? "" : host_header)) {
}

ClientNet::~ClientNet() {
    selector_->DeleteInSelectLoop(fsc_);
    selector_->DeleteInSelectLoop(net_factory_);
#ifdef USE_OPENSSL
    selector_->RunInSelectLoop(whisper::NewCallback(&SSL_CTX_free, ssl_context_));
#else
    delete ssl_context_;
#endif
}

std::vector<net::HostPort> ClientNet::ToServerVec(const std::string& server_name,
                                                  size_t num_connections) {
  std::vector<std::string> server_hostports;
  strutil::SplitString(server_name, ",", &server_hostports);
  std::vector<net::HostPort> servers;
  for (size_t j = 0; j < server_hostports.size(); ++j) {
      net::HostPort hp(server_hostports[j]);
      if (!hp.IsInvalid()) {
          for (size_t i = 0; i < num_connections; ++i) {
              servers.push_back(hp);
          }
      }
  }
  return servers;
}

#ifdef USE_OPENSSL

SSL_CTX* ClientNet::CreateSslContext(const std::string& pem_text,
                                     const std::string& filename,
                                     const std::string& dirname,
                                     std::string* error) {
    SSL_METHOD* method = const_cast<SSL_METHOD*>(SSLv23_method());
    if (method == NULL) {
        if (error) *error = strutil::StringPrintf(
            "Cannot create SSL method - did you init the lib ?. Code: %u",
            ERR_peek_last_error());
        return NULL;
    }
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (ctx == NULL) {
        if (error) *error = strutil::StringPrintf(
            "Cannot create SSL ctx - did you init the lib ?. Code: %u",
            ERR_peek_last_error());
        return NULL;
    }
    if (!pem_text.empty()) {
        BIO* cbio = BIO_new(BIO_s_mem());
        BIO_puts(cbio, pem_text.c_str());
        X509_STORE* store = X509_STORE_new();
        while (X509* cert = PEM_read_bio_X509(cbio, NULL, 0, NULL)) {
            if (!X509_STORE_add_cert(store, cert)) {
                if (error) *error = strutil::StringPrintf(
                    "Cannot use the provided certificate. Code: %u",
                    ERR_peek_last_error());
                X509_free(cert); X509_STORE_free(store); SSL_CTX_free(ctx); return NULL;
            }
        }
        SSL_CTX_set_cert_store(ctx, store);
        BIO_free(cbio);
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, NULL);
    }
    if (!filename.empty() || !dirname.empty()) {
        if (!SSL_CTX_load_verify_locations(
                ctx, filename.empty() ? NULL : filename.c_str(),
                dirname.empty() ? NULL : dirname.c_str())) {
            if (error) *error = strutil::StringPrintf(
                "Cannot load the provided locations. Code: %u",
                ERR_peek_last_error());
            SSL_CTX_free(ctx); return NULL;
        }
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, NULL);
    }
    return ctx;
}

SSL_CTX* ClientNet::CreateSslContextFromText(const string& contents) {
    std::string error;
    SSL_CTX* ctx = CreateSslContext(contents, "", "", &error);
    CHECK(ctx != NULL) << error;
    return ctx;
}

#else
SSL_CTX* ClientNet::CreateSslContext(const std::string& pem_text,
                                     const std::string& filename,
                                     const std::string& dirname,
                                     std::string* error) {
    *error = "Openssl not linked in.";
    return NULL;
}
SSL_CTX* ClientNet::CreateSslContextFromText(const string& contents) {
    return NULL;
}
#endif

}  // namespace rpc
}  // namespace whisper
