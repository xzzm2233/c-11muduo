#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();

    // 启动线程并返回其 EventLoop
    EventLoop *startLoop();

private:
    // 线程执行体
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};
