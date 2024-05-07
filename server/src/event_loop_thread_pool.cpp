#include "event_loop_thread_pool.h"

namespace mtd {
EventLoopThreadPool::EventLoopThreadPool(EventLoop *loop,
                                         const std::string &name)
    : base_loop_(loop),
      name_(name),
      started_(false),
      next_(0),
      num_threads_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::Start(const ThreadInitCallback &cb) {
  started_ = true;
  if (num_threads_ == 0 && cb) {
    cb(base_loop_);
  }
  for (int i = 0; i < num_threads_; ++i) {
    char buf[name_.size() + 32];
    ::snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
    threads_.emplace_back(
        std::make_unique<EventLoopThread>(cb, std::string(buf)));
    loops_.push_back(threads_.back()->StartLoop());
  }
}

// 轮询
EventLoop *EventLoopThreadPool::get_next_loop() {
  EventLoop *loop = base_loop_;
  if (!loops_.empty()) {
    loop = loops_[next_];
    ++next_;
    if (next_ >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::get_loops() {
  if (loops_.empty()) {
    return std::vector<EventLoop *>(1, base_loop_);
  } else {
    return loops_;
  }
}

}  // namespace mtd
