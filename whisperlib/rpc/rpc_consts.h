// (c) Copyright 2011, Urban Engines
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
// Author: Catalin Popescu (cp@urbanengines.com)
//
#ifndef __RPC_RPC_CONSTS_H__
#define __RPC_RPC_CONSTS_H__

namespace whisper {
namespace rpc {

static const char kRpcHttpXid[] = "X-Xid";
static const char kRpcErrorCode[] = "X-Rpc-Error";
static const char kRpcErrorReason[] = "X-Rpc-Reason";
static const char kRpcHttpIsStreaming[] = "X-Rpc-Streaming";
static const char kRpcHttpHeartBeat[] = "X-Rpc-Heart-Beat";
static const char kRpcContentType[] = "application/x-protobuf";
static const char kRpcErrorContentType[] = "text/plain";
static const char kRpcGzipEncoding[] = "gzip";

static const int  kRpcMinHeartBeat = 3;
static const int  kRpcDefaultHeartBeat = 20;

static const char kRpcErrorBadEncoded[] = "Badly encoded data";

static const char kRpcErrorMethodNotSupported[] = "Method not supported";
static const char kRpcErrorServerOverloaded[] = "Server overloaded";
static const char kRpcErrorUnauthorizedIP[] = "Unauthorized IP";
static const char kRpcErrorUnauthorizedUser[] = "Unauthorized user";
static const char kRpcErrorServiceNotFound[] = "Service not found";
static const char kRpcErrorMethodNotFound[] = "Method not found";
static const char kRpcErrorSerializingResponse[] = "Error serializing the response";
static const char kRpcErrorBadProxyHeader[] = "Missing or invalid proxy headers";
}  // namespace rpc
}  // namespace whisper

#endif  // __RPC_RPC_CONSTS_H__
