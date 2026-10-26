#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>

// 创建非阻塞、可监听套接字
static int createNoneblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

// 构造函数：创建监听 socket，绑定地址并注册读事件回调
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(createNoneblocking()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false)
{
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr); 
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

// 析构函数：关闭监听 Channel 的所有事件并移除
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

// 开始监听：调用 listen 并开启读事件监听
void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading(); // acceptChannel_ => Poller
}

// 处理新连接到来，接受连接并回调 newConnectionCallback_
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            // 轮询找到subLoop，唤醒，分发当前的新客户端的Channel
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit!", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}