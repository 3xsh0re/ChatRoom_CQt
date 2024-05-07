#include "poller.h"

namespace mtd
{
  Poller::Poller(EventLoop *loop) : loop_(loop) {}

  Poller::~Poller() {}
  bool Poller::HasChannel(Channel *channel) const {
    auto it = channels_.find(channel->get_fd());
    return it != channels_.end() && it->second == channel;
  }
} // namespace mtd
