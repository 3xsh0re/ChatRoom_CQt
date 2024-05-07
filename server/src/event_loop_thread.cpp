#include "event_loop_thread.h"

namespace mtd {
EventLoopThread::EventLoopThread(
    const ThreadInitCallback &cb,
    const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::ThreadFunc, this), name),
      mtx_(),
      cv_(),
      cb_(cb) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (loop_ != nullptr) {
    loop_->Quit();
    thread_.Join();
  }
}

EventLoop *EventLoopThread::StartLoop() {
  thread_.Start();
  EventLoop *loop = nullptr;
  {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&]() -> bool { return loop_ != nullptr; });
    loop = loop_;
  }
  return loop;
}

void EventLoopThread::ThreadFunc() {
  EventLoop loop;
  if (cb_) {
    cb_(&loop);
  }
  {
    std::unique_lock<std::mutex> lock(mtx_);
    loop_ = &loop;
    cv_.notify_one();
  }
  loop.Loop();
  std::unique_lock<std::mutex> lock(mtx_);
  loop_ = nullptr;
}
}  // namespace mtd
