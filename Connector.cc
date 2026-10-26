#include "Connector.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Channel.h"

#include <errno.h>
#include <cstring>

const int Connector::kMaxRetryDelayMs;

// 判断 socket 是否自己连接了自己（本地地址与对端地址相同）
static bool isSelfConnect(int sockfd)
{
    struct sockaddr_in localaddr = {0};
    struct sockaddr_in peeraddr = {0};
    socklen_t addrlen = sizeof localaddr;

    if (::getsockname(sockfd, (struct sockaddr *)&localaddr, &addrlen) < 0)
        return false;

    addrlen = sizeof peeraddr;
    if (::getpeername(sockfd, (struct sockaddr *)&peeraddr, &addrlen) < 0)
        return false;

    return localaddr.sin_port == peeraddr.sin_port &&
           localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}

// 获取 socket 的挂起错误码
static int getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        return errno;
    return optval;
}

// 构造函数：记录所属 loop 和服务器地址，初始化状态
Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
    LOG_DEBUG("ctor[%p]", this);
}

Connector::~Connector()
{
    LOG_DEBUG("dtor[%p]", this);
}

// 启动连接：转交 loop 线程执行
void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, shared_from_this()));
}

// 在 loop 线程中根据 connect_ 标志决定是否发起连接
void Connector::startInLoop()
{
    if (connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG("do not connect");
    }
}

// 重新发起连接：重置状态与重试延时
void Connector::restart()
{
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

// 停止连接：转交 loop 线程执行
void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, shared_from_this()));
}

// 在 loop 线程中停止连接，若正在连接则断开并回收
void Connector::stopInLoop()
{
    if (state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

// 创建非阻塞 socket 并发起连接，根据返回值决定成功、重试或报错
void Connector::connect()
{
    int sockfd = ::socket(AF_INET, SOCK_NONBLOCK | SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d creat connect sockfd err:%d", __FILE__, __FUNCTION__, __LINE__, errno);
    }

    int ret = ::connect(sockfd, (sockaddr *)serverAddr_.getSockAddr(), static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EFAULT:
    case ENOTSOCK:
        LOG_ERROR("connect error in Connector::startInLoop %d", savedErrno);
        ::close(sockfd);
        break;

    default:
        LOG_ERROR("Unexpected error in Connector::startInLoop %d", savedErrno);
        ::close(sockfd);
        break;
    }
}

// 进入连接中状态，为 socket 创建 Channel 并监听写事件
void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, shared_from_this()));
    channel_->setErrorCallback(std::bind(&Connector::handleError, shared_from_this()));

    channel_->enableWriting();
}

// 写事件触发，判断连接是否建立成功，失败则重试
void Connector::handleWrite()
{
    LOG_DEBUG("Connector::handleWrite %d", state_);

    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = getSocketError(sockfd);
        if (err)
        {
            LOG_INFO("Connector::handleWrite - SO_ERROR = %d", err);
            retry(sockfd);
        }
        else if (isSelfConnect(sockfd))
        {
            LOG_INFO("Connector::handleWrite - Self connect");
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
        }
    }
}

// 连接出错，移除 Channel 并重试
void Connector::handleError()
{
    LOG_ERROR("Connector::handleError state=%d", state_);
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = getSocketError(sockfd);
        LOG_DEBUG("SO_ERROR = %d", err);
        retry(sockfd);
    }
}

// 关闭 socket，按指数退避延时后重新连接
void Connector::retry(int sockfd)
{
    ::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO("Connector::retry - Retry connecting to %s in %d milliseconds.",
                 serverAddr_.toIpPort(), retryDelayMs_);
        loop_->runAfter(retryDelayMs_ / 1000.0,
                        std::bind(&Connector::startInLoop, shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG("do not connect");
    }
}

// 禁用并移除当前 Channel，返回其 fd，并调度重置 Channel
int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();

    loop_->queueInLoop(std::bind(&Connector::resetChannel, shared_from_this()));
    return sockfd;
}

// 重置 Channel 指针以释放资源
void Connector::resetChannel()
{
    channel_.reset();
}
