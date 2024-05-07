#ifndef EVENT_LOOP_THREAD_H
#define EVENT_LOOP_THREAD_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <memory>

#include "event_loop.h"
#include "m_thread.h"
#include "noncopyable.h"
namespace mtd {
class EventLoopThread : public noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                  const std::string &name = std::string());

  ~EventLoopThread();

  EventLoop* StartLoop();
 private:
  void ThreadFunc(); 
  EventLoop *loop_;
  std::string name_;
  bool exiting_;
  Thread thread_;
  std::mutex mtx_;
  std::condition_variable cv_;
  ThreadInitCallback cb_;
};
}  // namespace mtd

#endif