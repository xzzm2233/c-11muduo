#pragma once

#include "noncopyable.h"

#include <string>

class AsyncLogging;

// LOG_INFO("%s %d", arg1 , arg2)
#define LOG_INFO(logmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::GetInstance();           \
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::GetInstance();           \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::GetInstance();           \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::GetInstance();           \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

// 定义日志的级别 INFO ERROR FATAL DEBUG
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出一个日志类
class Logger : noncopyable
{
public:
    // 获取单例
    static Logger &GetInstance();

    // 设置日志级别
    void setLogLevel(int level);

    // 输出日志
    void log(const char* msg);

    // 设置异步日志对象
    void setAsyncLogging(AsyncLogging *async) { asyncLogging_ = async; }

private:
    int logLevel_;
    
    Logger()
        : logLevel_(INFO),
          asyncLogging_(nullptr) {}

    AsyncLogging *asyncLogging_; // 异步日志
};