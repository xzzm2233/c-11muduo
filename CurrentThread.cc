#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    // 缓存当前线程 id（通过系统调用获取真实线程 id）
    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}