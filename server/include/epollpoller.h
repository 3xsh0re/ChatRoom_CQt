#ifndef EPOLL_POLLER_H
#define EPOLL_POLLER_H
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "channel.h"
#include "poller.h"

namespace mtd {
class EpollPoller : public Poller {
 public:
  EpollPoller(EventLoop *loop);
  ~EpollPoller() override;

  Timestamp Poll(int timeout_ms, ChannelList *active_channels) override;

  void UpdateChannel(Channel *channel) override;

  void RemoveChannel(Channel *channel) override;

 private:
  using EventList = std::vector<epoll_event>;

  static const int kInitEventListSize = 16;

  void FillActivechannels(int num_events, ChannelList *active_channels) const;

  void Update(int operation, Channel *channel);

  int epoll_fd_;
  EventList events_;
};
}  // namespace mtd

#endif
