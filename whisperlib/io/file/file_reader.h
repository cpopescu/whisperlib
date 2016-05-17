// Copyright: Urban Engines, Inc. 2011 onwards.
// All rights reserved.
// cp@urbanengines.com

#ifndef __WHISPERLIB_IO_FILE_READER_H__
#define __WHISPERLIB_IO_FILE_READER_H__

#include "whisperlib/base/types.h"
#include "whisperlib/base/strutil.h"
#include "whisperlib/io/file/file_input_stream.h"

namespace whisper {
namespace io {
class FileReader {
public:
    FileReader() {
    }
    virtual ~FileReader() {
    }
    virtual bool ReadToString(const std::string& path, std::string* s) const = 0;
};

class SimpleFileReader : public FileReader {
public:
    SimpleFileReader(const std::string& dir)
    : dir_(dir) {
    }
    virtual ~SimpleFileReader() {
    }
    virtual bool ReadToString(const std::string& path, std::string* s) const {
        const std::string filename(strutil::JoinPaths(dir_, path));
        return io::FileInputStream::TryReadFile(filename, s);
    }
private:
    const std::string dir_;
};
}  // namespace io
}  // namespace whisper

#endif
