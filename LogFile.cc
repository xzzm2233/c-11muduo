#include "LogFile.h"
#include "Logger.h"

// 构造函数：保存配置参数并立即打开（滚动）日志文件
LogFile::LogFile(const std::string &basename,
                 off_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      writtenBytes_(0),
      mutex_(threadSafe ? new std::mutex : nullptr),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0),
      fp_(nullptr)
{
    rollFile();
}

// 析构函数：刷新并关闭文件
LogFile::~LogFile()
{
    if (fp_)
    {
        ::fflush(fp_);
        ::fclose(fp_);
    }
}

// 追加日志，根据是否线程安全决定是否加锁
void LogFile::append(const char *logline, int len)
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> lock(*mutex_);
        append_unlocked(logline, len);
    }
    else
    {
        append_unlocked(logline, len);
    }
}

// 不加锁地写入日志，必要时触发滚动或刷新
void LogFile::append_unlocked(const char *logline, int len)
{
    size_t n = ::fwrite(logline, 1, len, fp_);
    if (n != static_cast<size_t>(len))
    {
        LOG_ERROR("LogFile::append fwrite failed: expected %d, got %zu", len, n);
    }

    writtenBytes_ += n;

    if (writtenBytes_ > rollSize_)
    {
        rollFile();
        return;
    }
    else
    {
        ++count_;
        if (count_ >= checkEveryN_)
        {
            count_ = 0;
            time_t now = ::time(nullptr);
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;

            if (thisPeriod != startOfPeriod_)
            {
                rollFile();
            }
            else if (now - lastFlush_ > flushInterval_)
            {
                lastFlush_ = now;
                flush();
            }
        }
    }
}

// 根据 basename 和时间生成日志文件名
std::string LogFile::getLogFileName(const std::string &basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = ::time(nullptr);
    ::localtime_r(now, &tm);
    ::strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
    filename += timebuf;

    filename += ".log";
    return filename;
}

// 刷新文件缓冲区到磁盘
void LogFile::flush()
{
    if (fp_)
    {
        ::fflush(fp_);
    }
}

// 滚动日志文件：关闭旧文件，打开新文件
bool LogFile::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;

        count_ = 0;
        writtenBytes_ = 0;

        if (fp_)
        {
            ::fflush(fp_);
            ::fclose(fp_);
            fp_ = nullptr;
        }

        fp_ = ::fopen(filename.c_str(), "ae");
        if (!fp_)
        {
            LOG_ERROR("fopen %s failed", filename.c_str());
            return false;
        }
        return true;
    }
    return false;
}