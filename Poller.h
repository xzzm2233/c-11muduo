#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 等待事件就绪，返回发生的事件个数并把就绪 Channel 填入 activeChannels
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    // 注册/更新 Channel 关心的事件
    virtual void updateChannel(Channel *channel) = 0;
    // 移除 Channel
    virtual void removeChannel(Channel *channel) = 0;

    // 判断 Channel 是否在当前 Poller 中
    bool hasChannel(Channel *channel) const;

    // 根据环境变量创建默认的 Poller（优先 epoll，可退化为 poll）
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_; 
};