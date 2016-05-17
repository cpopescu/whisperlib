// Copyright (c) 2009, Whispersoft s.r.l.
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
// * Neither the name of Whispersoft s.r.l. nor the names of its
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
// Author: Catalin Popescu

// Simple test for an http server - we serve files under a path - nothing
// special :)


#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/system.h"
#include "whisperlib/base/gflags.h"
#include "whisperlib/io/file/file.h"
#include "whisperlib/io/file/file_input_stream.h"
#include "whisperlib/io/ioutil.h"
#include "whisperlib/sync/event.h"
#include "whisperlib/sync/mutex.h"

#include "whisperlib/http/http_server_protocol.h"
#include "whisperlib/net/selector.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(port,
             8081,
             "Serve on this port");
DEFINE_string(base_dir,
              "",
              "We serve from this base directory");

//////////////////////////////////////////////////////////////////////
//
// Utilities:
//
using namespace whisper;

string GenerateDirListing(const string& url_path) {
  const string dirpath(FLAGS_base_dir + url_path);
  vector<string> paths;
  if ( !io::DirList(dirpath, io::LIST_FILES | io::LIST_DIRS, NULL, &paths) ) {
    return "Error";
  }
  string s = strutil::StringPrintf("<h1>Directory: %s</h1>", url_path.c_str());
  for ( int i = 0; i < paths.size(); i++ ) {
    const string& crt_path = paths[i];
    string crt_url = strutil::JoinPaths(url_path, crt_path);
    if ( io::IsDir(crt_path) ) {
      s += strutil::StringPrintf("<p>[DIR] <a href=\"%s/\">%s/</a></p>",
                                 crt_url.c_str(),
                                 crt_path.c_str());
    } else {
      s += strutil::StringPrintf("<p><a href=\"%s\">%s</a></p>",
                                 crt_url.c_str(),
                                 crt_path.c_str());
    }
  }
  return s;
}

bool LooksTextFile(io::File* f) {
  bool is_text = true;
  // This is not very smart, but whatever, is just a test.
  const int32 text_to_check = std::min(static_cast<int32>(f->Size()), 1024);
  uint8* buf = new uint8[text_to_check];
  if ( f->Read(buf, text_to_check) != text_to_check ) {
    f->SetPosition(0);
    return false;
  }
  f->SetPosition(0);
  for ( int i = 0; i < text_to_check; i++ ) {
    if ( buf[i] < 1 || buf[i] > 127 ) {
      is_text = false;
      break;
    }
  }
  delete []buf;
  return is_text;
}

string ContentTypeFromFilename(const string& path, bool is_text) {
  string extension;
  const int pos_dot = path.rfind(".");
  const int pos_slash = path.rfind("/");
  if ( pos_dot != string::npos &&
       pos_dot > pos_slash ) {
    extension = path.substr(pos_dot + 1);
  }
  strutil::StrToLower(extension);
  string content_type = "application/octet-stream";
  if ( extension == "html" || extension == "htm" ) {
    content_type = "text/html";
  } else if ( extension == "jpg" ||
              extension == "jpeg" ) {
    content_type = "image/jpeg";
  } else if ( extension == "gif" ) {
    content_type = "image/gif";
  } else if ( extension == "png" ) {
    content_type = "image/png";
  } else if  ( is_text ) {
    content_type = "text/plain";
  }
  return content_type;
}

//////////////////////////////////////////////////////////////////////
//
// Response Handling
//

// One shot answer
void ReadFileToRequest(io::FileInputStream* fis,
                         http::ServerRequest* req,
                         const string& content_type) {
  string s;
  const int64 size = fis->Readable();
  CHECK_EQ(size, fis->ReadString(&s, size));
  req->request()->server_header()->AddField(
    http::kHeaderContentType, content_type, true);
  req->request()->server_data()->Write(s);
  req->Reply();
}

struct StreamData {
  io::FileInputStream* fis;
  http::ServerRequest* req;
  ~StreamData() {
    delete fis;
  }
};

// Streaming answer

void StreamingClosed(StreamData* sd) {
  sd->req->net_selector()->DeleteInSelectLoop(sd);
}

