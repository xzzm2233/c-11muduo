#include "Poller.h"
#include "Channel.h"

// 构造函数：记录所属的 EventLoop
Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop) {}

// 判断参数channel是否在当前Poller中
bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}