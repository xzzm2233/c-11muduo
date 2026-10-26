#pragma once

#include "noncopyable.h"

#include <memory>
#include <string>
#include <mutex>        
#include <ctime>         
#include <sys/types.h>     

class LogFile : noncopyable
{
public:
    LogFile(const std::string &basename,
            off_t rollSize,
            bool threadSafe = true,
            int flushInterval = 3,
            int checkEveryN = 1024);
    ~LogFile();

    // 追加日志
    void append(const char *logline, int len);
    // 刷新到磁盘
    void flush();
    // 滚动日志文件
    bool rollFile();

private:
    // 不加锁的追加日志
    void append_unlocked(const char *logline, int len);

    // 生成日志文件名
    static std::string getLogFileName(const std::string &basename, time_t *now);

    const std::string basename_;
    const off_t rollSize_;
    const int flushInterval_; // flush间隔，默认3秒
    const int checkEveryN_;

    int count_;
    off_t writtenBytes_;

    std::unique_ptr<std::mutex> mutex_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;

    FILE *fp_;

    const static int kRollPerSeconds_ = 60 * 60 * 24; // 每天的秒数
};