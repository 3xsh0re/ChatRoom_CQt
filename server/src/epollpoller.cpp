#include "epollpoller.h"

namespace mtd {
// channel state
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epoll_fd_ < 0) {
    LOG_FATAL("func:%s, errornum: %d", __FUNCTION__, errno);
  }
}

EpollPoller::~EpollPoller() { ::close(epoll_fd_); }

Timestamp EpollPoller::Poll(int timeout_ms, ChannelList *active_channels) {
  LOG_DEBUG("func:%s, channels size = %lu\n", __FUNCTION__, channels_.size());
  int num_events = ::epoll_wait(epoll_fd_, events_.data(),
                                static_cast<int>(events_.size()), timeout_ms);
  int save_errono = errno;
  Timestamp time = Timestamp::Now();
  if (num_events > 0) {
    LOG_DEBUG("func:%s, %d events happend \n", __FUNCTION__, num_events);
    FillActivechannels(num_events, active_channels);
    if (num_events == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (num_events == 0) {
    LOG_DEBUG("func:%s timeout\n", __FUNCTION__);
  } else {
    if (save_errono != EINTR) {
      LOG_ERROR("func:%s, epollpoller, erro num = %d", __FUNCTION__,
                save_errono);
    }
  }
  return time;
}

void EpollPoller::Update(int operation, Channel *channel) {
  int fd = channel->get_fd();
  epoll_event event{};
  event.events = channel->get_events();
  event.data.ptr = channel;
  if (::epoll_ctl(epoll_fd_, operation, fd, &event)) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_ERROR("func:%s, epollpoller, epoll_ctl del error:%d\n", __FUNCTION__,
                errno);
    } else {
      LOG_FATAL("func:%s, epollpoller, epoll_ctl add/mod error:%d\n",
                __FUNCTION__, errno);
    }
  }
}

void EpollPoller::FillActivechannels(int num_events,
                                     ChannelList *active_channels) const {
  Channel *channel{};
  for (int i = 0; i < num_events; ++i) {
    channel = static_cast<Channel *>(events_[i].data.ptr);
    channel->set_revents(events_[i].events);
    active_channels->push_back(channel);
  }
}

void EpollPoller::UpdateChannel(Channel *channel) {
  const int index = channel->get_index();
  int fd = channel->get_fd();
  LOG_INFO("EpollPoller func=%s, fd=%d", __FUNCTION__, fd);

  if (index == kNew || index == kDeleted) {
    if (index == kNew) {
      channels_[fd] = channel;
    }
    channel->set_index(kAdded);
    Update(EPOLL_CTL_ADD, channel);
  } else {
    if (channel->IsNoneEvent()) {
      Update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    } else {
      Update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EpollPoller::RemoveChannel(Channel *channel) {
  int fd = channel->get_fd();
  channels_.erase(fd);
  LOG_INFO("EpollPoller func:%s, fd=%d", __FUNCTION__, fd);
  int index = channel->get_index();
  if (index == kAdded) {
    Update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

}  // namespace mtd
