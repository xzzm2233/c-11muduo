#pragma once

#include "FixedBuffer.h"
#include "noncopyable.h"
#include "Thread.h"

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sys/types.h>

class AsyncLogging : noncopyable
{
public:
    AsyncLogging(const std::string &basename,
                 off_t rollSize,
                 int flushInterval = 3);

    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    // 写入一条日志（前端线程调用）
    void append(const char *logline, int len);

    // 启动日志线程
    void start()
    {
        running_ = true;
        thread_.start();
    }

    // 停止日志线程
    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    // 后端线程执行体
    void threadFunc();

    using Buffer = LargeBuffer;
    using BufferPtr = std::unique_ptr<Buffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;

    const int flushInterval_;
    std::atomic_bool running_;
    const std::string basename_;
    const off_t rollSize_;

    Thread thread_;

    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};