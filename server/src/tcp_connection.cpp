#include "tcp_connection.h"

namespace mtd {
static EventLoop *CheckLoopNotNull(EventLoop *loop) {
  if (loop == nullptr) {
    LOG_FATAL("func:%s, main loop is null!", __FUNCTION__);
  }
  return loop;
}
TcpConnection::TcpConnection(EventLoop *loop, const std::string &name,
                             int sockfd, const InetAddress &local_addr,
                             const InetAddress &peer_addr)
    : loop_(CheckLoopNotNull(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(std::make_unique<Socket>(sockfd)),
      channel_(std::make_unique<Channel>(loop, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr),
      high_water_mark_(64 * 1024 * 1024) {
  channel_->SetReadCallBack(
      std::bind(&TcpConnection::HandleRead, this, std::placeholders::_1));
  channel_->SetWriteCallBack(std::bind(&TcpConnection::HandleWrite, this));
  channel_->SetCloseCallBack(std::bind(&TcpConnection::HandleClose, this));
  channel_->SetErrorCallBack(std::bind(&TcpConnection::HandleError, this));
  LOG_INFO("TcpConnection, func:%s, Tcpconnection::ctor[%s] at fd=%d",
           __FUNCTION__, name_.c_str(), sockfd);
  socket_->SetKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_INFO("TcpConnection, func:%s, Tcpconnection::dtor[%s] at fd=%d, state=%d",
           __FUNCTION__, name_.c_str(), channel_->get_fd(), (int)state_);
}

void TcpConnection::HandleRead(Timestamp recv_time) {
  int save_errono{};
  ssize_t n = input_buf_.ReadFd(socket_->get_fd(), save_errono);
  if (n > 0) {
    msg_cb_(shared_from_this(), input_buf_, recv_time);
  } else if (n == 0) {
    HandleClose();
  } else {
    errno = save_errono;
    LOG_ERROR("func:%s, Tcpconnection : handle read", __FUNCTION__);
    HandleError();
  }
}

void TcpConnection::HandleWrite() {
  if (channel_->IsWriting()) {
    int save_errono{};
    ssize_t n = output_buf_.WriteFd(socket_->get_fd(), save_errono);
    if (n > 0) {
      output_buf_.Retrive(n);
      if (output_buf_.ReadeableBytes() == 0) {
        channel_->DisableWriting();
        if (write_complete_cb_) {
          loop_->QueueInLoop(std::bind(write_complete_cb_, shared_from_this()));
        }
        if (state_ == kDisconnecting) {
          ShutdownInLoop();
        }
      } else {
        LOG_ERROR("func:%s, Tcpconnection:: handle write", __FUNCTION__);
      }
    } else {
      LOG_ERROR("func:%s, Tcpconnection fd=%d is down, no more writing",
                __FUNCTION__, channel_->get_fd());
    }
  }
}

void TcpConnection::HandleClose() {
  LOG_INFO("TcpConnection, func:%s, fd=%d, state=%d", __FUNCTION__,
           channel_->get_fd(), (int)state_);
  set_state(kDisconnected);
  channel_->DisableAll();

  TcpConnectionPtr conn_ptr(shared_from_this());
  conn_cb_(conn_ptr);
  close_cb_(conn_ptr);
}

void TcpConnection::HandleError() {
  int optval;
  socklen_t optlen = sizeof(optval);
  int err{};
  if (::getsockopt(channel_->get_fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) <
      0) {
    err = errno;
  } else {
    err = optval;
  }
  LOG_ERROR("func:%s, Tcpconnection::handle error name:%s - SO_ERROR:%d",
            __FUNCTION__, name_.c_str(), err);
}

void TcpConnection::Send(const std::string &buf) {
  if (state_ == kConnected) {
    if (loop_->IsInLoopThread()) {
      SendInLoop(buf);
    } else {
      loop_->RunInLoop(std::bind(&TcpConnection::SendInLoop, this, buf));
    }
  }
}

void TcpConnection::Shutdown() {
  if (state_ == kConnected) {
    set_state(kDisconnected);
    loop_->RunInLoop(std::bind(&TcpConnection::ShutdownInLoop, this));
  }
}

void TcpConnection::ConnectionEstablished() {
  set_state(kConnected);
  channel_->Tie(shared_from_this());
  channel_->EnableReading();
  conn_cb_(shared_from_this());
}

void TcpConnection::ConnectDestroyed() {
  if (state_ == kConnected) {
    set_state(kDisconnected);
    channel_->DisableAll();
    conn_cb_(shared_from_this());
  }
  channel_->Remove();
}

void TcpConnection::SendInLoop(const std::string &str) {
  const void *msg = str.data();
  size_t len = str.length();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool fault_error = false;
  if (state_ == kDisconnected) {
    LOG_ERROR("func:%s, TcpConnection, disconnected, dive up writing",
              __FUNCTION__);
    return;
  }
  if (!channel_->IsWriting() && output_buf_.ReadeableBytes() == 0) {
    nwrote = ::write(channel_->get_fd(), msg, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && write_complete_cb_) {
        loop_->QueueInLoop(std::bind(write_complete_cb_, shared_from_this()));
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_ERROR("func:%s, TcpConnection::SendInLoop", __FUNCTION__);
        if (errno == EPIPE || errno == ECONNRESET) {
          fault_error = true;
        }
      }
    }
  }

  if (!fault_error && remaining > 0) {
    size_t old_len = output_buf_.ReadeableBytes();
    if (old_len + remaining >= high_water_mark_ && old_len < high_water_mark_) {
      loop_->QueueInLoop(std::bind(high_water_mark_cb_, shared_from_this(),
                                   old_len + remaining));
    }
    output_buf_.Append((char *)msg + nwrote, remaining);
    if (!channel_->IsWriting()) {
      channel_->EnableWriting();
    }
  }
}

void TcpConnection::ShutdownInLoop() {
  if (!channel_->IsWriting()) {
    socket_->ShutdownWrite();
  }
}
}  // namespace mtd
