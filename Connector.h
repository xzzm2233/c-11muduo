#pragma once

#include "noncopyable.h"
#include "InetAddress.h"

#include <memory>
#include <functional>
#include <atomic>

class EventLoop;
class Channel;

class Connector : noncopyable, public std::enable_shared_from_this<Connector>
{
public:
    using NewConnectionCallback = std::function<void(int sockfd)>;

    Connector(EventLoop *loop, const InetAddress &serverAddr);
    ~Connector();

    // 设置连接建立成功后的回调
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    // 启动连接
    void start();
    void restart();
    void stop();

    const InetAddress &serverAddress() const { return serverAddr_; }

private:
    enum States
    {
        kDisconnected,
        kConnecting,
        kConnected,
    };
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    // 更新连接状态
    void setState(States s) { state_ = s; }

    // 在 loop 线程中启动连接
    void startInLoop();
    // 在 loop 线程中停止连接
    void stopInLoop();
    // 创建非阻塞 socket，发起连接
    void connect();
    // 进入连接中状态并监听写事件
    void connecting(int sockfd);
    // EPOLLOUT 触发，判断连接成功还是失败
    void handleWrite();
    // 连接出错处理
    void handleError();
    // 重试连接
    void retry(int sockfd);
    // 移除并重置 Channel，返回其 fd
    int removeAndResetChannel();
    // 释放 Channel 资源
    void resetChannel();

    EventLoop *loop_;
    InetAddress serverAddr_;

    States state_;
    std::unique_ptr<Channel> channel_;

    NewConnectionCallback newConnectionCallback_;

    std::atomic<bool> connect_;
    int retryDelayMs_;
};