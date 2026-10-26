#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <cstring>

// 析构函数：关闭 socket 描述符
Socket::~Socket()
{
    close(sockfd_);
}

// 将 socket 绑定到指定地址
void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail", sockfd_);
    }
}
// 开始监听连接请求
void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail", sockfd_);
    }
}

// 接受一个新连接，返回连接 fd，并把对端地址写入 peeraddr
int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    memset(&addr, 0, sizeof addr);
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

// 关闭写端（半关闭连接）
void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

// 设置 TCP_NODELAY 选项，禁用 Nagle 算法
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

// 设置 SO_REUSEADDR 选项，允许地址复用
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

// 设置 SO_REUSEPORT 选项，允许多个 socket 绑定同一端口
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

// 设置 SO_KEEPALIVE 选项，开启保活机制
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}