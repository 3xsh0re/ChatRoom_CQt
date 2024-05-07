#include "channel.h"

namespace mtd {
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {}

Channel::~Channel() {}

void Channel::Tie(const std::shared_ptr<void> &obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::HandleEvent(Timestamp recv_time) {
  if (tied_) {
    std::shared_ptr<void> guard;
    guard = tie_.lock();
    if (guard) {
      HandleEventWithGuard(recv_time);
    }
  } else {
    HandleEventWithGuard(recv_time);
  }
}

void Channel::HandleEventWithGuard(Timestamp recv_time) {
  LOG_INFO("func:%s, channel HandleEvent revents:%d\n", __FUNCTION__, revents_);
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
    if (close_cb_) {
      close_cb_();
    }
  }

  if (revents_ & EPOLLERR) {
    if (error_cb_) {
      error_cb_();
    }
  }

  if (revents_ & (EPOLLIN | EPOLLPRI)) {
    if (read_cb_) {
      read_cb_(recv_time);
    }
  }

  if (revents_ & EPOLLOUT) {
    if (write_cb_) {
      write_cb_();
    }
  }
}

void Channel::Update() { loop_->UpdateChannel(this); }

void Channel::Remove() { loop_->RemoveChannel(this); }

}  // namespace mtd
