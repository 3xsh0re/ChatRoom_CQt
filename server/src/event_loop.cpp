#include "event_loop.h"

namespace mtd {
__thread EventLoop *t_loop_in_this_thread = nullptr;
const int kPollTimeout = 10000;

int CreateEventFd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG_FATAL("func:%s, eventfd error:%d\n", __FUNCTION__, errno);
  }
  return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      calling_pending_functors_(false),
      thread_id_(current_thread::Tid()),
      poller_(Poller::NewDefaultePoller(this)),
      wakeup_fd_(CreateEventFd()),
      wakeup_channel_(new Channel(this, wakeup_fd_)) {
  LOG_DEBUG("func:%s, EventLoop created %p in thread %d", __FUNCTION__, this,
            thread_id_);
  if (t_loop_in_this_thread) {
    LOG_FATAL("func:%s, another eventloop %p exist in this thread %d",
              __FUNCTION__, t_loop_in_this_thread, thread_id_);
  } else {
    t_loop_in_this_thread = this;
  }
  wakeup_channel_->SetReadCallBack(std::bind(&EventLoop::HandleRead, this));
  wakeup_channel_->EnableReading();
}

EventLoop::~EventLoop() {
  wakeup_channel_->DisableAll();
  wakeup_channel_->Remove();
  ::close(wakeup_fd_);
  t_loop_in_this_thread = nullptr;
}

void EventLoop::HandleRead() {
  uint64_t one = 1;
  ssize_t n = read(wakeup_fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_ERROR("func:%s, event_loop, reads %d bytes instead of 8", __FUNCTION__,
              n);
  }
}

void EventLoop::Loop() {
  looping_ = true;
  quit_ = false;
  LOG_INFO("func:%s, %p loopping", __FUNCTION__, this);
  while (!quit_) {
    active_channels_.clear();
    pool_return_time_ = poller_->Poll(kPollTimeout, &active_channels_);
    for (auto &channel : active_channels_) {
      channel->HandleEvent(pool_return_time_);
    }
    DoPendingFunctors();
  }
}

void EventLoop::Quit() {
  quit_ = true;
  if (!IsInLoopThread()) {
    WakeUp();
  }
}

void EventLoop::RunInLoop(Functor cb) {
  if (IsInLoopThread()) {
    cb();
  } else {
    QueueInLoop(cb);
  }
}

void EventLoop::QueueInLoop(Functor cb) {
  {
    std::unique_lock<std::mutex> lock(mtx_);
    pending_functors_.emplace_back(std::move(cb));
  }
  if (!IsInLoopThread() || calling_pending_functors_) {
    WakeUp();
  }
}

void EventLoop::WakeUp() {
  uint64_t one = 1;
  ssize_t n = ::write(wakeup_fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_ERROR("func:%s, event_loop, write in %d", __FUNCTION__, n);
  }
}

void EventLoop::UpdateChannel(Channel *channel) {
  poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel *channel) {
  poller_->RemoveChannel(channel);
}

bool EventLoop::HasChennel(Channel *channel) {
  return poller_->HasChannel(channel);
}

void EventLoop::DoPendingFunctors() {
  std::vector<Functor> functors;
  calling_pending_functors_ = true;
  {
    std::unique_lock<std::mutex> lock(mtx_);
    functors.swap(pending_functors_);
  }

  for (auto &func : functors) {
    func();
  }

  calling_pending_functors_ = false;
}

}  // namespace mtd
