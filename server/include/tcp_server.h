#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "acceptor.h"
#include "callback.h"
#include "event_loop_thread_pool.h"
#include "tcp_connection.h"
namespace mtd {
class TcpServer {
 public:
  using ThreadInitCallBack = EventLoopThread::ThreadInitCallback;

  enum Option {
    kNoReusePort,
    kReusePort,
  };

  TcpServer(EventLoop *loop, const InetAddress &listen_addr,
            const std::string name, Option option = kNoReusePort);

  ~TcpServer();

  void set_thread_num(int num);

  void set_thread_init_cb(const ThreadInitCallBack &cb) {
    thread_init_cb_ = cb;
  }

  void set_connection_cb(const ConnectionCallback &cb) { connection_cb_ = cb; }

  void set_msg_cb(const MessageCallback &cb) { message_cb_ = cb; }

  void set_write_complete_cb(const WriteCompleteCallback &cb) {
    write_complete_cb_ = cb;
  }

  void Start();

 private:
  void NewConnection(int fd, const InetAddress &peeraddr);

  void RemoveConnection(const TcpConnectionPtr &conn);

  void RemoveConnectionInLoop(const TcpConnectionPtr &conn);

  using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
  EventLoop *loop_;
  const std::string ip_port_;
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> thread_pool_;

  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
  WriteCompleteCallback write_complete_cb_;

  ThreadInitCallBack thread_init_cb_;
  std::atomic_int started_;
  int next_connection_id_;
  ConnectionMap connections_;
};
}  // namespace mtd

#endif