void ContinueStreaming(StreamData* sd) {
  int32 size, read_size;
  char* buf;

  size = read_size = sd->req->free_output_bytes();
  if ( sd->req->is_orphaned() ) {
    goto CloseReq;
  } else {
    buf = NULL;
    if ( size > 0 ) {
      buf = new char[size];
      read_size = sd->fis->Read(buf, size);
      if ( read_size <= 0 ) {
        delete [] buf;
        // TODO(cpopescu): Report an error !
        // if ( read_size < 0 ) {
        // }
        goto CloseReq;
      }
      // buf is taken under the io::MemoryStream
      sd->req->request()->server_data()->AppendRaw(buf, read_size);
    }
    sd->req->set_ready_callback(NewCallback(&ContinueStreaming, sd), 32000);
    sd->req->ContinueStreamingData();
  }
  return;
CloseReq:
  LOG_INFO << " Closing sd: " << sd;
  sd->req->EndStreamingData();   // Streaming Closed gets called..
}

void StreamFileToRequest(StreamData* sd) {
  sd->req->set_ready_callback(NewCallback(&ContinueStreaming, sd), 32000);
  sd->req->BeginStreamingData(http::OK, NULL,
                              NewCallback(&StreamingClosed, sd));
  ContinueStreaming(sd);
}

//////////////////////////////////////////////////////////////////////

// The actual processor - we run in a different thread !!

void ProcessPath(http::ServerRequest* req) {
  URL* const url = req->request()->url();
  const string url_path(url->UrlUnescape(url->path().c_str(),
                                         url->path().size()));
  CHECK(url != NULL);
  const string path(FLAGS_base_dir + url_path);
  if ( io::IsDir(path.c_str()) ) {
    LOG_INFO << " Directory request : " << url_path
             << " [" << path << "]";
    req->request()->server_data()->Write(GenerateDirListing(url_path));
    req->Reply();
  } else {
    io::File* infile = new io::File();
    // This is actually safe, as the URL lib takes care of bad ../ and other
    // nastyness.
    if ( !infile->Open(path.c_str(),
                       io::File::GENERIC_READ, io::File::OPEN_EXISTING) ) {
      req->request()->server_data()->Write(
        strutil::StringPrintf("<h1>404 - Path not found: %s</h1>",
                              path.c_str()));
      delete infile;
      req->ReplyWithStatus(http::NOT_FOUND);
      return;
    }
    const string content_type(
      ContentTypeFromFilename(path, LooksTextFile(infile)));
    io::FileInputStream* fis = new io::FileInputStream(infile);
    const int64 size = fis->Readable();
    if ( size > req->free_output_bytes() ) {
      LOG_INFO << " Streaming file request : " << url_path
               << " [" << path << "]";
      req->request()->server_header()->AddField(
          http::kHeaderContentType, content_type, true);
      StreamData* sd = new StreamData;
      sd->fis = fis;
      sd->req = req;
      StreamFileToRequest(sd);
    } else {
      LOG_INFO << " One read file request : " << url_path
               << " [" << path << "]";
      ReadFileToRequest(fis, req, content_type);
      delete fis;
    }
  }
}

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_base_dir.empty());

  http::ServerParams params;
  params.max_reply_buffer_size_ = 1<<18;
  params.dlog_level_ = true;
  net::Selector selector;

  vector<net::SelectorThread*> client_threads;
  for ( int i = 0; i < 10; ++i ) {
    client_threads.push_back(new net::SelectorThread());
    client_threads.back()->Start();
  }

  net::NetFactory net_factory(&selector);

  net::TcpConnectionParams tcp_connection_params;
  net::TcpAcceptorParams tcp_acceptor_params(tcp_connection_params);
  tcp_acceptor_params.set_client_threads(&client_threads);
  net_factory.SetTcpParams(tcp_acceptor_params,
                           tcp_connection_params);

  http::Server server("Test Server 1", &selector, net_factory, params);
  server.RegisterProcessor("", NewPermanentCallback(&ProcessPath), true, true);
  server.AddAcceptor(net::PROTOCOL_TCP, net::HostPort(0, FLAGS_port));
  selector.RunInSelectLoop(NewCallback(&server,
                                       &http::Server::StartServing));
  selector.Loop();
}
