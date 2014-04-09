Date: 2014-04-09 15:44:36

A simple web server
==================

```
#include <whisperlib/base/types.h>
#include <whisperlib/base/system.h>
#include <whisperlib/base/log.h>
#include <whisperlib/base/gflags.h>
#include <whisperlib/net/selector.h>
#include <whisperlib/http/http_server_protocol.h>

DEFINE_int32(port, 8080, "Port on which to run the server");

void ProcessPath(http::ServerRequest* req) {
  URL* const url = req->request()->url();
  LOG(INFO) << "Got request at " << url->path();
  req->request()->server_header()->AddField(
      http::kHeaderContentType, "text/plain", true);
  req->request()->server_data()->Write("Hello world!");
  req->Reply();
}

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = 1;
  common::Init(argc, argv);

  net::Selector selector;
  net::NetFactory net_factory(&selector);
  http::ServerParams params;
  params.dlog_level_ = true;
  http::Server server("test", &selector, net_factory, params);

  server.RegisterProcessor("", NewPermanentCallback(&ProcessPath), true, true);
  server.AddAcceptor(net::PROTOCOL_TCP, net::HostPort(0, FLAGS_port));
  LOG(INFO) << "Starting HTTP server on port " << FLAGS_port << "...";
  selector.RunInSelectLoop(NewCallback(&server,
                                       &http::Server::StartServing));
  selector.Loop();
}
```

For a more elaborate example, check out
whisperlib/http/test/http_server_test.cc
