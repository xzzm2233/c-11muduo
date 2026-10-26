#include "Timer.h"
#include "Timestamp.h"

std::atomic_int64_t Timer::s_numCreated_{0};

// 重新设定到期时间：重复任务则基于当前时间加间隔，否则置为无效时间
void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp();
    }
}