#include "tcp_server.h"

namespace mtd {
EventLoop *CheckLoopNotNull(EventLoop *loop) {
  if (loop == nullptr) {
    LOG_FATAL("func:%s main loop is null!", __FUNCTION__);
  }
  return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listen_addr,
                     const std::string name, Option option)
    : loop_(CheckLoopNotNull(loop)),
      ip_port_(listen_addr.ToIpPort()),
      name_(name),
      acceptor_(
          std::make_unique<Acceptor>(loop, listen_addr, option == kReusePort)),
      thread_pool_(std::make_shared<EventLoopThreadPool>(loop, name_)),
      connection_cb_(),
      message_cb_(),
      next_connection_id_(1),
      started_(0) {
  acceptor_->set_new_connection_cb(std::bind(&TcpServer::NewConnection, this,
                                             std::placeholders::_1,
                                             std::placeholders::_2));
}

TcpServer::~TcpServer() {
  for (auto &item : connections_) {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->get_loop()->RunInLoop(
        std::bind(&TcpConnection::ConnectDestroyed, conn.get()));
  }
}

void TcpServer::set_thread_num(int num) { thread_pool_->set_threads_num(num); }

void TcpServer::Start() {
  if (started_++ == 0) {
    thread_pool_->Start(thread_init_cb_);
    loop_->RunInLoop(std::bind(&Acceptor::Listen, acceptor_.get()));
  }
}

void TcpServer::NewConnection(int fd, const InetAddress &peeraddr) {
  EventLoop *io_loop = thread_pool_->get_next_loop();
  char buf[64]{};
  ::snprintf(buf, sizeof(buf), "%s#%d", ip_port_.c_str(), next_connection_id_);
  ++next_connection_id_;
  std::string conn_name(name_ + buf);
  LOG_INFO(
      "func:%s, TcpServer::NewConnection [%s] - new connection [%s] from %s",
      __FUNCTION__, name_.c_str(), conn_name.c_str(),
      peeraddr.ToIpPort().c_str());
  sockaddr_in local{};
  socklen_t addr_len = sizeof(local);
  if (::getsockname(fd, (sockaddr *)&local, &addr_len) < 0) {
    LOG_ERROR("func:%s, TcpServer,sockets::get local addr", __FUNCTION__);
  }

  InetAddress local_addrr(local);
  TcpConnectionPtr conn = std::make_shared<TcpConnection>(
      io_loop, conn_name, fd, local_addrr, peeraddr);
  connections_[conn_name] = conn;
  conn->set_connection_cb(connection_cb_);
  conn->set_msg_cb(message_cb_);
  conn->set_write_complete_cb(write_complete_cb_);
  conn->set_close_cb(
      std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
  io_loop->RunInLoop(std::bind(&TcpConnection::ConnectionEstablished, conn));
}

void TcpServer::RemoveConnection(const TcpConnectionPtr &conn) {
  loop_->RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr &conn) {
  LOG_INFO(
      "TcpServer func:%s, TcpServer::RemoveConnectionInLoop [%s] - connection "
      "%s",
      __FUNCTION__, name_.c_str(), conn->get_name().c_str());
  connections_.erase(conn->get_name());
  EventLoop *io_loop = conn->get_loop();
  io_loop->QueueInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));
}

}  // namespace mtd
