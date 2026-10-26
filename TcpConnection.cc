#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string>

// 校验 EventLoop 指针非空，为空则记录致命日志
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null!", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

// 构造函数：保存连接信息，创建 Socket 与 Channel 并注册各事件回调
TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(nameArg),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) // 64M
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

// 析构函数：仅记录销毁日志，Socket/Channel 由其智能指针负责释放
TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d",
             name_.c_str(), channel_->fd(), (int)state_);
}

// 对外发送接口：确保在 loop 线程内调用 sendInLoop
void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            auto self = shared_from_this();
            loop_->runInLoop([self, buf]()
                             { self->sendInLoop(buf.c_str(), buf.size()); });
        }
    }
}

// 在 loop 线程内真正发送数据：能立即写出则写，否则放入输出缓冲区等待写事件
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nworte = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // channel_第一次开始写数据，并且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nworte = ::write(channel_->fd(), data, len);
        if (nworte >= 0)
        {
            remaining = len - nworte;
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nworte < 0
        {
            nworte = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE RESET
                {
                    faultError = true;
                }
            }
        }
    }

    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char *)data + nworte, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

// 关闭连接：置为正在关闭状态，并在 loop 线程中执行
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

// 在 loop 线程内真正执行关闭写端操作
void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting())
    {
        socket_->shutdownWrite(); // 关闭写端
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

// poller => channel::closeCallback => TcpConnection::handleClose
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

// 处理可写事件：把输出缓冲区剩余数据写回 fd
void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("tcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}

// 处理连接关闭事件：置为断开状态并依次回调连接回调和关闭回调
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);
    closeCallback_(connPtr);
}

// 处理错误事件：读取 SO_ERROR 并记录错误日志
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s -SO_ERROR:%d", name_.c_str(), err);
}