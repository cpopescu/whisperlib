// (c) Copyright 2011, Urban Engines
// All rights reserved.
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
