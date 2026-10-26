#pragma once

#include <vector>
#include <stddef.h>
#include <string>
#include <algorithm>

/*
    [  prependable  ] [     readable      ] [    writable    ]
    0          readerIndex_            writerIndex_         buffer.size()
*/
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
    }

    // 可读数据的字节数
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    // 可写空间的字节数
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    // 头部预留空间的字节数
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回可读数据的起始地址
    const char *peek() const
    {
        return begin() + readerIndex_;
    }

    // 丢弃 len 字节已读数据
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else // len == readableBytes()
        {
            retrieveAll();
        }
    }

    // 清空缓冲区，重置读写指针
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 应用可读取的数据长度
    }

    // 读取 len 字节并转成 string 返回
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); 
        return result;
    }

    // 确保可写空间至少有 len 字节
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); 
        }
    }

    // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    // 返回可写区域的起始地址
    char *beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char *beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);

    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    // 返回缓冲区起始地址
    char *begin()
    {
        // it.operator*()
        return &*buffer_.begin();
    }

    const char *begin() const
    {
        return &*buffer_.begin();
    }

    // 扩容或整理内部空间，腾出至少 len 字节可写空间
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};