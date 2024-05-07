#ifndef POLLER_H
#define POLLER_H

#include <unordered_map>
#include <vector>

#include "noncopyable.h"
#include "timestamp.h"
#include "channel.h"
#include <stdlib.h>

namespace mtd {
class Channel;
class EventLoop;

class Poller : public noncopyable {
 public:
  using ChannelList = std::vector<Channel *>;
  Poller(EventLoop *loop);
  virtual ~Poller();

  virtual Timestamp Poll(int timeout_ms, ChannelList *active_channels) = 0;
  virtual void UpdateChannel(Channel *channel) = 0;
  virtual void RemoveChannel(Channel *channel) = 0;

  bool HasChannel(Channel *channel) const;

  static Poller *NewDefaultePoller(EventLoop *loop);

 protected:
  using ChannelMap = std::unordered_map<int, Channel *>;//fd:channel*
  ChannelMap channels_;

 private:
  EventLoop *loop_;
};
}  // namespace mtd

#endif