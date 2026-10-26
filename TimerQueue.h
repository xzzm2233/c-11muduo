#pragma once

#include "Timestamp.h"
#include "noncopyable.h"
#include "Callbacks.h"
#include "Channel.h"

#include <set>
#include <vector>
#include <atomic>

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : noncopyable
{
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    // 添加定时任务
    TimerId addTimer(TimerCallback cb,
                     Timestamp when,
                     double interval);
    // 取消定时任务
    void cancel(TimerId timerId);

private:
    using Entry = std::pair<Timestamp, Timer *>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<Timer *, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    // 在 loop 线程中插入定时器
    void addTimerInLoop(Timer *timer);
    // 在 loop 线程中取消定时器
    void cancelInLoop(TimerId timerId);

    // 处理定时器描述符的可读事件
    void handleRead();

    // getExpired用来取出到期的 Entry
    std::vector<Entry> getExpired(Timestamp now);
    // 重置到期任务
    void reset(const std::vector<Entry> &expired, Timestamp now);

    // 插入定时器
    bool insert(Timer *timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_;

    TimerList timers_;
    ActiveTimerSet activeTimers_;
    ActiveTimerSet cancelingTimers_;

    // 表示是否正在执行回调
    std::atomic_bool callingExpiredTimers_;
};
