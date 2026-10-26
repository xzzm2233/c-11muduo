#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int32_t Thread::numCreated_(0);

// 构造函数：保存线程函数和名字，若名字为空则生成默认名字
Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(name)
{
    setDefaultName();
}

// 析构函数：若线程已启动但未 join，则分离线程
Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); 
    }
}

// 启动线程，并用信号量等待线程真正开始运行后才返回
void Thread::start() 
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                           {
        tid_ = CurrentThread::tid();

        sem_post(&sem);

        func_(); }));

    sem_wait(&sem);
}

// 等待线程结束
void Thread::join()
{
    joined_ = true;
    thread_->join();
}

// 若名字为空则生成默认名字 "ThreadN"
void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}