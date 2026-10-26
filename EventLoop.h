#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "TimerId.h"
#include "Callbacks.h"

class Channel;
class Poller;
class TimerQueue;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();

    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行cb
    void runInLoop(Functor cb);

    // 把 cb 放入待执行队列，并唤醒 loop 所在线程去执行
    void queueInLoop(Functor cb);

    // 在指定时间点执行一次
    TimerId runAt(Timestamp time, TimerCallback cb);
    // 延迟delay秒后执行一次
    TimerId runAfter(double delay, TimerCallback cb);
    // 每隔interval秒执行一次，重复
    TimerId runEvery(double interval, TimerCallback cb);

    // 取消定时任务
    void cancel(TimerId timerId);

    // 用来唤醒loop所在的线程
    void wakeup();

    // 向 Poller 注册/更新 Channel
    void updateChannel(Channel *channel);
    // 从 Poller 中移除 Channel
    void removeChannel(Channel *channel);
    // 判断 Channel 是否已在 Poller 中
    bool hasChannel(Channel *channel);

    // 判断当前线程是否是 loop 所在的线程
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    void handleRead();        // wake up
    void doPendingFunctors(); 

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_; 
    std::atomic_bool quit_;  

    const pid_t threadId_; 

    Timestamp pollReturnTime_; 
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;

    int wakeupFd_; 
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; 
    std::vector<Functor> pendingFunctors_;    
    std::mutex mutex_;                       
};