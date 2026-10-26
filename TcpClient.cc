#include "EventLoop.h"
#include "Connector.h"
#include "TcpClient.h"
#include "Logger.h"

#include <stdio.h>
#include <functional>
#include <cstring>

// 获取 socket 的本地地址
static struct sockaddr_in getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, (sockaddr *)&localaddr, &addrlen) < 0)
    {
        LOG_ERROR("getsockname error");
    }
    return localaddr;
}

// 获取 socket 的对端地址
static struct sockaddr_in getPeerAddr(int sockfd)
{
    struct sockaddr_in peeraddr;
    memset(&peeraddr, 0, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, (sockaddr *)&peeraddr, &addrlen) < 0)
    {
        LOG_ERROR("getpeername error");
    }
    return peeraddr;
}

// 构造函数：创建 Connector 并注册新连接回调
TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),
      name_(nameArg),
      connectionCallback_(),
      messageCallback_(),
      retry_(false),
      connect_(true),
      nextConnId_(1)
{
    connector_->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
    LOG_INFO("TcpClient::TcpClient[%s] - connector %p", name_, connector_.get());
}

// 析构函数：销毁已建立的连接或停止连接器
TcpClient::~TcpClient()
{
    LOG_INFO("TcpClient::TcpClient[%s] - connector %p", name_, connector_.get());
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn)
    {
        loop_->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        if (unique)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connection_.reset();
        }
    }
    else
    {
        connector_->stop();
    }
}

// 发起连接：启动连接器连接服务器
void TcpClient::connect()
{
    LOG_INFO("TcpClient::connect[%s] - connecting to %s",
             name_, connector_->serverAddress().toIpPort());
    connect_ = true;
    connector_->start();
}

// 断开连接：关闭当前连接
void TcpClient::disconnect()
{
    connect_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

// 停止客户端：停止连接器
void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

// 连接建立成功后的回调：创建 TcpConnection 并建立连接
void TcpClient::newConnection(int sockfd)
{
    InetAddress peerAddr(getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    InetAddress localAddr(getLocalAddr(sockfd));

    TcpConnectionPtr conn(new TcpConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

// 连接关闭时的回调：清空连接，必要时按重试策略重连
void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_)
    {
        LOG_INFO("TcpClient::connect [%s] - Reconnecting to %s", name_, connector_->serverAddress().toIpPort());
        connector_->restart();
    }
}
