#pragma once

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "TcpConnection.h"
#include "AsyncLogging.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    // 设置线程初始化回调
    void setThreadInitcallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    // 设置连接回调
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    // 设置消息回调
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    // 设置写完成回调
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置底层subLoop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

private:
    // 新连接到来时的回调
    void newConnection(int sockfd, const InetAddress &peerAddr);
    // 移除连接（转交主循环）
    void removeConnection(const TcpConnectionPtr &conn);
    // 在主循环中移除连接
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; 
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; 

    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;       
    MessageCallback messageCallback_;             
    WriteCompleteCallback writeCompleteCallback_; 

    ThreadInitCallback threadInitCallback_; // loop线程初始化的回调

    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; 
};