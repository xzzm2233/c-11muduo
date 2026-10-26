#include "TimerQueue.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"

#include <sys/timerfd.h>
#include <cstring>
#include <functional>
#include <unistd.h>

// 创建定时器描述符（非阻塞、close-on-exec）
int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_FATAL("Failed in timerfd_create");
    }
    return timerfd;
}

// 计算从当前时间到目标时间的剩余时间，至少 100 微秒
struct timespec howMuchTimeFormNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() -
                           Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

// 读取定时器描述符，消费其可读事件（记录超时次数）
void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_DEBUG("TimerQueue::handleRead() %llu at %s", (unsigned long long)(howmany), now.toString().c_str());
    if (n != sizeof howmany)
    {
        LOG_ERROR("TimerQueue::handleRead() reads %zd bytes instead of 8", n);
    }
}

// 重新设置定时器描述符的超时时间
void resetTimerfd(int timerfd, Timestamp expiration)
{
    struct itimerspec newValue;
    memset(&newValue, 0, sizeof newValue);
    newValue.it_value = howMuchTimeFormNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, nullptr);
    if (ret)
    {
        LOG_ERROR("timerfd_settime()");
    }
}

// 构造函数：创建定时器描述符和 Channel，并监听其可读事件
TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

// 析构函数：关闭定时器描述符并释放所有 Timer
TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);

    for (const Entry &timer : timers_)
    {
        delete timer.second;
    }
}

// 添加定时任务，转交 loop 线程执行，并返回 TimerId
TimerId TimerQueue::addTimer(TimerCallback cb,
                             Timestamp when,
                             double interval)
{
    Timer *timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

// 取消定时任务，转交 loop 线程执行
void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

// 在 loop 线程中插入定时器，若提前了最近到期时间则重置定时器描述符
void TimerQueue::addTimerInLoop(Timer *timer)
{
    bool earliestChanged = insert(timer);

    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

// 在 loop 线程中取消定时器
void TimerQueue::cancelInLoop(TimerId timerId)
{
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
        timers_.erase(Entry(it->first->expiration(), it->first));
        delete it->first;
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
}

// 定时器描述符可读：取出所有到期任务并执行，然后重置
void TimerQueue::handleRead()
{
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for (const Entry &it : expired)
    {
        it.second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}

// 取出所有已到期的定时任务
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        activeTimers_.erase(timer);
    }
    return expired;
}

// 处理到期任务：重复任务重新插入，否则释放，并重置定时器描述符
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire;

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it.second->restart(now);
            insert(it.second);
        }
        else
        {
            delete it.second;
        }
    }

    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}

// 把定时器插入主队列和辅助集合，返回是否改变了最早到期时间
bool TimerQueue::insert(Timer *timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();

    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }

    // 插入主队列
    timers_.insert(Entry(when, timer));

    // 插入辅助集合
    activeTimers_.insert(ActiveTimer(timer, timer->sequence()));

    return earliestChanged;
}
