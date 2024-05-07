#ifndef TCP_CONNECTINO_H
#define TCP_CONNECTINO_H

#include "address.h"
#include "callback.h"
#include "channel.h"
#include "event_loop.h"
#include "logger.h"
#include "m_buffer.h"
#include "m_socket.h"
#include "m_thread.h"
#include "timestamp.h"
namespace mtd {
class TcpServer;
class TcpConnection : public noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
 public:
  friend class TcpServer;
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                const InetAddress &local_addr, const InetAddress &peer_addr_);

  ~TcpConnection();

  EventLoop *get_loop() const { return loop_; }

  const std::string &get_name() const { return name_; }
  const InetAddress &get_local_addr() const { return local_addr_; }
  const InetAddress &get_peer_addr() const { return peer_addr_; }

  const int get_fd() const { return channel_->get_fd(); }

  bool IsConnected() const { return state_ == kConnected; }

  void Send(const std::string &buf);

  void set_connection_cb(const ConnectionCallback &cb) { conn_cb_ = cb; }

  void set_msg_cb(const MessageCallback &cb) { msg_cb_ = cb; }

  void set_write_complete_cb(const WriteCompleteCallback &cb) {
    write_complete_cb_ = cb;
  }

  void set_high_water_mark_cb(const HighWaterMarkCallback &cb) {
    high_water_mark_cb_ = cb;
  }

  void set_close_cb(const CloseCallback &cb) { close_cb_ = cb; }

  void Shutdown();

 private:
  void ConnectionEstablished();

  void ConnectDestroyed();

  void HandleRead(Timestamp recv_time);

  void HandleWrite();

  void HandleClose();

  void HandleError();

  void SendInLoop(const std::string &buf);

  void ShutdownInLoop();

  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

  void set_state(StateE state) { state_ = state; }

  EventLoop *loop_;
  const std::string name_;
  std::atomic_int state_;
  bool reading_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress local_addr_;
  const InetAddress peer_addr_;

  ConnectionCallback conn_cb_;
  MessageCallback msg_cb_;
  WriteCompleteCallback write_complete_cb_;
  HighWaterMarkCallback high_water_mark_cb_;
  CloseCallback close_cb_;
  size_t high_water_mark_;
  Buffer input_buf_;
  Buffer output_buf_;
};
}  // namespace mtd

#endif