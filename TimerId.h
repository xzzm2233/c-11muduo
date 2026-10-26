#pragma once

#include <stdint.h>

class Timer;

// 定时器的唯一标识：由 Timer 指针和序号组成
class TimerId
{
public:
    // 默认构造，表示无效的 TimerId
    TimerId()
        : timer_(nullptr),
          sequence_(0)
    {
    }

    // 用 Timer 指针和序号构造
    TimerId(Timer *timer, int64_t sequence)
        : timer_(timer),
          sequence_(sequence)
    {
    }

    friend class TimerQueue;

private:
    Timer *timer_;
    int64_t sequence_;
};