/** Copyright (c) 2014, Urban Engines inc.
 * All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Urban Engines inc nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Catalin Popescu
 */
#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/system.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/net/selector.h"
#include "whisperlib/rpc/client_net.h"
#include "whisperlib/http/http_client_protocol.h"
#include "whisperlib/http/failsafe_http_client.h"
#include "whisperlib/url/url.h"
#include "util/common/ue_chain_crt.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#endif

DEFINE_string(url, "", "Url to fetch");
DEFINE_string(accept_encoding, "gzip, deflate",
              "The contents of the accept encoding header.");
#ifdef USE_OPENSSL
DEFINE_string(cert_file, "", "Certificate check file for https. "
              "By default we use the urban engines server verification "
              "when empty.");
DEFINE_bool(no_verify, false,
            "Disable ssl verification.");
#endif
DEFINE_bool(stream_request, false,
            "Make the request a stream.");

void RequestDone(whisper::http::ClientRequest* req,
                 whisper::rpc::ClientNet* client,
                 whisper::net::Selector* selector) {
    std::cout << req->request()->server_data()->ToString();
    if (req->error() != whisper::http::CONN_OK) {
        LOG_ERROR << " Error in the connection : " << req->error_name();
    } else {
        LOG_INFO << " Connection finished OK.";
    }
    selector->DeleteInSelectLoop(req);
    selector->DeleteInSelectLoop(client);
    selector->RunInSelectLoop(
        whisper::NewCallback(selector, &whisper::net::Selector::MakeLoopExit));
}


bool StreamNext(whisper::http::ClientRequest* req,
                whisper::rpc::ClientNet* client,
                whisper::http::ClientStreamReceiverProtocol* stream,
                whisper::net::Selector* selector) {
    whisper::io::MemoryStream* const in = req->request()->server_data();
    const whisper::http::HttpReturnCode http_code =
        req->request()->server_header()->status_code();
    bool status = true;
    if (http_code != whisper::http::OK) {
        status = false;
        LOG_INFO << "Got a bad http error code: "
                 << http_code << " / "
                 << whisper::http::GetHttpReturnCodeDescription(http_code);
    }
    std::cout << in->ToString();
    in->Clear();
    if (!req->is_finalized()) {
        return status;
    }
    selector->DeleteInSelectLoop(req);
    selector->DeleteInSelectLoop(stream);
    selector->DeleteInSelectLoop(client);
    selector->RunInSelectLoop(
        whisper::NewCallback(selector, &whisper::net::Selector::MakeLoopExit));
    return false;
}

int main(int argc, char* argv[]) {
  whisper::common::Init(argc, argv);

  URL url(FLAGS_url);

  CHECK(url.is_valid());
  CHECK(!url.host().empty());
  CHECK(url.scheme() == "http" || url.scheme() == "https");
  const int port = url.IntPort() >= 0 ? url.IntPort()
      : (url.scheme() == "http" ? 80 : 443 );
  const std::string hostport =
      strutil::StringPrintf("%s:%d", url.host().c_str(), port);

  SSL_CTX* ctx = NULL;
  if (url.scheme() == "https") {
#ifndef USE_OPENSSL
      LOG_FATAL << "Open ssl not linked in - cannot fetch https.";
#else
      SSL_library_init();
      if (FLAGS_cert_file.empty() && !FLAGS_no_verify) {
          ctx = whisper::rpc::ClientNet::CreateSslContextFromText(kUeCertChain);
      } else {
          std::string error;
          if (FLAGS_no_verify) {
              ctx = whisper::rpc::ClientNet::CreateSslContext("", "", "", &error);
          } else {
              ctx = whisper::rpc::ClientNet::CreateSslContext("", FLAGS_cert_file, "", &error);
          }
          CHECK(ctx != NULL) << error;
      }
#endif
  }

  whisper::net::Selector selector;
  whisper::http::ClientParams params;
  params.dlog_level_ = true;
  whisper::rpc::ClientNet* client = new whisper::rpc::ClientNet(
      &selector, &params, whisper::rpc::ClientNet::ToServerVec(hostport),
      url.host().c_str(), 5, 60000, 1000, ctx);

  whisper::http::ClientRequest* req = new whisper::http::ClientRequest(
      whisper::http::METHOD_GET, &url);
  // req->request()->set_server_use_gzip_encoding(true, true);
  // req->request()->client_header()->AddField(std::string(whisper::http::kHeaderContentEncoding),
  //                                          FLAGS_accept_encoding, true, true);
  req->request()->client_header()->AddField("Host", url.host(), true, true);

  whisper::http::ClientStreamReceiverProtocol* stream = nullptr;
  whisper::ResultClosure<bool>* stream_callback = nullptr;
  if (FLAGS_stream_request) {
      stream = client->fsc()->CreateStreamReceiveClient(-1);
      stream_callback = whisper::NewPermanentCallback(
          &StreamNext, req,  client, stream, &selector);

      selector.RunInSelectLoop(
          whisper::NewCallback(
              stream, &whisper::http::ClientStreamReceiverProtocol::BeginStreamReceiving,
              req, stream_callback));

  } else {
      whisper::Closure* completion = whisper::NewCallback(
          &RequestDone, req, client, &selector);
      selector.RunInSelectLoop(
          whisper::NewCallback(client->fsc(),
                               &whisper::http::FailSafeClient::StartRequest,
                               req, completion));
  }
  selector.Loop();
}
