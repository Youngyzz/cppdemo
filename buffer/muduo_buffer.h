#ifndef MUDUO_BUFFER_H
#define MUDUO_BUFFER_H
#include <cppdemo/copyable/muduo_copyable.h>
#include <assert.h>

namespace muduo {
/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer : public muduo::copyable 
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0 );
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    } 

    /// implicit copy-ctor, move-ctor, dtor and assignment are fine.
    /// NOTE: implicit move-ctor is added in g++ 4.6

    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    size_t readableBytes() const 
    { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const 
    { return buffer_.size() - writerIndex_; }

    size_t prependableBytes() const 
    { return readerIndex_; }

    const char* peek() const 
    { return begin() + readerIndex_; }
    ///
    /// Peek int32_t from host endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    int32_t peekInt32() const
    {
        assert(readableBytes() >= sizeof int32_t);
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof be32);
        return be32;
    }

    int16_t peekInt16() const 
    {
        assert(readableBytes() >= sizeof int16_t);
        int16_t be16 = 0;
        ::memcpy(&be16, peek(), sizeof be16);
        return be16;
    }

    int8_t peekInt8() const 
    {
        assert(readableBytes() >= sizeof int8_t);
        int8_t x = *peek();
        return x;
    }

    /// Prepend int32_t using host endian
    void prependInt32(int32_t x)
    {
        prepend(&be32, sizeof be32);
    }

    void prependInt16(int16_t x)
    {
        prepend(&be16, sizeof be16);
    }

    void prependInt8(int8_t x)
    {
        prepend(&x, sizeof x);
    }

    void prepend(const void* data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }

    void shrink(size_t reserve)
    {
        // FIXME: use vector:shrink_to_fit() in C++ 11 if possible
        Buffer other;
        other.ensureWritableBytes(readableBytes() + reserve);
        other.append(peek(),)
        swap(other);
    }

    // retrieve returns void, to prevent 
    void retrieve(size_t len)
    {
        assert(len <= readableBytes())
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else 
        {
            retrieveAll();
        }
    }

    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveInt64() 
    {
        retrieve(sizeof int64_t);
    }

    void retrieveInt32()
    {
        retrieve(sizeof int32_t);
    }

    void retrieveInt16()
    {
        retrieve(sizeof int16_t);
    }

    void retrieveInt8()
    {
        retrieve(sizeof int8_t);
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        string result(peek(), len);
        retrieve(len);
        return result;
    }

    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        hasWritten(len);
    }

    void append(const void* data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    ///
    /// Append int64_t using network endian
    ///
    void appendInt64(int64_t x)
    {
        append(&x, sizeof x);
    }

    ///
    /// Append int32_t using network endian
    ///
    void appendInt32(int32_t x)
    {
        append(&x, sizeof x);
    }

    void appendInt16(int16_t x)
    {
        append(&x, sizeof x);
    }

    void appendInt8(int8_t x)
    {
        append(&x, sizeof x);
    }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    char* beginWrite() 
    { return begin() + writerIndex_; }

    const char* beginWrite() const 
    { return begin() + writerIndex_; }

    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

private:
    char *begin()
    { return &*buffer_.begin(); }

    const char* begin() const 
    { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            //resize has already move readable data?
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            // move readable data to the front, make space inside buffer
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin()+readerIndex_, 
                      begin()+writerIndex_，
                      begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;  
            assert(readable == readableBytes());
        }
    }

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

};// end class
}// end namespace muduo
#endif