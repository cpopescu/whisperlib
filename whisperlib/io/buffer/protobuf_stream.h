// Copyright (c) 2012, Urban Engines
// All rights reserved.
//
// Author: Catalin Popescu
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
#ifndef __WHISPERLIB_IO_PROTOBUF_STREAM_H__
#define __WHISPERLIB_IO_PROTOBUF_STREAM_H__

//
// Wrappers for io::MemoryStream to expose a google::protobuf::io::ZeroCopyXXXStream
// interface.
//
#include "whisperlib/io/buffer/memory_stream.h"
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/message_lite.h>

namespace whisper {
namespace io {

//////////////////////////////////////////////////////////////////////
//
// Zero copy Read stream for protobuf usage  based on io::MemoryStream
//
// io::MemoryStream* ms = ...
// google::protobuf::MessageLite* message = ...
// ...
// {
//     io::ProtobufReadStream stream(ms);
//     message->ParseFromZeroCopyStream(&stream);
// }   // At this point the full message is passed over in ms.
//
// or simply use the io::ParseProto helper
//

class ProtobufReadStream : public google::protobuf::io::ZeroCopyInputStream {
public:
    explicit ProtobufReadStream(MemoryStream* stream, int limit = 0)
        : stream_(stream),
          position_(0),
          limit_(limit ? position_ + limit : 0) {
        stream_->MarkerSet();
    }
    virtual ~ProtobufReadStream() {
        stream_->MarkerClear();
    }
    virtual bool Next(const void** data, int* size) {
        *size = 0;
        if (limit_) {
            if (limit_ <= position_) {
                return false;
            }
            *size = limit_ - position_;
        }
        size_t cb = *size;
        const bool ret = stream_->ReadNext(reinterpret_cast<const char**>(data), &cb);
        *size = cb;
        position_ += *size;
        return ret;
    }
    virtual void BackUp(int count) {
        CHECK_LE(count, position_);
        stream_->MarkerRestore();
        stream_->MarkerSet();
        position_ -= count;
        stream_->Skip(position_);
    }
    virtual bool Skip(int count) {
        int to_skip = count;
        if (limit_ && to_skip > (limit_ - position_)) {
            to_skip = (limit_ - position_);
        }
        const int skipped = stream_->Skip(to_skip);
        position_ += skipped;
        return skipped == count;
    }
    virtual google::protobuf::int64 ByteCount() const {
        return position_;
    }

private:
    MemoryStream* const stream_;
    int position_;
    const int limit_;

    DISALLOW_EVIL_CONSTRUCTORS(ProtobufReadStream);
};

inline bool ParseProto(google::protobuf::MessageLite* message,
                       io::MemoryStream* ms, int limit = 0) {
    io::ProtobufReadStream stream(ms, limit);
    return message->ParseFromZeroCopyStream(&stream);
}
inline bool ParsePartialProto(google::protobuf::MessageLite* message,
                              io::MemoryStream* ms, int limit = 0) {
    io::ProtobufReadStream stream(ms, limit);
    return message->ParsePartialFromZeroCopyStream(&stream);
}

//////////////////////////////////////////////////////////////////////
//
// Zero copy Write stream for protobuf usage  based on io::MemoryStream
//
// Modus operandi:
//
// io::MemoryStream* ms = ...
// google::protobuf::MessageLite* message = ...
// ...
// {
//     io::ProtobufWriteStream stream(ms);
//     message->SerializeToZeroCopyStream(&stream);
// }   // At this point the full message written over in ms (after delete).
//
// Or simply use io::SerializeProto
//
class ProtobufWriteStream : public google::protobuf::io::ZeroCopyOutputStream {
public:
    explicit ProtobufWriteStream(MemoryStream* stream)
    : stream_(stream),
      last_scratch_size_(0),
      position_(0) {
    }
    ~ProtobufWriteStream() {
        if (last_scratch_size_ > 0) {
            stream_->ConfirmScratch(last_scratch_size_);
        }
    }

    virtual bool Next(void** data, int* size) {
        if (last_scratch_size_ > 0) {
            stream_->ConfirmScratch(last_scratch_size_);
        }
        stream_->GetScratchSpace(reinterpret_cast<char**>(data), &last_scratch_size_);
        *size = last_scratch_size_;
        position_ += *size;
        return true;
    }

    virtual void BackUp(int count) {
        CHECK_LE(count, last_scratch_size_);
        position_ -= count;
        stream_->ConfirmScratch(last_scratch_size_ - count);
        last_scratch_size_ = 0;
    }

    virtual google::protobuf::int64 ByteCount() const {
        return position_;
    }
 private:
    MemoryStream* stream_;
    size_t last_scratch_size_;
    int64_t position_;

    DISALLOW_EVIL_CONSTRUCTORS(ProtobufWriteStream);
};

inline bool SerializeProto(const google::protobuf::MessageLite* message,
                           io::MemoryStream* ms) {
    io::ProtobufWriteStream stream(ms);
    return message->SerializeToZeroCopyStream(&stream);
}
inline bool SerializePartialProto(const google::protobuf::MessageLite* message,
                                  io::MemoryStream* ms) {
    io::ProtobufWriteStream stream(ms);
    return message->SerializePartialToZeroCopyStream(&stream);
}
}  // namespace io
}  // namespace whisper

#endif  // __WHISPERLIB_IO_PROTOBUF_STREAM_H__
