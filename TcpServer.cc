#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>
#include <functional>
#include <cstring>

// 校验主循环指针非空
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

// 构造函数：创建 Acceptor 和线程池，并注册新连接回调
TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                  std::placeholders::_1, std::placeholders::_2));
}

// 析构函数：销毁所有连接
TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

// 设置底层 subLoop 的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 启动服务器：只启动一次，启动线程池并让 Acceptor 开始监听
void TcpServer::start()
{
    if (started_++ == 0) 
    {
        threadPool_->start(threadInitCallback_); 
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 处理新连接：挑选一个 subLoop，创建 TcpConnection 并建立连接
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    sockaddr_in local;
    memset(&local, 0, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    TcpConnectionPtr conn(new TcpConnection(
        ioLoop,
        connName,
        sockfd, // Socket Channel
        localAddr,
        peerAddr));
    connections_[connName] = conn;

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

// 转交主循环执行连接的移除操作
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

// 在主循环中真正移除连接，并调度销毁
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s",
             name_.c_str(), conn->name().c_str());

    size_t n = connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
