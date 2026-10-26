#pragma once

#include "noncopyable.h"

#include <cstring>

/**
 * data_: [  已写入的数据  ][  剩余空间  ]
          ↑               ↑            ↑
         data_           cur_      data_+SIZE
 */
template <int SIZE>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer()
        : cur_(data_)
    {
    }

    // 向缓冲区追加数据（空间不足则忽略）
    void append(const char *buf, size_t len)
    {
        if (avail() > static_cast<int>(len))
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    // 返回数组起始地址
    const char *data() const { return data_; }
    // 返回已写入的数据长度
    int length() const { return static_cast<int>(cur_ - data_); }
    // 返回当前写指针
    char *current() { return cur_; }

    // 返回剩余可用空间
    int avail() const { return static_cast<int>(end() - cur_); }
    // 写指针前移 len 字节
    void add(size_t len) { cur_ += len; }

    // 重置写指针到起始位置
    void reset() { cur_ = data_; }
    // 清空缓冲区内容
    void bzero() { memset(data_, 0, sizeof data_); }

private:
    const char *end() const { return data_ + sizeof data_; }

    char data_[SIZE];

    // 指向下一个要写入的位置
    char *cur_;
};

const int kLargeBuffer = 4000 * 1000;

using LargeBuffer = FixedBuffer<kLargeBuffer>;