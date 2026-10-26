#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include "TimerQueue.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d", errno);
    }
    return evtfd;
}

// 构造函数：创建 Poller、wakeup 事件描述符、定时器队列以及用于唤醒的 Channel
EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      timerQueue_(new TimerQueue(this)),
      wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    
    wakeupChannel_->enableReading();
}

// 析构函数：移除唤醒 Channel 并关闭 wakeup 描述符，清理线程局部变量
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping", this);

    while (!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd  client的fd && wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            channel->handleEvent(pollReturnTime_);
        }

        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping", this);
    looping_ = false;
}

// 退出事件循环：置位退出标志，若在其它线程调用则唤醒本线程以尽快退出
void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) 
    {
        cb();
    }
    else 
    {
        queueInLoop(cb);
    }
}

// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); 
    }
}

// wakeup 描述符可读，读取其中数据以清空可读事件
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", n);
    }
}

// 用来唤醒loop所在线程
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %zd bytes instead of 8", n);
    }
}

// EventLoop的方法 => Poller的方法
// 向 Poller 注册/更新 Channel 关心的事件
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

// 从 Poller 中移除 Channel
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

// 判断 Channel 是否已在 Poller 中
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() // 执行回调
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}

// 在指定时间点执行一次定时任务
TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

// 延迟 delay 秒后执行一次定时任务
TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

// 每隔 interval 秒重复执行定时任务
TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

// 取消指定定时任务
void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}