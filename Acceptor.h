#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
    
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    // 设置新连接到来时的回调
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    // 返回是否正在监听
    bool listenning() const { return listenning_; }
    // 开始监听
    void listen();

private:
    // 处理监听 fd 的可读事件（即新连接到来）
    void handleRead();

    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};