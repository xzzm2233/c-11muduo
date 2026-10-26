#pragma once
#include <string>
#include <stdint.h>

// 时间类
class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    // 获取当前时间
    static Timestamp now();
    // 转为字符串表示
    std::string toString() const;
    
    // 是否有效（微秒数大于 0）
    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    // 返回自 epoch 以来的微秒数
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

// 在给定时间上加上 seconds 秒，返回新时间戳
inline Timestamp addTime(const Timestamp &timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

// 比较两个时间戳的先后
inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

// 判断两个时间戳是否相等
inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}