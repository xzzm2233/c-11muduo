#include "Timestamp.h"

#include <ctime>
#include <sys/time.h>

const int Timestamp::kMicroSecondsPerSecond;

// 默认构造函数：初始化为 0（表示无效时间）
Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

// 用微秒数构造时间戳
Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

// 获取当前时间戳
Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr); 
    int64_t micros = static_cast<int64_t>(tv.tv_sec) * kMicroSecondsPerSecond + tv.tv_usec;
    return Timestamp(micros);
}

// 转成 "年/月/日 时:分:秒" 格式的字符串
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    struct tm tm_time;
    localtime_r(&seconds, &tm_time); // 线程安全，写入局部变量
    snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec);
    return std::string(buf);
}