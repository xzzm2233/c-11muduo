#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    // 返回所属 EventLoop
    EventLoop *getLoop() const { return loop_; }
    // 返回连接名字
    const std::string &name() const { return name_; }
    // 返回本地地址
    const InetAddress &localAddress() const { return localAddr_; }
    // 返回对端地址
    const InetAddress &peerAddress() const { return peerAddr_; }

    // 判断是否处于已连接状态
    bool connected() const { return state_ == kConnected; }

    // 发送数据
    void send(const std::string &buf);
    // 关闭连接
    void shutdown();

    // 设置连接建立/关闭回调
    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    // 设置收到消息的回调
    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    // 设置写完成的回调
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    // 设置高水位回调及高水位阈值
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    // 设置关闭回调
    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    // 更新连接状态
    void setState(StateE state) { state_ = state; }

    // 以下为 Channel 的事件回调
    void handleRead(Timestamp receiveTime); // 处理读事件
    void handleWrite();                     // 处理写事件
    void handleClose();                     // 处理关闭事件
    void handleError();                     // 处理错误事件

    // 在 loop 线程内发送数据
    void sendInLoop(const void *data, size_t len);
    // 在 loop 线程内关闭写端
    void shutdownInLoop();

    EventLoop *loop_; 
    const std::string name_;
    std::atomic_int state_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;      
    MessageCallback messageCallback_;             
    WriteCompleteCallback writeCompleteCallback_; 
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_;  
    Buffer outputBuffer_; 
};