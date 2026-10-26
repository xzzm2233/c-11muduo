#include "Logger.h"
#include "Timestamp.h"
#include "AsyncLogging.h"

#include <iostream>

// 返回单例 Logger 对象
Logger &Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}

// 输出一条日志：格式化后交给异步日志或直接打印到标准输出
void Logger::log(const char *msg)
{
    const char *levelStr = "UNKNOWN";

    switch (logLevel_)
    {
    case INFO:
        levelStr = "INFO";
        break;
    case ERROR:
        levelStr = "ERROR";
        break;
    case FATAL:
        levelStr = "FATAL";
        break;
    case DEBUG:
        levelStr = "DEBUG";
        break;
    default:
        break;
    }

    char buf[2048] = {0};
    int len = snprintf(buf, sizeof buf, "[%s]%s : %s \n",
                       levelStr,
                       Timestamp::now().toString().c_str(),
                       msg);

    if (len < 0)
    {
        return;
    }

    if (len >= static_cast<int>(sizeof buf))
    {
        len = sizeof buf - 1;
    }

    if (asyncLogging_)
    {
        asyncLogging_->append(buf, len);
        return;
    }

    // 打印时间和msg
    std::cout << buf;
}