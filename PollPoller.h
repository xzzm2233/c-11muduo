#pragma once

#include "Poller.h"

#include <vector>
#include <sys/poll.h>

class PollPoller : public Poller
{
public:
    PollPoller(EventLoop *loop);
    ~PollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    void fillActiveChannel(int numEvents,
                           ChannelList *activeChannels) const;

    using PollFdList = std::vector<pollfd>;
    PollFdList pollfds_;
};