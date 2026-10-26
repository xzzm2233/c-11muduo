#include "PollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <algorithm>

PollPoller::PollPoller(EventLoop *loop)
    : Poller(loop)
{
}

PollPoller::~PollPoller() = default;

// 调用 ::poll 等待事件，返回发生的事件个数并填充就绪 Channel
Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_INFO("%d events happened", numEvents);
        fillActiveChannel(numEvents, activeChannels);
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout!", __FUNCTION__);
    }
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("PollPoller::poll()");
        }
    }
    return now;
}

// 注册/更新 Channel，同步 pollfd 数组与 channels_ 映射
void PollPoller::updateChannel(Channel *channel)
{
    LOG_INFO("func=%s => fd=%d events=%d", __FUNCTION__, channel->fd(), channel->events());

    if (channel->index() < 0)
    {
        pollfd pfd{};
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    }
    else
    {
        int idx = channel->index();
        struct pollfd &pfd = pollfds_[idx];
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;

        if (channel->isNoneEvent())
        {
            // 把fd设为一个负数
            pfd.fd = -channel->fd() - 1;
        }
    }
}

// 从 Poller 中移除 Channel，并用末尾元素填补空缺
void PollPoller::removeChannel(Channel *channel)
{
    LOG_INFO("func=%s => fd=%d", __FUNCTION__, channel->fd());
    int idx = channel->index();
    channels_.erase(channel->fd());
    if (static_cast<size_t>(idx) == pollfds_.size() - 1)
    {
        pollfds_.pop_back(); 
    }
    else
    {
        int channelAtEnd = pollfds_.back().fd;
        std::iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        if (channelAtEnd < 0)
        {
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}

// 遍历 pollfds，把发生事件的 Channel 填入 activeChannels
void PollPoller::fillActiveChannel(int numEvents,
                                   ChannelList *activeChannels) const
{
    for (PollFdList::const_iterator pfd = pollfds_.begin();
         pfd != pollfds_.end() && numEvents > 0; ++pfd)
    {
        if (pfd->revents > 0)
        {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            Channel *channel = ch->second;
            channel->set_revents(pfd->revents);
            activeChannels->push_back(channel);
        }
    }
}
