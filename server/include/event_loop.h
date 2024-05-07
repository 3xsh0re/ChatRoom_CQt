#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <iostream>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>

#include "current_thread.h"
#include "noncopyable.h"
#include "timestamp.h"
#include "logger.h"
#include "poller.h"
namespace mtd {
class Channel;
class Poller;
class EventLoop {
 public:
  using Functor = std::function<void()>;
  EventLoop();
  ~EventLoop();

  void Loop();

  void Quit();

  Timestamp get_pool_return_time() const { return pool_return_time_; }

  void RunInLoop(Functor cb);

  void QueueInLoop(Functor cb);

  void WakeUp();

  void UpdateChannel(Channel *channel);

  void RemoveChannel(Channel *channel);

  bool HasChennel(Channel *channel);

  bool IsInLoopThread() const { return thread_id_ == current_thread::Tid(); }

 private:
  using ChannelList = std::vector<Channel *>;

  void HandleRead();

  void DoPendingFunctors();

  std::atomic_bool looping_;
  std::atomic_bool quit_;
  std::atomic_bool calling_pending_functors_;

  const pid_t thread_id_;

  Timestamp pool_return_time_;
  std::unique_ptr<Poller> poller_;

  int wakeup_fd_;
  std::unique_ptr<Channel> wakeup_channel_;

  ChannelList active_channels_;
  std::vector<Functor> pending_functors_;
  std::mutex mtx_;
};
}  // namespace mtd

#endif