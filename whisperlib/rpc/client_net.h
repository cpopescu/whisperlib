// Copyright: Urban Engines, Inc. 2012 onwards.
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
// * Neither the name of Urban Engines inc nor the names of its
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
// cp@urbanengines.com
//

#ifndef __WHISPERLIB_RPC_CLIENT_NET_H__
#define __WHISPERLIB_RPC_CLIENT_NET_H__
#include "whisperlib/base/types.h"

#include "whisperlib/net/selector.h"
#include "whisperlib/net/address.h"
#include "whisperlib/net/connection.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#else
struct SSL_CTX {};
#endif

namespace whisper {

namespace http {
struct ClientParams;
class FailSafeClient;
class BaseClientConnection;
}

namespace rpc {

/** Class that wraps the common network stuff for creating
 * an rpc client for general purpose
 */
class ClientNet {
public:
    /** Multi server initialization - we can choose to talk to
     * any of the provided servers when performing the connections.
     * @param selector - runs the network select loop for these clients
     * @param params - parameters for the http connection. To issues
     *    multiple concurrent requests (only to whisperlib based servers
     *    as it is not a proper http features), set max_concurrent_requests
     * @param servers - servers to connect to. Use ToServerArray to build some.
     * @param host_header - pass this Host in the http header request.
     * @param num_retries - run these many retries for a request before giving up.
     *    Use -1 to never give up and always retry. Plese note that in this case
     *    one request can block the head of line.
     * @param request_timeout_ms - consider a request failed after these
     *    many miliseconds.
     * @param reopen_connection_interval_ms - after a failed connection, wait
     *    this long to reopen the connection to a server.
     * @param ssl_context - initialized ssl context for the connection.
     *    When non null, an https connection is established to the servers.
     *    We take control of the ssl context, but the app is requered to
     *    call SSL_library_init().
     */
    ClientNet(net::Selector* selector,
              const http::ClientParams* params,
              const std::vector<net::HostPort>& servers_hostport,
              const char* host_header = NULL,
              int num_retries = 5,
              int32 request_timeout_ms = 40000,
              int32 reopen_connection_interval_ms = 5000,
              SSL_CTX* ssl_context = NULL);
    ~ClientNet();

    /** Access the underlaying fail safe client connection to initialize
     * rpc clients and such.
     */
    http::FailSafeClient* fsc() const {
        return fsc_;
    }

    /** Utility to convert a server and a number of redundant connections
     * to it to a vector<net::HostPort> to pass to the constructor.
     */
    static std::vector<net::HostPort> ToServerVec(const std::string& server_name,
                                                  size_t num_connections = 1);

    /** Utility for creating an ssl context loaded w/ goodies.
     *  @param pem_text - if nonempty, the text of the public key (pem format)
     *  @param filename - set to load the verification PEM certificates from
     *     this multi-PEM file.
     *  @param dirname - set to load the verification PEM certificates from
     *     this dir (one per file). See SSL_CTX_load_verify_locations for details.
     *  @param error - returns the error text for the operation.
     *  @return - the newly created / initialized ssl context, or NULL on error.
     *     the caller has to SSL_CTX_free this.
     */
    static SSL_CTX* CreateSslContext(const std::string& pem_text,
                                     const std::string& filename,
                                     const std::string& dirname,
                                     std::string* error);
    /** Utility for creating a client context accepting the
     * provided pem certificate list.
     *
     * Use utl/ue_chain_crt.h for the public Urban Engines server
     *  certificates to verify a UE server.
     */
    static SSL_CTX* CreateSslContextFromText(const std::string& contents);
private:
    net::Selector* const selector_;
    SSL_CTX* const ssl_context_;
    net::NetFactory* net_factory_;
    http::FailSafeClient* const fsc_;
};

}  // namespace rpc
}  // namespace http

#endif  // __WHISPERLIB_RPC_CLIENT_NET_H__
