#pragma once

#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }

    ~Socket();

    // 返回 socket 描述符
    int fd() const { return sockfd_; }
    // 绑定本地地址
    void bindAddress(const InetAddress &localaddr);
    // 监听连接
    void listen();
    // 接受一个新连接
    int accept(InetAddress *peeraddr);

    // 关闭写端
    void shutdownWrite();

    // 以下为设置 socket 选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};