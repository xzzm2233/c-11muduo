#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseloop, const std::string &nameArg);
    ~EventLoopThreadPool();

    // 设置线程数量
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    // 启动线程池
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 轮询获取下一个 EventLoop
    EventLoop *getNextLoop();

    // 获取所有 EventLoop
    std::vector<EventLoop *> getAllLoops();

    // 是否已启动
    bool started() const { return started_; }
    // 返回线程池名字
    const std::string name() const { return name_; }

private:
    EventLoop *baseLoop_; // EventLoop loop;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;
};