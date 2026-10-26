#include "Poller.h"
#include "EPollPoller.h"
#include "PollPoller.h"

#include <stdlib.h>

// 根据环境变量 MUDUO_USE_POLL 决定使用 epoll 还是 poll 实现
Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return new PollPoller(loop); 
    }
    else
    {
        return new EPollPoller(loop); 
    }
}