#include "poller.h"
#include "epollpoller.h"

namespace mtd
{
  Poller *Poller::NewDefaultePoller(EventLoop *loop) {
    if (::getenv("MUDUO_USE_POLL")) {
      return nullptr;
    } else {
      return new EpollPoller(loop);
    }
  } 
} // namespace mtd
