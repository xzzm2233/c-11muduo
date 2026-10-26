#pragma once

#include "noncopyable.h"

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    // 启动线程
    void start();
    // 等待线程结束
    void join();

    // 是否已启动
    bool started() const { return started_; }
    // 返回线程 id
    pid_t tid() const { return tid_; }
    // 返回线程名字
    const std::string &name() const { return name_; }

    // 返回已创建线程的总数
    static int numCreated() { return numCreated_; }

private:
    // 生成默认线程名
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int32_t numCreated_;
};