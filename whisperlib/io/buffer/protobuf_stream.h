// Copyright (c) 2012, 1618labs
// All rights reserved.
//
// Author: Catalin Popescu
//
#ifndef __WHISPERLIB_IO_PROTOBUF_STREAM_H__
#define __WHISPERLIB_IO_PROTOBUF_STREAM_H__

//
// Wrappers for io::MemoryStream to expose a google::protobuf::io::ZeroCopyXXXStream
// interface.
//
#include <whisperlib/io/buffer/memory_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/message_lite.h>

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
    explicit ProtobufReadStream(MemoryStream* stream)
        : stream_(stream),
          position_(0) {
        stream_->MarkerSet();
    }
    virtual ~ProtobufReadStream() {
        stream_->MarkerClear();
    }
    virtual bool Next(const void** data, int* size) {
        *size = 0;
        const bool ret =  stream_->ReadNext(reinterpret_cast<const char**>(data), size);
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
        const int skipped = stream_->Skip(count);
        position_ += skipped;
        return skipped == count;
    }
    virtual int64 ByteCount() const {
        return position_;
    }

private:
    MemoryStream* const stream_;
    int position_;

    DISALLOW_EVIL_CONSTRUCTORS(ProtobufReadStream);
};

inline bool ParseProto(google::protobuf::MessageLite* message,
                       io::MemoryStream* ms) {
    io::ProtobufReadStream stream(ms);
    return message->ParseFromZeroCopyStream(&stream);
}
inline bool ParsePartialProto(google::protobuf::MessageLite* message,
                              io::MemoryStream* ms) {
    io::ProtobufReadStream stream(ms);
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

    virtual int64 ByteCount() const {
        return position_;
    }
 private:
    MemoryStream* stream_;
    int32 last_scratch_size_;
    int64 position_;

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
}

#endif  // __WHISPERLIB_IO_PROTOBUF_STREAM_H__
