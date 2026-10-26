#pragma once
#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // 处理就绪事件
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 绑定 Channel 的持有者，防止回调期间对象被销毁
    void tie(const std::shared_ptr<void> &);

    // 返回被监听的 fd
    int fd() { return fd_; }
    // 返回当前关心的事件
    int events() const { return events_; }
    // 记录 Poller 返回的就绪事件
    void set_revents(int revt) { revents_ = revt; }

    // 开启读事件监听
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    // 关闭读事件监听
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    // 开启写事件监听
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    // 关闭写事件监听
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    // 关闭所有事件监听
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // 返回 Channel 在 Poller 中的索引
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // 返回所属的 EventLoop
    EventLoop *ownerLoop() { return loop_; }
    // 从所属 EventLoop 中移除当前 Channel
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    EventLoop *loop_; // 事件循环
    const int fd_;    // Poller监听的对象
    int events_;      
    int revents_;     // Poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};