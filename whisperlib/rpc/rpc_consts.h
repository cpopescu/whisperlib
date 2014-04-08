// (c) Copyright 2011, 1618labs
// All rights reserved.
// Author: Catalin Popescu (cp@1618labs.com)
//

#ifndef __RPC_RPC_CONSTS_H__
#define __RPC_RPC_CONSTS_H__

namespace rpc {

static const char kRpcHttpXid[] = "X-Xid";
static const char kRpcErrorCode[] = "X-Rpc-Error";
static const char kRpcErrorReason[] = "X-Rpc-Error";
static const char kRpcContentType[] = "application/protobuf";
static const char kRpcErrorContentType[] = "text/plain";
static const char kRpcGzipEncoding[] = "gzip";

static const char kRpcErrorBadEncoded[] = "Badly encoded data";

static const char kRpcErrorMethodNotSupported[] = "Method not supported";
static const char kRpcErrorServerOverloaded[] = "Server overloaded";
static const char kRpcErrorUnauthorizedIP[] = "Unauthorized IP";
static const char kRpcErrorUnauthorizedUser[] = "Unauthorized user";
static const char kRpcErrorServiceNotFound[] = "Service not found";
static const char kRpcErrorMethodNotFound[] = "Method not found";
static const char kRpcErrorSerializingResponse[] = "Error serializing the response";
static const char kRpcErrorBadProxyHeader[] = "Missing or invalid proxy headers";
}

#endif  // __RPC_RPC_CONSTS_H__
