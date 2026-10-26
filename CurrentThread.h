#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid;

    // 缓存当前线程 id
    void cacheTid();

    // 返回当前线程 id，首次调用时先缓存
    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}
