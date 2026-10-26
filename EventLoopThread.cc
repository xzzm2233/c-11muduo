#include "EventLoopThread.h"
#include "EventLoop.h"

#include <memory>

// 构造函数：保存线程初始化回调和名字，创建底层线程
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb)
{
}

// 析构函数：退出循环并等待线程结束
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

// 启动线程并等待其创建好 EventLoop 后返回该 loop
EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层的新线程

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 线程执行体：创建 EventLoop，执行初始化回调后进入事件循环
void EventLoopThread::threadFunc()
{
    EventLoop loop; 
    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop =>Poller.poll
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}