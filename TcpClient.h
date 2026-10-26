#pragma once

#include "noncopyable.h"
#include "TcpConnection.h"

#include <atomic>
#include <mutex>

class EventLoop;
class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable
{
public:
    TcpClient(EventLoop *loop,
              const InetAddress &serverAddr,
              const std::string &nameArg);
    ~TcpClient();

    // 发起连接
    void connect();
    // 断开连接
    void disconnect();
    // 停止客户端
    void stop();

    // 返回当前连接（线程安全）
    TcpConnectionPtr connection() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_;
    }

    // 返回所属 EventLoop
    EventLoop *getLoop() const { return loop_; }
    // 是否允许重连
    bool retry() const { return retry_; }
    // 开启断线重连
    void enableRetry() { retry_ = true; }

    // 返回客户端名字
    const std::string &name() const
    {
        return name_;
    }

    // 设置连接回调
    void setConnectionCallback(ConnectionCallback cb)
    {
        connectionCallback_ = std::move(cb);
    }

    // 设置消息回调
    void setMessageCallback(MessageCallback cb)
    {
        messageCallback_ = std::move(cb);
    }

    // 设置写完成回调
    void setWriteCompleteCallback(WriteCompleteCallback cb)
    {
        writeCompleteCallback_ = std::move(cb);
    }

private:
    // 新连接建立的回调
    void newConnection(int sockfd);

    // 连接关闭的回调
    void removeConnection(const TcpConnectionPtr &conn);

    EventLoop *loop_;
    ConnectorPtr connector_;
    const std::string name_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    std::atomic_bool retry_;
    std::atomic_bool connect_;

    int nextConnId_;
    mutable std::mutex mutex_;
    TcpConnectionPtr connection_;
};