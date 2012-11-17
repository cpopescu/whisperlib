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

#include <stdio.h>
#include <whisperlib/io/checkpoint/state_keeper.h>
#include <whisperlib/base/gflags.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(dir,
              "",
              "The directory to analyze");
DEFINE_string(file_base,
              "",
              "StateKeeper file_base. If empty, use autodetect.");
DEFINE_int32(block_size,
             0,
             "StateKeeper blocks size. If 0, use autodetect.");
DEFINE_int32(blocks_per_file,
             0,
             "StateKeeper blocks per file. If 0, use autodetect.");

DEFINE_bool(print,
            true,
            "We print the content of the state(according to the prefix)");

DEFINE_string(print_prefix,
              "",
              "If true, when we print (--print is true), "
              "we print only the keys under this prefix");

DEFINE_bool(print_keys_only,
            false,
            "If true, when we print (--print is true), "
            "we print only the keys (no values)");

DEFINE_string(delete_prefix,
              "",
              "If specified, we delete keys under that prefix");

DEFINE_string(delete_key,
              "",
              "If specified, we delete that particular key");

DEFINE_string(add_key,
              "",
              "If specified, we add a key to the state, with "
              "value from --add_value");

DEFINE_string(add_value,
              "",
              "If specified w/ --add_key, this is the value for the key that"
              " we add");

DEFINE_string(add_value_escaped,
              "",
              "If specified w/ --add_key, this is the value for the key that"
              " we add (escaped w/ %%02x for non ascii)");

DEFINE_bool(checkpoint,
            false,
            "If true, we do force a state checkpoint at the very end");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  //////////////////////////////////////////////////////////////////////////
  // auto-detect part
  string file_base = FLAGS_file_base;
  int32 block_size = FLAGS_block_size;
  int32 blocks_per_file = FLAGS_blocks_per_file;
  if ( file_base == "" || block_size == 0 || blocks_per_file == 0 ) {
    string f;
    int32 bs, bpf;
    if ( !io::DetectStateKeeperSettings(FLAGS_dir, &f, &bs, &bpf) ) {
      LOG_ERROR << "DetectStateKeeperSettings failed for dir: ["
                << FLAGS_dir << "]";
      common::Exit(1);
    }
    if ( file_base == "" ) {
      file_base = f;
      LOG_INFO << "Detected file_base: [" << file_base << "]";
    }
    if ( block_size == 0 ) {
      block_size = bs;
      LOG_INFO << "Detected blocksize: " << block_size;
    }
    if ( blocks_per_file == 0 ) {
      blocks_per_file = bpf;
      LOG_INFO << "Detected blocks_per_file: " << blocks_per_file;
    }
  }
  if ( file_base == "" || block_size == 0 || blocks_per_file == 0 ) {
    LOG_ERROR << "Invalid settings, file_base: [" << file_base << "]"
                 ", block_size: " << block_size
              << ", blocks_per_file: " << blocks_per_file;
    common::Exit(1);
  }
  ::sleep(2);
  //////////////////////////////////////////////////////////////////////////

  // If there is no change requested, then avoid creating a StateKeeper
  // (because the StateKeeper needs write access to files, and the state may
  //  be in use by another process)

  if ( FLAGS_print ) {
    // just read the state from disk, without a StateKeeper
    map<string, string> data;
    CHECK(io::StateKeeper::ReadState(FLAGS_dir, file_base,
        &data, true, block_size, blocks_per_file))
           << "Failed to read state";

    map<string, string>::const_iterator begin;
    map<string, string>::const_iterator end;
    strutil::GetBounds(FLAGS_print_prefix, data, &begin, &end);
    uint32 key_count = 0;
    for ( map<string, string>::const_iterator it = begin; it != end; ++it ) {
      key_count++;
      if ( FLAGS_print_keys_only ) {
        LOG_INFO << "#" << key_count << " Key: " << it->first;
      } else {
        LOG_INFO << "#" << key_count << endl
                 << " - Key: " << it->first << endl
                 << " - Value: " << strutil::StrEscape(it->second, '%', "");
      }
    }
    LOG_INFO << "# " << key_count << " keys selected, out of a total of "
             << data.size();
  }

  if ( FLAGS_delete_prefix.empty() &&
       FLAGS_delete_key.empty() &&
       FLAGS_add_key.empty() &&
       FLAGS_checkpoint == false ) {
    common::Exit(0);
  }

  LOG_INFO << "Creating a StateKeeper..";
  io::StateKeeper sk(FLAGS_dir, file_base, block_size, blocks_per_file);
  if ( !sk.Initialize() ) {
    LOG_ERROR << "Failed to initialize StateKeeper";
    common::Exit(1);
  }

  if ( !FLAGS_delete_prefix.empty() ) {
    CHECK(sk.DeletePrefix(FLAGS_delete_prefix));
  }
  if ( !FLAGS_delete_key.empty() ) {
    CHECK(sk.DeleteValue(FLAGS_delete_key));
  }
  if ( !FLAGS_add_key.empty() ) {
    if ( FLAGS_add_value_escaped.empty() ) {
      CHECK(sk.SetValue(FLAGS_add_key, FLAGS_add_value));
    } else {
      CHECK(sk.SetValue(FLAGS_add_key,
                        strutil::StrUnescape(FLAGS_add_value_escaped, '%')));
    }
  }
  if ( FLAGS_checkpoint ) {
    CHECK(sk.Checkpoint());
  }
  common::Exit(0);
}
