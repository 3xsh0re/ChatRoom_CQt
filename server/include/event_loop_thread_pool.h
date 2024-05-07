#ifndef EVENT_LOOP_THREAD_POOL_H
#define EVENT_LOOP_THREAD_POOL_H

#include "event_loop_thread.h"

namespace mtd {

class EventLoopThreadPool {
 public:
  using ThreadInitCallback = EventLoopThread::ThreadInitCallback;

  EventLoopThreadPool(EventLoop *loop, const std::string &name);

  ~EventLoopThreadPool();

  void set_threads_num(int num) { num_threads_ = num; }

  void Start(const ThreadInitCallback &cb = ThreadInitCallback());

  EventLoop *get_next_loop();

  std::vector<EventLoop *> get_loops();

  bool get_started() const { return started_; }

  const std::string get_name() const { return name_; }

 private:
  EventLoop *base_loop_;
  std::string name_;
  bool started_;
  int num_threads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;
};

}  // namespace mtd

#endif