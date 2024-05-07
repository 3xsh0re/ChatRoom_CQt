#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/epoll.h>

#include <functional>
#include <iostream>
#include <memory>

#include "event_loop.h"
#include "logger.h"
#include "noncopyable.h"
#include "timestamp.h"
namespace mtd {
class EventLoop;
class Timestamp;

class Channel : public noncopyable {
 public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  Channel(EventLoop *loop, int fd);
  ~Channel();

  void HandleEvent(Timestamp receiveTime);

  void SetReadCallBack(ReadEventCallback cb) { read_cb_ = std::move(cb); }
  void SetWriteCallBack(EventCallback cb) { write_cb_ = std::move(cb); }
  void SetCloseCallBack(EventCallback cb) { close_cb_ = std::move(cb); }
  void SetErrorCallBack(EventCallback cb) { error_cb_ = std::move(cb); }

  void Tie(const std::shared_ptr<void> &);

  int get_fd() const { return fd_; }

  int get_events() const { return events_; }

  void set_revents(int rev) { revents_ = rev; }

  void EnableReading() {
    events_ |= kReadEvent;
    Update();
  }

  void DisableReading() {
    events_ &= ~kReadEvent;
    Update();
  }

  void EnableWriting() {
    events_ |= kWriteEvent;
    Update();
  }

  void DisableWriting() {
    events_ &= ~kWriteEvent;
    Update();
  }

  void DisableAll() {
    events_ = kNoneEvent;
    Update();
  }

  bool IsNoneEvent() { return events_ == kNoneEvent; }
  bool IsWriting() { return events_ & kWriteEvent; }
  bool IsReading() { return events_ & kReadEvent; }

  int get_index() const { return index_; }
  void set_index(int idx) { index_ = idx; }

  EventLoop *get_owner_loop() const { return loop_; }
  void Remove();

 private:
  void Update();

  void HandleEventWithGuard(Timestamp recv_time);

  static const int kReadEvent;
  static const int kNoneEvent;
  static const int kWriteEvent;

  EventLoop *loop_;
  const int fd_;
  int events_;
  int revents_;
  int index_;

  std::weak_ptr<void> tie_;
  bool tied_;

  ReadEventCallback read_cb_;
  EventCallback write_cb_;
  EventCallback close_cb_;
  EventCallback error_cb_;
};
}  // namespace mtd

#endif