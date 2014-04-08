// Copyright: 1618labs, Inc. 2013 onwards.
// All rights reserved.
// cp@1618labs.com
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
//

#ifndef __WHISPERLIB_IO_FILE_READER_H__
#define __WHISPERLIB_IO_FILE_READER_H__

#include <whisperlib/base/types.h>
#include <whisperlib/base/strutil.h>
#include <whisperlib/io/file/file_input_stream.h>

namespace io {
class FileReader {
public:
    FileReader() {
    }
    virtual ~FileReader() {
    }
    virtual bool ReadToString(const string& path, string* s) const = 0;
};

class SimpleFileReader : public FileReader {
public:
    SimpleFileReader(const string& dir)
    : dir_(dir) {
    }
    virtual ~SimpleFileReader() {
    }
    virtual bool ReadToString(const string& path, string* s) const {
        const string filename(strutil::JoinPaths(dir_, path));
        return io::FileInputStream::TryReadFile(filename, s);
    }
private:
    const string dir_;
};
}

#endif
