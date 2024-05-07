#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "address.h"
#include "event_loop.h"
#include "m_socket.h"
namespace mtd {
class Acceptor {
 public:
  using NewConnectionCallback =
      std::function<void(int fd, const InetAddress &addr)>;
  Acceptor(EventLoop *loop, const InetAddress &listen_addr, bool reuse_port);

  ~Acceptor();

  void set_new_connection_cb(const NewConnectionCallback &cb) { new_cb_ = cb; }

  bool get_listenning() const { return listenning_; }

  void Listen();

 private:
  /* data */
  void HandleRead();

  EventLoop *loop_;
  Socket sock_;
  Channel accept_channel_;
  NewConnectionCallback new_cb_;
  bool listenning_;
  int idle_fd_;
};

}  // namespace mtd

#endif