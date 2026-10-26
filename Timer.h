#pragma once

#include "noncopyable.h"
#include "Callbacks.h"
#include "Timestamp.h"

#include <atomic>
#include <stdint.h>

class Timer : noncopyable
{
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(++s_numCreated_)
    {

    }

    // 执行定时任务回调
    void run() const
    {
        callback_();
    }

    // 返回到期时间
    Timestamp expiration() const {return expiration_;}
    // 是否重复任务
    bool repeat() const {return repeat_;}
    // 返回唯一序号
    int64_t sequence() const {return sequence_;}

    // 重新设定到期时间
    void restart(Timestamp now);

    // 返回已创建的 Timer 数量
    static int64_t numCreated() {return s_numCreated_.load();}

private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_; // 任务重复间隔
    const bool repeat_;     // 任务是否重复
    const int64_t sequence_; // Timer的标识符 

    static std::atomic_int64_t s_numCreated_; 
};